//Required HTTPClientESP32Ex library to be installed  https://github.com/mobizt/HTTPClientESP32Ex

#include <WiFi.h>
#include "FirebaseESP32.h"
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#define FIREBASE_HOST "my-first-firebase-projec-d7eac.firebaseio.com" //Change to your Firebase RTDB project ID e.g. Your_Project_ID.firebaseio.com
#define FIREBASE_AUTH "ZYhtRPsJTEnCGjRPxBmhJ3gvhtJm5QQbFlqjNpFy" //Change to your Firebase RTDB secret password
#define WIFI_SSID "Tamagna_2.4G"
#define WIFI_PASSWORD "30033003"
#define DHTTYPE DHT11

const int ledPin = 21;
const int sensorPin = 4;
const String dbName = "devices";
const int deviceId = 1;
float currentTemp = 0.0;
float currentHumid = 0.0;

//Define FirebaseESP32 data object
DHT_Unified dht(sensorPin, DHTTYPE);
FirebaseData firebaseData;
FirebaseJson json;
 
void setup()
{ 
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
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
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
 
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Firebase.reconnectWiFi(true);
 
  //Set database read timeout to 1 minute (max 15 minutes)
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  //tiny, small, medium, large and unlimited.
  //Size and its write timeout e.g. tiny (1s), small (10s), medium (30s) and large (60s).
  Firebase.setwriteSizeLimit(firebaseData, "tiny");
  Serial.println("Connected...");
}
 
void loop()
{
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  if (isnan(event.temperature)) {
    Serial.println(F("Error reading temperature!"));
  }
  else {
    Serial.print(F("Temperature: "));
    currentTemp = event.temperature;
    currentTemp = (roundf(currentTemp*10.))/10.;
    Serial.print(currentTemp);
    Serial.println(F("°C"));
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);
  }
  // Get humidity event and print its value.
  dht.humidity().getEvent(&event);
  if (isnan(event.relative_humidity)) {
    Serial.println(F("Error reading humidity!"));
  }
  else {
    Serial.print(F("Humidity: "));
    currentHumid = event.relative_humidity;
    currentHumid = (roundf(currentHumid*10.))/10.;
    Serial.print(currentHumid);
    Serial.println(F("%"));
    digitalWrite(ledPin, HIGH);
    delay(200);
    digitalWrite(ledPin, LOW);
    delay(200);
  }

  String strTemp = String(currentTemp);
  strTemp = strTemp.substring(0, strTemp.length()-1);
  String strHumid = String(currentHumid);
  strHumid = strHumid.substring(0, strHumid.length()-1);
  
  json.set("/temp", strTemp);
  Firebase.updateNode(firebaseData, "/"+dbName+"/"+String(deviceId), json);
  json.set("/humid", String(strHumid));
  Firebase.updateNode(firebaseData, "/"+dbName+"/"+String(deviceId), json);
  digitalWrite(ledPin, HIGH);
  delay(200);
  digitalWrite(ledPin, LOW);
  delay(10000);
}
