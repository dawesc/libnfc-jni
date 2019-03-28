package uk.co.polarmoment.bacta.libnfc_jni;

import uk.co.polarmoment.bacta.libnfc_jni.jni.SnepServer;

/**
 * Hello world!
 *
 */
public class App {

	public static void main(String[] args) {
		System.out.println("Hello World!");
		new SnepServer().hello();
	}
}
