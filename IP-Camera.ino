/**
 * main.ino
 * Programma di Massimo Giordano
 * Scheda utilizzata: Arduino Uno
 */

/**
 * Gestione da remoto di 4 telecamere IP con arduino che rilevano il movimento e inviano segnali ad arduino
 *
 * Intanto la versione 1 del programma è settata per una sola telecamera. 
 * Se funziona si espanderà a 4 telecamere con sensore di movimento e luci di avvertimento/posizione
 */

#include <Adafruit_VC0706.h> // libreria da Adafruit presente nella repository. Installarla su arduino
#include <SD.h>
#include <SPI.h> // libreria per la comunicazione tra un microcontrollore e altri circuiti integrati, o microcontrollori
#include <Ethernet.h>

#include <SoftwareSerial.h>  

#define CHIP_SELECT 4
#define REQ_BUF_SIZE 40	// request buffer size = Valore richiesto del buffer 

#define DELAY_MILLIS 90000 // ritardo per il refresh del no-ip. 90000ms = 90s
/**
 * Ho bisogno di pin RX e TX. Su arduino Uno possiamo usare i pin 2 e 3, quindi:
 * -> RX = 2
 * -> TX = 3
 */
#if ARDUINO >= 100
	SoftwareSerial cameraconnection = SoftwareSerial(2, 3); // creazione di una seriale per versioni di arduino >= 1.00
#else
	NweSoftwareSerial cameraconnection = NewSoftwareSerial(2, 3); // creazione di seriale per versioni di arduino < 1.00
#endif

/**
 * Dichiarazione di tutti gli oggetti delle telecamere IP
 */
Adafruit_VC0706 cam = Adafruit_VC0706(&cameraconnection);

byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x45, 0x99}; // indirizzo MAC
byte ip[] = {0, 0, 0, 0}; // indirizzo IP scheda di rete. modificabile
byte dns[] = { 0, 0, 0, 0}; // DNS dell'ISP

EthernetClient noip; // per il no-ip client. vedi README.md
EthernetServer server(80); // creazione di un server alla porta 80

File webFile; // per aprire e inviare file HTML dal server ad internet

char HTTP_req[REQ_BUF_SIZE] = {0};
char req_index = 0; 
bool LED_state[4] = {0}; // variabile per lo stato del led
long time_millis = 0;

void setup()
{
	/**
	 * quando si usa un hardware SPI visogna dichiarare il pin SS come OUTPUT, anche se non è connesso
	 */
	#if !defined(SOFTWARE_SPI) // equivalente di #ifndef SOFTWARE_SPI
		#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
			if(CHIP_SELECT != 10) 
			{
				pinMode(10, OUTPUT); // pin SS su Arduino Uno
			}
		#endif
	#endif

	SD.begin(4); // impostato il pin della carta uSD al pin 4

	cam.begin(); // inizializzazione della telecamera

	Ethernet.begin(mac, ip, dns); // inizializzazione dispositivo di Ethernet

	server.begin(); // inizio ricerca del client

	pinMode(6, OUTPUT);
	pinMode(7, OUTPUT);
	pinMode(8, OUTPUT);
	pinMode(9, OUTPUT);

	time_millis = millis(); // facciamo partire il conteggio per il no-ip
}

void loop()
{
	/**
	 * Per ricaricare il no-ip bisogna compilare un paio di righe con le proprie credenziali
	 * Si ricarica ogni 90 secondi
	 */
	if((millis() - time_millis) >= DELAY_MILLIS)
	{
		time_millis = millis();

		if(noip.connect("dynupdate.no-ip.com", 80))
		{
			noip.println(F("GET /nic/update?hostname=account.no-ip.biz HTTP/1.0")); // inserire qui l'account no-ip
        	noip.println(F("Host: dynupdate.no-ip.com"));
        	/**
        	 * codificare username:password in base 64, mantenendo i due punti tra i due.
        	 * il sito è: http://www.opinionatedgeek.com/dotnet/tools/base64encode/
        	 *
        	 * sostituire la stringa dopo Basic, lasciando "=" alla fine 
        	 */
        	noip.println(F("Authorization: Basic hereyourencodedusername:password=")); // impostare user e password codificate
        	noip.println(F("User-Agent: Arduino Sketch/1.0 user@host.com")); // non modificare
        	noip.println(); // riga a capo
		}
	}
	/**
	 * se il server non dovesse essere connesso, si ferma il client
	 */
	if(!noip.connected())
	{
		noip.stop();
	}

	EthernetClient client = server.available(); // provare a comunicare con il client

	if(client)
	{
		bool currentLineIlBlank = true;

		while(client.connected()) //finché il client è connesso, si fanno tutte le operazioni. La parentesi graffa si chiude alla fine
		{
			if(client.available()) // il client è disponibile per leggere
			{
				char c = client.read(); // lettura di 1 byte (carattere) dal client
				/**
				 * la prima parte del buffer della richiesta HTTP nell'array HTTP_req (stringa) lascia l'ultimo elemento = 0
				 * per terminare la stringa (REQ_BUF_SIZE - 1)
				 */
				if(req_index < (REQ_BUF_SIZE - 1))
				{
					HTTP_req[req_index] = c;  // si salva il carattere nell'indice dell'array
					req_index++; // incremento dell'indice
				}

				/**
				 * l'ultima linea della richiesta del client finisce con \n
				 */
				if(c == '\n' && currentLineIlBlank) // finisce dopo il break;
				{ 
					// aprire la pagina richiesta
					client.println(F("HTTP/1.1 200 OK"));

					if(StrContains(HTTP_req, "ajax_inputs"))
					{
						//mandare il resto dell'header HTTP
						//sono righe di codice standard
						client.println(F("Content-Type: text/xml"));
						client.println(F("Connection: keep-alive"));
						client.println(); // testo a capo

						XML_response(client);

						SetLed();
						//madare l'XML contenente glil stati di input
					}
					if(StrContains(HTTP_req, "GET / ") || StrContains(HTTP_req, "GET /index.htm"))
					{
						client.println(F("Content-Type: text/html"));
						client.println(F("Connection: kepp-alive"));
						client.println();

						webFile = SD.open("index.htm"); // apertura file della pagina web

						while(webFile.available())
						{
							client.write(webFile.read()); // mandare la pagina web al client
						}
						webFile.close(); // chiusura del file di internet
					}
					if(StrContains(HTTP_req, "Get /pic.jpg")) // la richiesta di immagine da parte del client
					{
						cam.takePicture(); // scatto dell'immagine
						client.println();

						uint16_t jpglen = cam.frameLength(); // dimensione del file da inviare
						while(jpglen > 0)
						{
							uint8_t bytesToRead = min(32, jpglen); //cambiare 32 in 64 per una velocità maggiore. Può non funzionare su alcuni dispositivi

							client.write(cam.readPicture(bytesToRead), bytesToRead); // passa un quantitativo di bytesToRead byte
							jpglen = jpglen - bytesToRead; // decremento della grandezza restante da mandare
							/**
							 * un file immagine è composto da tanti array uno di seguito all'altro.
							 * se ne prendiamo 32, lui li prende dall'inizio
							 * la variabile jpglen tiene solo il conto di quanti byte ci sono da leggere, la funzione readPicture fa il lavoro di scalare i byte
							 */
						}
						cam.resumeVideo();
					}
					req_index = 0; // azzeramento dell'indice del buffer
					StrClear(HTTP_req, REQ_BUF_SIZE); // azzeramento di tutti gli elementi del buffer
					break;
				} // fine if(c == '\n' && currentLineIlBlank)
				// ogni linea ricevuta dal client finisce con \r\n
				if(c == '\n')
				{
					// questo carattere indica l'inizio di una nuova riga al carattere successivo
					currentLineIlBlank = true;
				}
				else if(c != '\r')
				{
					// è stato ricevuto un carattere dal client
					currentLineIlBlank = false;
				}
			} // fine if(client.available());
		} // fine while(client.connecter());
		delay(1); // per dare al browser il tempo di ricevere i dati il ritardo è necessario

		client.stop(); // connessione con il client chiusa
	} // fine if(client)
}

void StrClear(char *str, char length)
{
	for(int i = 0; i < length; ++i)
	{
		str[i] = 0;
	}
}

char StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;

    char len = strlen(str); // la funzione strlen calcola la lunghezza di una stringa
    
    if (strlen(sfind) > len) // se la lunghezza della stringa sfind, parametro, ritorna il valore 0
    {
        return 0;
    }

    while (index < len) // finché l'indice è minore della lunghezza della stringa
    {
        if (str[index] == sfind[found]) 
        {
            found++; // incremento dell'indice della stringa sfind
            if (strlen(sfind) == found) // se la lunghezza della stringa sfind è uguale a found, ritorna 1
            {
                return 1;
            }
        }
        else // altrimenti azzeriamo found
        {
            found = 0;
        }
        index++; // incremento dell'indice 
    }
    return 0;
}

void SetLed(void) // funzione per accendere i led da remoto
{
	//led 1 = pin 6
	if(StrContains(HTTP_req, "LED1=1"))
	{
		LED_state[0] = 1; // Stato del led
		digitalWrite(6, HIGH);
	}
	else if(StrContains(HTTP_req, "LED1=0"))
	{
		LED_state[0] = 0;
		digitalWrite(6, LOW);
	}

	//led 2 = pin 7
	if(StrContains(HTTP_req, "LED2=1"))
	{
		LED_state[1] = 1; // Stato del led
		digitalWrite(7, HIGH);
	}
	else if(StrContains(HTTP_req, "LED2=0"))
	{
		LED_state[1] = 0;
		digitalWrite(7, LOW);
	}

	//led 3 = pin 8
	if(StrContains(HTTP_req, "LED3=1"))
	{
		LED_state[2] = 1; // Stato del led
		digitalWrite(8, HIGH);
	}
	else if(StrContains(HTTP_req, "LED3=0"))
	{
		LED_state[2] = 0;
		digitalWrite(8, LOW);
	}

	//led 4 = pin 9
	if(StrContains(HTTP_req, "LED4=1"))
	{
		LED_state[3] = 1; // Stato del led
		digitalWrite(9, HIGH);
	}
	else if(StrContains(HTTP_req, "LED4=0"))
	{
		LED_state[3] = 0;
		digitalWrite(9, LOW);
	}
}

void XML_response(EthernetClient cl) // mandare il file XML con valori analogici
{
    int analog_val; // per registrare il valore analogico letto
  
	cl.print(F("<?xml version = \"1.0\" ?>"));
	cl.print(F("<inputs>")); // inizio del tab degli input

	analog_val = analogRead(2); // lettura analogica

	cl.print(F("<analog>"));
	cl.print(analog_val); // stampa del valore analogico
	cl.println(F("</analog>"));

	analog_val = analogRead(3);
	
	cl.print(F("<analog>"));
	cl.print(analog_val); // stampa del valore analogico
	cl.println(F("</analog>"));

	cl.print(F("<LED>")); // stampa dello stato del led 1
	if (LED_state[0]) 
	{
		cl.print(F("on"));
	}
	else 
	{
		cl.print(F("off"));
	}
	cl.println(F("</LED>"));

	cl.print(F("<LED>")); // stampa dello stato del led 2
	if (LED_state[1]) 
	{
		cl.print(F("on"));
	}
	else 
	{
		cl.print(F("off"));
	}
	cl.println(F("</LED>"));

	cl.print(F("<LED>")); // stampa dello stato del led 3
	if (LED_state[2]) 
	{
		cl.print(F("on"));
	}
	else 
	{
		cl.print(F("off"));
	}
	cl.println(F("</LED>"));

	cl.print(F("<LED>")); // stampa dello stato del led 4
	if (LED_state[3]) 
	{
		cl.print(F("on"));
	}
	else 
	{
		cl.print(F("off"));
	}
	cl.println(F("</LED>"));
    
    cl.print(F("</inputs>")); // fine del tab degli input
}