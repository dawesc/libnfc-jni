package uk.co.polarmoment.bacta.libnfc_jni.jni;

import uk.co.polarmoment.bacta.libnfc_jni.LibraryLoader;

public class SnepServer {
	public native void runSnepServer();

	static {
		try {
			LibraryLoader.loadLibrary("libnfc-jni");
		} catch (Exception e) {
			System.err.println(e);
			e.printStackTrace();
			System.exit(1);
		}
	}
}