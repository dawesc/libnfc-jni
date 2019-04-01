package uk.co.polarmoment.bacta.libnfc_jni.jni;

import uk.co.polarmoment.bacta.libnfc_jni.LibraryLoader;

public class SnepServer {
	public native void runSnepServer();
	public native void finishSnepServer();
	
	private void dataArrived(byte[] data) {
		
	}
	private void processError(String error) {
		
	}
	static {
		try {
			LibraryLoader.loadLibrary("nfc-jni");
		} catch (Exception e) {
			System.err.println(e);
			e.printStackTrace();
			System.exit(1);
		}
	}
	
	public void close() {
		finishSnepServer();
	}
}