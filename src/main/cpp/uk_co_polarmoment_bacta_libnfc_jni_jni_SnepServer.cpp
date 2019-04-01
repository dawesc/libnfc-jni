#include <atomic>
#include <jni.h>
#include <uk_co_polarmoment_bacta_libnfc_jni_jni_SnepServer.h>
#include <gsl/gsl>

/* LLCP from LibNFC */
#include <err.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <nfc/llcp.h>
#include <nfc/llc_service.h>
#include <nfc/llc_link.h>
#include <nfc/mac.h>
#include <nfc/llc_connection.h>
/* LLCP from LibNFC */

#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <vector>

#define THROW(message) \
{ \
	std::stringstream e; \
	e << __FILE__ << ": " << message; \
	throw std::runtime_error(e.str()); \
}

namespace {

std::string GetErrorStr(std::exception_ptr errPtr) {
	try {
		if (errPtr)
			std::rethrow_exception(errPtr);
		return "No error found";
	} catch (const std::exception& e) {
		return e.what();
	} catch (const std::string& e) {
		return e;
	} catch (const char* e) {
		return e;
	}
	return "Unknown error type";
}

enum class SnepCommandType : uint8_t {
CONTINUE = 0x00,      /** Continue */
GET = 0x01,     	  /** GET */
PUT = 0x02,      	  /** PUT */
};
void MacLinkClose(struct mac_link* macLink) {
	if (!macLink) {
		return;
	}
	if (macLink->device)
		nfc_abort_command(macLink->device);

	void *err;
	mac_link_wait(macLink, &err);
	mac_link_free(macLink);
}
void LlcpClose(decltype(llcp_init())* initResult) {
	if (initResult && *initResult >= 0) {
		llcp_fini();
	}
}
void LlcLinkFree(struct llc_link* ptr) {
	if (!ptr)
		return;
	llc_link_free(ptr);
}
void NfcClose(nfc_device* ptr) {
	if (!ptr)
		return;
	nfc_close(ptr);
}

void NfcExit(nfc_context* ptr) {
	if (!ptr)
		return;
	nfc_exit(ptr);
}

class SnepServer : std::enable_shared_from_this<SnepServer> {
public:
	static void StartInstance(JNIEnv *env , jobject thisObj);
	static void StopInstance();
	void StopMacLink(int sig);
	~SnepServer();
    static std::string SHexDump(gsl::span<uint8_t> buf);
	static void* SnepThread(void* arg);

	void ProcessDataArrived(gsl::span<uint8_t> bytesRead);
	void ProcessErrorCallback(std::exception_ptr err);

private:
	static std::shared_ptr<SnepServer> SNEP_INSTANCE;
	SnepServer(JNIEnv *env , jobject thisObj) :
		jenv(env),
		thisObj(thisObj),
		globalRef(env->NewGlobalRef(thisObj)),
		running_(false)
		{ /* Not used */ }

	void Run();
	void Stop();
	void SnepThreadT(struct llc_connection* arg);
	void SnepThreadSafe(struct llc_connection* arg);

	JNIEnv* jenv;
	jobject thisObj;
	jobject globalRef;
	std::atomic<bool> running_;
	std::unique_ptr<struct mac_link, decltype(MacLinkClose)*> macLink         = std::unique_ptr<struct mac_link,       decltype(MacLinkClose)*>(nullptr, MacLinkClose);
	std::unique_ptr<struct llc_link, decltype(LlcLinkFree)*>  llcLink         = std::unique_ptr<struct llc_link,       decltype(LlcLinkFree)*> (nullptr, LlcLinkFree);
	std::unique_ptr<nfc_device, decltype(NfcClose)*> device                   = std::unique_ptr<nfc_device,            decltype(NfcClose)*>    (nullptr, NfcClose);
	std::unique_ptr<decltype(llcp_init()), decltype(LlcpClose)*> llcpInstance = std::unique_ptr<decltype(llcp_init()), decltype(LlcpClose)*>   (nullptr, LlcpClose);
	std::unique_ptr<nfc_context, decltype(NfcExit)*> context                  = std::unique_ptr<nfc_context,           decltype(NfcExit)*>     (nullptr, NfcExit);

};

std::shared_ptr<SnepServer> SnepServer::SNEP_INSTANCE;

void SnepServer::StartInstance(JNIEnv *env , jobject thisObj) {
	if (SNEP_INSTANCE)
		SNEP_INSTANCE->Stop();
	SNEP_INSTANCE = std::shared_ptr<SnepServer>(new SnepServer(env, thisObj));
	SNEP_INSTANCE->Run();
}

void SnepServer::StopInstance() {
	if (SNEP_INSTANCE)
		SNEP_INSTANCE->Stop();
	SNEP_INSTANCE.reset();
}

void SnepServer::StopMacLink(int sig) {
  (void) sig;
  if (macLink && macLink->device)
  		nfc_abort_command(macLink->device);
}

/*
 * Explicit shutdown order
 */
SnepServer::~SnepServer() {
	running_.exchange(false);
	macLink.reset();
	llcLink.reset();
	device.reset();
	llcpInstance.reset();
	context.reset();
	jenv->DeleteGlobalRef(thisObj);
}

std::string SnepServer::SHexDump(gsl::span<uint8_t> buf) {
	std::vector<char> output(buf.size()*4, 0);
  size_t res = 0;
  for (size_t s = 0; s < buf.size(); s++) {
	sprintf(output.data() + res, "%02x  ", *(buf.data() + s));
	res += 4;
  }
  return std::string(output.begin(), output.end());
}

void* SnepServer::SnepThread(void* arg) {
	struct llc_connection* connection = static_cast<struct llc_connection*>(arg);
	auto weakSelf = static_cast<std::weak_ptr<SnepServer>*>(connection->user_data);
	if (!weakSelf)
		return nullptr;
	auto lockedSelf = weakSelf->lock();
	if (!lockedSelf)
		return nullptr;
	lockedSelf->SnepThreadT(connection);
	return nullptr;
}

void SnepServer::SnepThreadT(struct llc_connection* connection) {
	try {
		SnepThreadT(connection);
	} catch (...) {
		ProcessErrorCallback(std::current_exception());
	}
	Stop();
}

void SnepServer::SnepThreadSafe(struct llc_connection* connection) {
	std::vector<uint8_t> buffer(1024, 0), frame(1024, 0);

	while (running_.load()) {
		auto bytesReceived = llc_connection_recv(connection, buffer.data(),
				buffer.size(), nullptr);
		if (bytesReceived < 2) { // Error if < 0, SNEP's header (2 bytes)  and NDEF entry header (5 bytes)
			THROW("Error if < 0, SNEP's header (2 bytes)  and NDEF entry header (5 bytes)");
		}

		size_t n = 0;

		// Header
		auto versionMajor = (buffer[0] >> 4);
		auto versionMinor = (buffer[0] & 0x0F);
		switch (static_cast<SnepCommandType>(buffer[1])) {
		case SnepCommandType::CONTINUE:
			break;
		case SnepCommandType::GET:
			break;
		case SnepCommandType::PUT: {
				auto ndefLength = be32toh(*((uint32_t * )(buffer.data() + 2))); // NDEF length
				if ((bytesReceived - 6) < ndefLength)
					THROW("Less received bytes than expected ?");

				/** return snep success response package */
				frame[0] = 0x10; /** SNEP version */
				frame[1] = 0x81;
				frame[2] = 0;
				frame[3] = 0;
				frame[4] = 0;
				frame[5] = 0;
				llc_connection_send(connection, frame.data(), 6);

				auto dataThatArrived = gsl::make_span<uint8_t>(buffer.data() + 6,
						buffer.data() + bytesReceived);
				ProcessDataArrived(dataThatArrived);
			}
			break;
		}
	}
	llc_connection_stop(connection);
}

void SnepServer::Stop() {
  if (!running_.exchange(false)) return; //Already running

  macLink.reset();
  llcLink.reset();
  llcpInstance.reset();
  context.reset();
}

void SnepServer::Run() {
  if (running_.exchange(true)) return; //Already running
  {
	  nfc_context* contextTmp;
	  nfc_init(&contextTmp);
	  context = decltype(context)(contextTmp, nfc_exit);
  }

  if (llcp_init() < 0)
	THROW("llcp_init()");

  device = std::unique_ptr<nfc_device, decltype(nfc_close)*>(nfc_open(context.get(), NULL), nfc_close);
  if (!device)
	THROW("Cannot connect to NFC device");

  llcLink = std::unique_ptr<struct llc_link, decltype(llc_link_free)*>(llc_link_new(), llc_link_free);
  if (!llcLink)
	THROW("Cannot allocate LLC link data structures");

  struct llc_service *com_android_snep = llc_service_new_with_uri(NULL, SnepThread, "urn:nfc:sn:snep", new std::weak_ptr<SnepServer>(shared_from_this()));
  if (!com_android_snep)
	THROW("Cannot create com.android.snep service");

  llc_service_set_miu(com_android_snep, 512);
  llc_service_set_rw (com_android_snep, 2);

  if (llc_link_service_bind(llcLink.get(), com_android_snep, LLCP_SNEP_SAP) < 0)
	THROW("Cannot bind service");

  macLink = decltype(macLink)(mac_link_new(device.get(), llcLink.get()), MacLinkClose);
  if (!macLink)
	THROW("Cannot create MAC link");

  if (mac_link_activate_as_target(macLink.get()) < 0) {
	THROW("Cannot activate MAC link");
  }
}

void SnepServer::ProcessDataArrived(gsl::span<uint8_t> bytesRead) {
	try {
		auto clazz = jenv->FindClass("uk/co/polarmoment/bacta/libnfc_jni/jni/SnepServer"); // class path
		if (!clazz) THROW("Class not found");
		auto mid   = jenv->GetMethodID(clazz, "dataArrived", "([B)V");// function name
		if (!mid)   THROW("dataArrived not found");

		jbyteArray retArray = jenv->NewByteArray(static_cast<jsize>(bytesRead.size()));
		if(jenv->GetArrayLength(retArray) != bytesRead.size()) {
			jenv->DeleteLocalRef(retArray);
			THROW("Couldn't allocate java array");
		}

		void *temp = jenv->GetPrimitiveArrayCritical(retArray, 0);
		memcpy(temp, bytesRead.data(), bytesRead.size());
		jenv->ReleasePrimitiveArrayCritical(retArray, temp, static_cast<jint>(0));
		jenv->CallVoidMethod(thisObj, mid, retArray);
	} catch (...) {
		ProcessErrorCallback(std::current_exception());
	}
}

void SnepServer::ProcessErrorCallback(std::exception_ptr err) {
	std::string errStr = GetErrorStr(err);

	try {
		auto clazz = jenv->FindClass  ("uk/co/polarmoment/bacta/libnfc_jni/jni/SnepServer"); // class path
		if (!clazz) THROW("Class not found");
		auto mid   = jenv->GetMethodID(clazz, "processError", "(Ljava/lang/String;)");// function name
		if (!mid) THROW("processError not found");

		auto javaError = jenv->NewStringUTF(errStr.c_str());
		jenv->CallVoidMethod(thisObj, mid, javaError);
	} catch (...) {
		std::cerr << " Error handling " << errStr << " " << GetErrorStr(std::current_exception()) << std::endl;
	}
}
}

/*
 * Class:     uk_co_polarmoment_bacta_libnfc_jni_jni_SnepServer
 * Method:    runSnepServer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_uk_co_polarmoment_bacta_libnfc_1jni_jni_SnepServer_runSnepServer(JNIEnv *env , jobject thisObj) {
	SnepServer::StartInstance(env, thisObj);
}

JNIEXPORT void JNICALL Java_uk_co_polarmoment_bacta_libnfc_1jni_jni_SnepServer_finishSnepServer
  (JNIEnv *, jobject) {
	SnepServer::StopInstance();
}
