package uk.co.polarmoment.bacta.libnfc_jni;

import java.io.IOException;

import uk.co.polarmoment.bacta.libnfc_jni.jni.SnepServer;

/**
 * Hello world!
 *
 */
public class App {

	public static void main(String[] args) {
		SnepServer server = null;
		try {
			System.out.println("Hello World!");
			server = new SnepServer();
			server.runSnepServer();
			System.out.println("Press enter to end!");
			System.in.read();
			System.out.println("Done!");
		} catch (IOException e) {
			System.err.println(e.getMessage());
			e.printStackTrace();
		} finally {
			server.close();
		}
	}
}
