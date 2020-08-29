//Required HTTPClientESP32Ex library to be installed  https://github.com/mobizt/HTTPClientESP32Ex

#include <WiFi.h>
#include "FirebaseESP32.h"


#define FIREBASE_HOST "my-first-firebase-projec-d7eac.firebaseio.com" //Change to your Firebase RTDB project ID e.g. Your_Project_ID.firebaseio.com
#define FIREBASE_AUTH "ZYhtRPsJTEnCGjRPxBmhJ3gvhtJm5QQbFlqjNpFy" //Change to your Firebase RTDB secret password
#define WIFI_SSID "Tamagna_2.4G"
#define WIFI_PASSWORD "30033003"


//Define FirebaseESP32 data object
FirebaseData firebaseData;
FirebaseJson json;
int Vresistor = A0; 
int Vrdata = 0; 
float dummyData = 0.0;
 
void setup()
{
 
  Serial.begin(115200);
 
  //pinMode(Vresistor, INPUT);
 
 
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
 
  /*
  This option allows get and delete functions (PUT and DELETE HTTP requests) works for device connected behind the
  Firewall that allows only GET and POST requests.
  
  Firebase.enableClassicRequest(firebaseData, true);
  */
 
  //String path = "/data";
  
 
  Serial.println("------------------------------------");
  Serial.println("Connected...");
  
}
 
void loop()
{
  /*
  Vrdata = analogRead(Vresistor);
  int Sdata = map(Vrdata,0,4095,0,1000);
  Serial.println(Sdata); 
  delay(100); 
  json.set("/data", Sdata);
  Firebase.updateNode(firebaseData,"/Sensor",json);
  */
  int deviceId = 1;
  dummyData = dummyData + 0.1;
  Serial.println(dummyData);
  json.set("/temp", String(dummyData));
  Firebase.updateNode(firebaseData, "/devices/"+String(deviceId), json);
  /*
  int deviceId = 1;
  dummyData = dummyData + 0.1;
  Serial.println(dummyData); 
  Firebase.setString("/devices/"+String(deviceId)+"/temp", String(dummyData))
  Serial.println("Sent data"); 
  */
  delay(5000);
}
