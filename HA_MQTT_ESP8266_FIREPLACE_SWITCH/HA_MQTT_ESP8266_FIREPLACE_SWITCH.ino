

/*
 * MQTT Fireplace Switch with temp and humidity sensor
 * ESP + DHT22 + Relay
*/
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"          // Librairie des capteurs DHT

#define wifi_ssid "SSID"
#define wifi_password "PASS"

#define mqtt_server "xx.xx.xx.xx"
#define mqtt_user "user"      // if exist
#define mqtt_password "password"  //idem

#define temperature_topic "dev_sensor/temperature"  //Topic temperature
#define humidity_topic "dev_sensor/humidity"        //Topic humidity

//Buffer to decode MQTT messages
char message_buff[100];

long lastMsg = 0;   
long lastRecu = 0;
bool debug = false;  //Display log message if True

#define DHTPIN D4    // DHT Pin 

// Un-comment you sensor
//#define DHTTYPE DHT11       // DHT 11 
#define DHTTYPE DHT22         // DHT 22  (AM2302)

// Create abjects
DHT dht(DHTPIN, DHTTYPE);     
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(9600);     
  pinMode(D0,OUTPUT);     //Pin 2 for LED
  setup_wifi();           //Connect to Wifi network
  client.setServer(mqtt_server, 1883);    // Configure MQTT connexion
  client.setCallback(callback);           // callback function to execute when a MQTT message   
  dht.begin();
}

//Connexion au réseau WiFi
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi OK ");
  Serial.print("=> ESP8266 IP address: ");
  Serial.print(WiFi.localIP());
}

//Reconnexion
void reconnect() {

  while (!client.connected()) {
    Serial.print("Connecting to MQTT broker ...");
    if (client.connect("ESP8266Client", mqtt_user, mqtt_password)) {
      Serial.println("OK");
    } else {
      Serial.print("KO, error : ");
      Serial.print(client.state());
      Serial.println(" Wait 5 secondes before to retry");
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  long now = millis();
  // Send a message every minute
  if (now - lastMsg > 1000 * 60) {
    lastMsg = now;
    // Read humidity
    float h = dht.readHumidity();
    // Read temperature in Celcius
    float t = dht.readTemperature();

    // Oh, nothing to send
    if ( isnan(t) || isnan(h)) {
      Serial.println("KO, Please chez DHT sensor !");
      return;
    }
  
    if ( debug ) {
      Serial.print("Temperature : ");
      Serial.print(t);
      Serial.print(" | Humidity : ");
      Serial.println(h);
    }  
    client.publish(temperature_topic, String(t).c_str(), true);   // Publish temperature on temperature_topic
    client.publish(humidity_topic, String(h).c_str(), true);      // and humidity
  }
  if (now - lastRecu > 100 ) {
    lastRecu = now;
    client.subscribe("dev_sensor/switch1");
  }
}

// MQTT callback function
// D'après http://m2mio.tumblr.com/post/30048662088/a-simple-example-arduino-mqtt-m2mio
void callback(char* topic, byte* payload, unsigned int length) {

  int i = 0;
  if ( debug ) {
    Serial.println("Message recu =>  topic: " + String(topic));
    Serial.print(" | longueur: " + String(length,DEC));
  }
  // create character buffer with ending null terminator (string)
  for(i=0; i<length; i++) {
    message_buff[i] = payload[i];
  }
  message_buff[i] = '\0';
  
  String msgString = String(message_buff);
  if ( debug ) {
    Serial.println("Payload: " + msgString);
  }
  
  if ( msgString == "ON" ) {
    digitalWrite(D0,HIGH);  
  } else {
    digitalWrite(D0,LOW);  
  }
}


