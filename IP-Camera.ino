/**
 * main.ino
 * Programma di Massimo Giordano
 * Scheda utilizzata: Arduino Uno
 */

/**
 * Gestione da remoto di 4 telecamere IP con arduino che rilevano il movimento e inviano segnali ad arduino
 */

#include <Adafruit_VC0706.h> // libreria da Adafruit. Per installarla, scaricarla da GitHub
#include <SD.h>
#include <SPI.h>
#include <Ethernet.h>

#include <SoftwareSerial.h>  
/**
 * Ho bisogno di pin RX e TX. Su arduino Uno possiamo usare i pin 2 e 3, quindi:
 * -> RX = 2
 * -> TX = 3
 */
#if ARDUINO >= 100 // controllo sulla versione di arduino, se è >= di 1.00
	SoftwareSerial cameraconnection = SoftwareSerial(2, 3);
#else
	NweSoftwareSerial cameraconnection = NewSoftwareSerial(2, 3);
#endif

/**
 * Dichiarazione di tutti gli oggetti delle telecamere IP
 */
Adafruit_VC0706 camera1 = Adafruit_VC0706(&cameraconnection);
Adafruit_VC0706 camera2 = Adafruit_VC0706(&cameraconnection);
Adafruit_VC0706 camera3 = Adafruit_VC0706(&cameraconnection);
Adafruit_VC0706 camera4 = Adafruit_VC0706(&cameraconnection);

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x45, 0x99}; // indirizzo MAC
byte ip[] = {0, 0, 0, 0}; // indirizzo IP scheda di rete. motificabile
byte dns[] = { 0, 0, 0, 0}; // DNS dell'ISP
EthernetServer server(80); // creazione di un server alla porta 80

void setup()
{
	Ethernet.begin(mac, ip); // inizializzazione dispositivo di Ethernet
	server.begin();			 // inizio lettura

}

void loop()
{
	
}