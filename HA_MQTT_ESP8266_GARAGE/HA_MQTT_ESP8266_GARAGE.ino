#include <PubSubClient.h> 
#include <Adafruit_Si7021.h> 
#include <ESP8266WiFi.h> 
#include "DHT.h"
// START CONFIGURATION 
// Replace these with your WiFi credentials 
const char* ssid     = "SSID"; 
const char* password = "PASS"; 
// Replace these with your MQTT connection info 
const char* mqtt_server = "XX.XX.XX.XX"; 
 const char* mqtt_user = "user"; 
 const char* mqtt_password = "password"; 
// Define which MQTT topics the data is published to 
#define DHTPIN D4     // what digital pin the DHT22 is conected to
#define DHTTYPE DHT22   // there are multiple kinds of DHT sensors
#define humidity_topic "sensor/garage/humidity" 
#define temperature_topic "sensor/garage/temperature" 
#define garage_status_topic "sensor/garage/door-status" 
#define garage_command_topic "sensor/garage/door-command" 
#define motion_topic "sensor/garage/motion"
// Pins 
#define pin_led 16 
#define pin_door_status D5 //5 
#define pin_door_relay 4
#define pin_motion 3  //3 
long lastMsg = 0;   
long lastRecu = 0;
bool debug = false;  //Display log message if True

#define DHTPIN D4    // DHT Pin 
// END CONFIGURATION 
// DHT dht(DHTPIN, DHTTYPE);
DHT dht(DHTPIN, DHTTYPE, 22);
WiFiClient espClient; 
PubSubClient client(espClient); 
Adafruit_Si7021 sensor; 

void setup() { 
 // Configure the pins 
 digitalWrite(pin_led, HIGH); 
 digitalWrite(pin_door_relay, HIGH); 
 pinMode(pin_led, OUTPUT); 
 pinMode(pin_door_relay, OUTPUT); 
 pinMode(pin_door_status, INPUT_PULLUP); 
 pinMode(pin_motion, INPUT); 
 Serial.begin(115200); 
 dht.begin();
 delay(100); 

 // We start by connecting to a WiFi network 
 Serial.println(); 
 Serial.println(); 
 Serial.print("Connecting to "); 
 Serial.println(ssid); 
 WiFi.begin(ssid, password); 
 
 // Blink the LED until we're connected 
 int i = 0; 
 while (WiFi.status() != WL_CONNECTED) { 
   digitalWrite(pin_led, i % 2); 
   delay(500); 
   Serial.print("."); 
   i++; 
 } 
 digitalWrite(pin_led, HIGH); 
 Serial.println(""); 
 Serial.println("WiFi connected");   
 Serial.println("IP address: "); 
 Serial.println(WiFi.localIP()); 
 Serial.println(""); 
 
 // Connect to MQTT 
 client.setServer(mqtt_server, 1883); 
 client.setCallback(onMessageReceived); 
 
 // Set up the climate sensor 
 sensor = Adafruit_Si7021(); 
 if (sensor.begin()) { 
   Serial.println('Climate sensor ready'); 
 } else { 
   // TODO: We should probably reset the device if this happens 
   Serial.println('SENSOR FAILED TO START'); 
 } 
 delay(50); 
 digitalWrite(pin_led, HIGH); 
} 

// Attempts to reconnect to MQTT if we're ever disconnected 
void reconnect() { 
 // Loop until we're reconnected 
 while (!client.connected()) { 
   digitalWrite(pin_led, LOW); 
   Serial.print("Attempting MQTT connection..."); 
   // Attempt to connect 
   if (client.connect("GarageMultiSensor", mqtt_user, mqtt_password)) { 
     Serial.println("connected"); 
     client.subscribe(garage_command_topic); 
     digitalWrite(pin_led, HIGH); 
     delay(100); 
   } else { 
     Serial.print("failed, rc="); 
     Serial.print(client.state()); 
     Serial.println(" try again in 5 seconds"); 
     // Wait 5 seconds before retrying 
     delay(5000); 
   } 
 } 
} 

// Helps check whether any value change is significant enough to warrant a data push 
bool checkBound(float newValue, float prevValue, float maxDiff) { 
 return !isnan(newValue) && 
        (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff); 
} 
// long lastMsg = 0; 
float temp = 0.0; 
float hum = 0.0; 
float diff = 0.1; 
bool motion = false; 
bool doorOpen = false; 
void loop() { 

  // Ensure we stay connected 
 if (!client.connected()) { 
   reconnect(); 
 } 
 
 // Let MQTT do its thing 
 client.loop(); 
 digitalWrite(pin_led, HIGH); 
 bool sent = false; 
 
 // Check the temp, humidity, and door state once per second 
 long now = millis(); 
 if (now - lastMsg > 1000) { 
   lastMsg = now; 
   // Read the values from the sensor 
   float newTemp = dht.readTemperature(); 
   float newHum = dht.readHumidity();
   bool newDoorOpen = digitalRead(pin_door_status); 
   // Check whether the temperature has changed 
   if (checkBound(newTemp, temp, diff)) { 
     temp = newTemp; 
     Serial.print("New temperature:"); 
     Serial.println(String(temp).c_str()); 
     client.publish(temperature_topic, String(temp).c_str(), true); 
     sent = true; 
   } 
   // Check whether the humidity has changed 
   if (checkBound(newHum, hum, diff)) { 
     hum = newHum; 
     Serial.print("New humidity:"); 
     Serial.println(String(hum).c_str()); 
     client.publish(humidity_topic, String(hum).c_str(), true); 
     sent = true; 
   } 
   if (doorOpen && !newDoorOpen) { 
     doorOpen = false; 
     Serial.println("Door closed"); 
     client.publish(garage_status_topic, "closed", true); 
     sent = true; 
   } else if (!doorOpen && newDoorOpen) { 
     doorOpen = true; 
     Serial.println("Door open"); 
     client.publish(motion_topic, "open", true); 
     sent = true; 
   } 
 } 
 // Constantly check for motion 
 bool newMotion = digitalRead(pin_motion); 
 if (motion && !newMotion) { 
   motion = false; 
   Serial.println("No more motion"); 
   client.publish(motion_topic, "OFF", true); 
   sent = true; 
 } else if (!motion && newMotion) { 
   motion = true; 
   Serial.println("Motion detected"); 
   client.publish(motion_topic, "ON", true); 
   sent = true; 
 } 


 
 // If any data was sent (due to a change) then blink the blue LED 
 if (sent) { 
   sent = false; 
   digitalWrite(pin_led, LOW); 
   delay(100); 
   digitalWrite(pin_led, HIGH); 
 } 
} 
void onMessageReceived(char* topic, byte* payload, unsigned int length) { 
 int i; 
 char message_buff[100]; 
 for(i=0; i<length; i++) { 
   message_buff[i] = payload[i]; 
 } 
 message_buff[i] = '\0'; 
 String msgString = String(message_buff); 
 Serial.println("Payload: " + msgString); 
 bool activateRelay = false; 
 if (!doorOpen && msgString.equals("OPEN")) { 
   activateRelay = true; 
 } else if (doorOpen && msgString.equals("CLOSE")) { 
   activateRelay = true; 
 } 
 if (activateRelay) { 
   Serial.println("Toggling door state"); 
   digitalWrite(pin_door_relay, LOW); 
   delay(300); 
   digitalWrite(pin_door_relay, HIGH); 
 } 
} 
