// Anbindung einer WS2812 LED-Leiste an das lokale Netzwerk
// incl. OTA, MQTT und Last Mode

// ---------- Einzubindende Librarys -------------------
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
// -----------------------------------------------------

// ----- Wifi ------------------------------------
const char ssid[] = "SSID";
const char pass[] = "KEY";
WiFiClient net;
// -----------------------------------------------


// ------------- MQTT Definitionen ---------------------------------------------- 
const char* SubscribeTo = "/ESP32/DG/Gaeste/tele"; //Sendekanal Nachrichten
const char* SubscribeFrom = "/ESP32/DG/Gaeste/color"; //Empfangskanal Nachrichten 
const char* mqtt_server = "192.168.178.69"; //MQTT Server
PubSubClient pubClient(net); //Client Initialisieren
// ------------------------------------------------------------------------------


// --- Nachrichten initialisieren -----
long lastMsg = 0;
char msg[50];
String message("#000000"); 
String lastMessage("#000000");
String color("");
// ------------------------------------

// ---- Allgemeine Deklarationen --------------
unsigned long lastMillis = 100000;


#define EEPROM_SIZE 64
int r = 30;
int g = 30;
int b = 30;


// ---- Deklarationen LED-leiste --------------
#define PIN            22       // Steuerungspin/Datenpin der LED-Leiste
#define NUMPIXELS      184      // Anzahl der angeschlossenen LED (Zähl-Start mit 0) 184
int brightness = 120;           // Definition Helligkeit der Leiste (Siehe NEOPIXEL Dokumentation)
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);  // Initialisierung des pixels Objekt


// -------------- Callback Routine beim Empfangen einer MQTT Nachricht ---------------
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  
  for (int i = 0; i < length; i++) {
    //Serial.print(length);
    Serial.println((char)payload[i]);
    
    if(i > 0){
      color = color + String((char)payload[i]);  
    }  
  }
  
  //Serial.println();
  //Serial.print("Länge Color: ");
  //Serial.println(sizeof(color));

  // ----- Wenn Payload einen Farbwert enthält, diesen auf die Leiste schreiben ---------

  if((char)payload[0] == '#'){
    // setting color
     setNeoColor(color);
  }
     
  //}
  
  
  // ------------------------------------------------------------------------------------
}
// --------------------------------------------------------------------------------------  


// --------- Routine Farben auf LED schreiben ---------------------
void setNeoColor(String value){
    Serial.println("Setting Neopixel..." + value);
    message = value;
    int number = (int) strtol( &value[0], NULL, 16);	// Konvertierung Farbwert String nach LongInteger
    
    g = number >> 16;        						// Bitshift rechts
    r = number >> 8 & 0xFF;
    b = number & 0xFF;

    // ------ Farbe im Rom ablegen --------

    // ------------------------------------

    // ------ Debug Print auf der Konsole ------
    Serial.print("RGB: ");
    Serial.print(r, DEC);
    Serial.print(" ");
    Serial.print(g, DEC);
    Serial.print(" ");
    Serial.print(b, DEC);
    Serial.println(" ");
    // -----------------------------------------

    // ------- Schleife zum Schreiben der Farben -----------------
   for(int i=0; i < NUMPIXELS; i++) {					// Ausgabe auf die LEDs
      pixels.setPixelColor(i, pixels.Color( g, r, b ) );
    }
    pixels.show();										// Aktivieren der neuen Farben
   // ------------------------------------------------------------
    Serial.println("on.");
    color = "";
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, pass);

  // ------ Start MQTT -----------------------------------
  Serial.print("Connecting to MQTT...");
  pubClient.setServer(mqtt_server, 1883);
  pubClient.setCallback(callback);
  Serial.println("done!");
  // ----------------------------------------------------- 
  
  // ------------- Initialiserung Update OverTheAir ---------
    
   ArduinoOTA.setPort(3232); 				// Port defaults to 3232
   ArduinoOTA.setHostname("DG-LED-Port"); 	// Hostname
   ArduinoOTA.setPassword("admin"); 		// Authentication

   ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
  // -----------------------------------------------------------------------------------------

  // --------------- Letzte verwendete Farbe aus EEPROM laden ------------

  // ---------------------------------------------------------------------
 
  // ----------- Initialisierung LED-Leiste ----------------- 
  pixels.setBrightness(brightness);
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.show(); 
  delay(50);
  
  //Serial.println("Länge: " + sizeof(color));
   for(int i=0; i < NUMPIXELS; i++) {          // Ausgabe auf die LEDs
      pixels.setPixelColor(i, pixels.Color( g, r, b ) );
    }
    pixels.show();                    // Aktivieren der neuen Farben
    
  // --------------------------------------------------------
  
  }

void loop() {

   ArduinoOTA.handle();
  
   delay(10);  // <- fixes some issues with WiFi stability

// -----------Prüfung, ob MQTT Verbindung steht -------------------
  if (!pubClient.connected()) {
    delay(100);
    reconnect();
  }
  pubClient.loop();
// -------------------------------------------------------------------



}
// -----------------------------------------------------------------------------


void reconnect() { 										// Loop until we're reconnected
  while (!pubClient.connected()) {
    if (pubClient.connect("net")) {						// Attempt to connect
		  Serial.println("connected");    
		  pubClient.publish(SubscribeTo, "Restart");	// Once connected, publish an announcement...
		  pubClient.subscribe(SubscribeFrom);	      	// ... and resubscribe
    } else {
		  Serial.print("failed, rc=");
		  Serial.print(pubClient.state());
		  Serial.println(" try again in 5 seconds");    
		  delay(5000);									// Wait 5 seconds before retrying
    }
  }
}
