package uk.co.polarmoment.bacta.libnfc_jni;

import java.io.*;

/**
 * Contains helper methods for loading native libraries, particularly JNI.
 *
 * @author gkubisa
 */
public class LibraryLoader {

	/**
	 * Loads a native shared library. It tries the standard System.loadLibrary
	 * method first and if it fails, it looks for the library in the current
	 * class path. It will handle libraries packed within jar files, too.
	 *
	 * @param name name of the library to load
	 * @throws IOException if the library cannot be extracted from a jar file
	 * into a temporary file
	 */
	public static void loadLibrary(String name) throws IOException {
		try {
			System.loadLibrary(name);
			System.out.println("It's ok using local");
		} catch (UnsatisfiedLinkError e) {
			String filename = System.mapLibraryName(name);
			System.out.println("Using res file: " + filename);
			InputStream in = LibraryLoader.class.getClassLoader().getResourceAsStream(filename);
			if (in == null)
				throw new RuntimeException("Cannot load library " + name);
			
			int pos = filename.lastIndexOf('.');
			File file = File.createTempFile(filename.substring(0, pos), filename.substring(pos));
			file.deleteOnExit();
			try {
				byte[] buf = new byte[4096];
				OutputStream out = new FileOutputStream(file);
				try {
					while (in.available() > 0) {
						int len = in.read(buf);
						if (len >= 0) {
							out.write(buf, 0, len);
						}
					}
				} finally {
					try { out.close(); } catch (Throwable t) {}
				}
			} finally {
				try { in.close(); } catch (Throwable t) {}
			}
			System.load(file.getAbsolutePath());
		}
	}
}
