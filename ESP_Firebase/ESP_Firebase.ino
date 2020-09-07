//Required HTTPClientESP32Ex library to be installed  https://github.com/mobizt/HTTPClientESP32Ex

#include <WiFi.h>
#include "FirebaseESP32.h"
#include <Adafruit_Sensor.h>

#include <PubSubClient.h>

#include <DHT.h>
#include <DHT_U.h>

#define DHTTYPE DHT11

#define WIFI_SSID "Tamagna_2.4G"
#define WIFI_PASSWORD "30033003"

#define FIREBASE_HOST "my-first-firebase-projec-d7eac.firebaseio.com" //Change to your Firebase RTDB project ID e.g. Your_Project_ID.firebaseio.com
#define FIREBASE_AUTH "ZYhtRPsJTEnCGjRPxBmhJ3gvhtJm5QQbFlqjNpFy" //Change to your Firebase RTDB secret password

#define TOKEN "BBFF-EMbMYuTvx68o3ZIxXkfdhLSDY9PMOt" // Put your Ubidots' TOKEN
#define MQTT_CLIENT_NAME "ESP32_01" // MQTT client Name, please enter your own 8-12 alphanumeric character ASCII string; 
#define VARIABLE_LABEL1 "temperature" // Assing the variable label
#define VARIABLE_LABEL2 "humidity" // Assing the variable label
#define DEVICE_LABEL "esp32" // Assig the device label

// Pin configurations
const int sensorPin = 4;
const int ledTemp = 18;
const int ledHumid = 19;
const int ledWifi = 21;
const int ledFirebase = 22;
const int ledUdibots = 23;

const String dbName = "devices";
const int deviceId = 1;
const int keepLedOn = 2000; // LEDs will be kept on for 2 seconds
const int wifiReconnectDelay = 2 * 1000; // Retry reconneting Wifi after 2 seconds
const int maxWifiReconnectBeforeRestart = 10; // ESP32 will be restarted if it fails to connect Wifi after 10 attempts
const int sendInterval = 2 * 60 * 1000; // Read sensor every 2 minutes, if reading has changed then send
const int unchangedCountThreshold = 10; // Send data forcibly after 10 interations, even if there are no changes in sensor data
const int udibotReconnectDelay = 2 * 1000; // Retry reconneting Uditbot MQTT after 2 seconds
const int maxUdibotReconnect = 10;

int loopCounter = 0;
float lastTemp = 200.0;
float lastHumid = 200.0;
unsigned long time_now = 0;

char mqttBroker[]  = "industrial.api.ubidots.com";
char payload[100];
char topic[150];
// Space to store values to send
char str_sensor[10];

//Define FirebaseESP32 data object
DHT_Unified dht(sensorPin, DHTTYPE);
FirebaseData firebaseData;
FirebaseJson json;

WiFiClient ubidots;
PubSubClient client(ubidots);

void callback(char* topic, byte* payload, unsigned int length) {
  char p[length + 1];
  memcpy(p, payload, length);
  p[length] = NULL;
  String message(p);
  Serial.write(payload, length);
  Serial.println(topic);
}

// Reconnect to Udibots
void reconnect() {
  int count = 0;
  // Loop until we're reconnected
  while (!client.connected() && count < maxUdibotReconnect) {
    Serial.println("Attempting MQTT connection...");
    
    // Attemp to connect
    if (client.connect(MQTT_CLIENT_NAME, TOKEN, "")) {
      Serial.println("Connected");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      // Wait 2 seconds before retrying
      count++;
      delay(udibotReconnectDelay);
    }
  }
  if(count >= maxUdibotReconnect)
    Serial.println("Failed to connect, skipping.");
}

// Connect to Wifi
boolean connectWifi(){
  int wifiRetryCount = 0;
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED && wifiRetryCount < maxWifiReconnectBeforeRestart )
  {
    delay(wifiReconnectDelay);
    Serial.print(".");
    wifiRetryCount ++;
  }
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("Resetting");
    ESP.restart();
  }
  else
  {
    Serial.println("");
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    return true;
  }
  return false;
}

// Send data to Udibots
void sendToUdibots(float temp, float humid) {
  if (!client.connected()) {
    reconnect();
  }

  if (client.connected()) {
    sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
    sprintf(payload, "%s", ""); // Cleans the payload
    sprintf(payload, "{\"%s\":", VARIABLE_LABEL1); // Adds the variable label
    float sensor = temp;
    dtostrf(sensor, 4, 2, str_sensor);
    sprintf(payload, "%s {\"value\": %s}}", payload, str_sensor); // Adds the value
    Serial.println("Publishing data to Ubidots Cloud");
    client.publish(topic, payload);
    client.loop();
    sprintf(topic, "%s%s", "/v1.6/devices/", DEVICE_LABEL);
    sprintf(payload, "%s", ""); // Cleans the payload
    sprintf(payload, "{\"%s\":", VARIABLE_LABEL2); // Adds the variable label
    sensor = humid;
    dtostrf(sensor, 4, 2, str_sensor);
    sprintf(payload, "%s {\"value\": %s}}", payload, str_sensor); // Adds the value
    Serial.println("Publishing data to Ubidots Cloud");
    client.publish(topic, payload);
    client.loop();
    Serial.println("Sent to Udibots!");
    digitalWrite(ledUdibots, HIGH);
  }
}

// Send data to Firebase
void sendToFirebase(float temp, float humid) {
  float roundedTemp = (roundf(temp*10.))/10.;
  String strTemp = String(roundedTemp);
  strTemp = strTemp.substring(0, strTemp.length()-1);
  
  float roundedHumid = (roundf(humid*10.))/10.;
  String strHumid = String(roundedHumid);
  strHumid = strHumid.substring(0, strHumid.length()-1);
  
  json.set("/temp", strTemp);
  Firebase.updateNode(firebaseData, "/"+dbName+"/"+String(deviceId), json);
  json.set("/humid", String(strHumid));
  Firebase.updateNode(firebaseData, "/"+dbName+"/"+String(deviceId), json);
  Firebase.setTimestamp(firebaseData, "/"+dbName+"/"+String(deviceId)+"/updatedAt");
  Serial.println("Sent to Firebase!");
  digitalWrite(ledFirebase, HIGH);
}

void blinkLed(int ledPin, int duration, int count){
  for (int i=0; i<count; i++){
    digitalWrite(ledPin, HIGH);
    delay(duration);
    digitalWrite(ledPin, LOW);
    delay(duration);
  }
}
 
void setup()
{ 
  Serial.begin(115200);
  pinMode(ledTemp, OUTPUT);
  pinMode(ledHumid, OUTPUT);
  pinMode(ledWifi, OUTPUT);
  pinMode(ledFirebase, OUTPUT);
  pinMode(ledUdibots, OUTPUT);
  dht.begin();
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println(F("------------------------------------"));
  Serial.println(F("Temperature Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("°C"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("°C"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("°C"));
  Serial.println(F("------------------------------------"));
  // Print humidity sensor details.
  dht.humidity().getSensor(&sensor);
  Serial.println(F("Humidity Sensor"));
  Serial.print  (F("Sensor Type: ")); Serial.println(sensor.name);
  Serial.print  (F("Driver Ver:  ")); Serial.println(sensor.version);
  Serial.print  (F("Unique ID:   ")); Serial.println(sensor.sensor_id);
  Serial.print  (F("Max Value:   ")); Serial.print(sensor.max_value); Serial.println(F("%"));
  Serial.print  (F("Min Value:   ")); Serial.print(sensor.min_value); Serial.println(F("%"));
  Serial.print  (F("Resolution:  ")); Serial.print(sensor.resolution); Serial.println(F("%"));
  Serial.println(F("------------------------------------"));
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  boolean wifiStatus = connectWifi();
 
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
 
  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
  client.setServer(mqttBroker, 1883);
  client.setCallback(callback);  
  Serial.println("Connected...");
  delay(5000);
}
 
void loop()
{
  float temp;
  float humid;
  boolean tempWorking = false;
  boolean humidWorking = false;
  sensors_event_t event;

  if ((time_now == 0) || (millis() > time_now + sendInterval)) {
    time_now = millis();

    blinkLed(ledTemp, 200, 3);
    dht.temperature().getEvent(&event);
    if (isnan(event.temperature)) {
      Serial.println(F("Error reading temperature!"));
    }
    else {
      Serial.print(F("Temperature: "));
      temp = event.temperature;
      Serial.print(temp);
      Serial.println(F("°C"));
      digitalWrite(ledTemp, HIGH);
      tempWorking = true;
    }

    blinkLed(ledHumid, 200, 3);
    dht.humidity().getEvent(&event);
    if (isnan(event.relative_humidity)) {
      Serial.println(F("Error reading humidity!"));
    }
    else {
      Serial.print(F("Humidity: "));
      humid = event.relative_humidity;
      Serial.print(humid);
      Serial.println(F("%"));
      digitalWrite(ledHumid, HIGH);
      humidWorking = true;
    }

    if ((temp != lastTemp && tempWorking == true) || (humid != lastHumid && humidWorking == true) || (loopCounter >= 5)) {
      blinkLed(ledWifi, 200, 3);
      boolean wifiStatus = connectWifi();    
      if (wifiStatus)
        digitalWrite(ledWifi, HIGH);
      blinkLed(ledFirebase, 200, 3);
      sendToFirebase(temp, humid);    
      blinkLed(ledUdibots, 200, 3);
      sendToUdibots(temp, humid);
      lastTemp = temp;
      lastHumid = humid;
      delay(keepLedOn);
      digitalWrite(ledTemp, LOW);
      digitalWrite(ledHumid, LOW);
      digitalWrite(ledWifi, LOW);
      digitalWrite(ledFirebase, LOW);
      digitalWrite(ledUdibots, LOW);
      loopCounter = 0;
    }
  }
  loopCounter++;
}
