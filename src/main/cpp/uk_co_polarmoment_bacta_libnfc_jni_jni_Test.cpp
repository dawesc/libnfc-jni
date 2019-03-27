#include <jni.h>
#include <stdio.h>
#include <uk_co_polarmoment_bacta_libnfc_jni_jni_Test.h>

JNIEXPORT void JNICALL Java_uk_co_polarmoment_bacta_libnfc_1jni_jni_Test_hello(JNIEnv *, jobject) {
	printf("Hello World\n");
#ifdef __cplusplus
	printf("__cplusplus is defined\n");
#else
	printf("__cplusplus is NOT defined\n");
#endif
	return;
}
