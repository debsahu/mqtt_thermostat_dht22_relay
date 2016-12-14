#include <TimeLib.h>
#include <NtpClientLib.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
//#include <ArduinoOTA.h>
#include <DHT.h>
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"`
#include "OLEDDisplayUi.h" // Include the UI lib
#include "images.h" // Include custom images

SSD1306  display(0x3c, D2, D1);
OLEDDisplayUi ui     ( &display );

#define DHTTYPE DHT22
#define DHTPIN  D3

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE);

#define WIFI_SSID "WiFi SSID"
#define WIFI_PASSWORD "WiFi Password"
#define MQTT_SERVER_IP "MQTT Server IP"
//boolean otaflag=false;

float bedroomtemp = -999.0;
float bedroomhum = -999.0;
float comproomtemp = -999.0;
float comproomhum = -999.0;
float bedroom3temp = -999.0;
float bedroom3hum = -999.0;

float lastPubTemp = 0.0;
float lastPubHum = 0.0;
float lastPubttemp = 0.0;

float settemp=21.1;
float tempat=settemp;
boolean manualmode=false;
boolean setbtemp=false;

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float hic=0.0;
float diff = 0.1;

extern "C" {
#include "user_interface.h"
}

float avgTemp(){
  float nt=4;
  float sumtemp=0;
  
  if(temp>0){
    sumtemp=sumtemp+temp;
  }else{
    nt=nt-1;
  }
  if(bedroomtemp>0){
    sumtemp=sumtemp+bedroomtemp;
  }else{
    nt=nt-1;
  }
  if(bedroom3temp>0){
    sumtemp=sumtemp+bedroom3temp;
  }else{
    nt=nt-1;
  }
  if(comproomtemp>0){
    sumtemp=sumtemp+comproomtemp;
  }else{
    nt=nt-1;
  }

  if (nt>0) {
    return sumtemp/nt;
  }else{
    return 0.0;
  }
}

float avgHum(){
  float nh=4;
  float sumhum=0;
    
  if(hum>0){
    sumhum=sumhum+hum;
  }else{
    nh=nh-1;
  }
  if(bedroomhum>0){
    sumhum=sumhum+bedroomhum;
  }else{
    nh=nh-1;
  }
  if(bedroom3hum>0){
    sumhum=sumhum+bedroom3hum;
  }else{
    nh=nh-1;
  }
  if(comproomhum>0){
    sumhum=sumhum+comproomhum;
  }else{
    nh=nh-1;
  }

  if (nh>0) {
    return sumhum/nh;
  }else{
    return 0.0;
  }
}

String IPAddress2String(IPAddress address)
{
 return "IP:" + 
        String(address[0]) + "." + 
        String(address[1]) + "." + 
        String(address[2]) + "." + 
        String(address[3]);
}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, String(millis())); yield();
}

void drawFrame0(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {

  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 0 + y, "THERMOSTAT"); yield();
  display->drawString(64 + x, 17 + y, NTP.getTimeStr()); yield();
  display->drawString(64 + x, 34 + y, NTP.getDateStr()); yield();

}

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  String ipAddress = IPAddress2String(WiFi.localIP());
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 0 + y, ipAddress); yield();
  if(avgTemp()>0){
    display->drawString(64 + x, 17 + y, "aTemp: " + String(avgTemp()) + " C"); yield();
  }else{
    display->drawString(64 + x, 17 + y, "aTemp: NO DATA"); yield();
  }
  if(avgHum()>0){
    display->drawString(64 + x, 34 + y, "aHum : " + String(avgHum()) + " %"); yield();
  }else{
    display->drawString(64 + x, 34 + y, "aHum : NO DATA"); yield();
  }
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  if(manualmode){
  display->drawString(64 + x, 0 + y, "Manual Mode"); yield();
  }else{
    display->drawString(64 + x, 0 + y, "MQTT Controlled"); yield();
  }
  display->drawString(64 + x, 17 + y, "SetTemp: " + String(settemp) + " C"); yield();
  if(setbtemp){
    tempat=bedroomtemp;
    display->drawString(64 + x, 34 + y, "B: Y Diff : " + String(settemp-tempat) + " C"); yield();
  }else{
    tempat=avgTemp();
    display->drawString(64 + x, 34 + y, "B: N Diff : " + String(settemp-tempat) + " C"); yield();
  }
//  if(manualmode){
//  display->drawString(64 + x, 0 + y, "Manual Mode: Y");
//  display->drawString(64 + x, 17 + y, "SetTemp: " + String(settemp) + " C");
//    if(setbtemp){
//      display->drawString(64 + x, 34 + y, "B: Y Diff : " + String(settemp-tempat) + " C");
//    }else{
//      display->drawString(64 + x, 34 + y, "B: N Diff : " + String(settemp-tempat) + " C");
//    }
//  }else{
//    display->drawString(64 + x, 0 + y, "Manual Mode: N");
//    display->drawString(64 + x, 17 + y, "MQTT Controlled");
//    display->drawString(64 + x, 34 + y, "Home Assistant");
//  }
}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 0 + y, "Thermometer"); yield();
  
  if(bedroomtemp>0){
    display->drawString(64 + x, 17 + y, "Temp: " + String(temp) + " C"); yield();
  }else{
    display->drawString(64 + x, 17 + y, "Temp: NO DATA"); yield();
  }
  if(bedroomhum>0){
    display->drawString(64 + x, 34 + y, "Hum : " + String(hum) + " %"); yield();
  }else{
    display->drawString(64 + x, 34 + y, "Hum : NO DATA"); yield();
  }
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 0 + y, "Bed Room"); yield();
  
  if(bedroomtemp>0){
    display->drawString(64 + x, 17 + y, "Temp: " + String(bedroomtemp) + " C"); yield();
  }else{
    display->drawString(64 + x, 17 + y, "Temp: NO DATA"); yield();
  }
  if(bedroomhum>0){
    display->drawString(64 + x, 34 + y, "Hum : " + String(bedroomhum) + " %"); yield();
  }else{
    display->drawString(64 + x, 34 + y, "Hum : NO DATA"); yield();
  }
}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 0 + y, "Comp Room"); yield();

  if(comproomtemp>0){
    display->drawString(64 + x, 17 + y, "Temp: " + String(comproomtemp) + " C"); yield();
  }else{
    display->drawString(64 + x, 17 + y, "Temp: NO DATA"); yield();
  }
  if(comproomhum>0){
    display->drawString(64 + x, 34 + y, "Hum : " + String(comproomhum) + " %"); yield();
  }else{
    display->drawString(64 + x, 34 + y, "Hum : NO DATA"); yield();
  }
}

void drawFrame6(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_16);
  display->drawString(64 + x, 0 + y, "Bed Room 3"); yield();

  if(bedroom3temp>0){
    display->drawString(64 + x, 17 + y, "Temp: " + String(bedroom3temp) + " C"); yield();
  }else{
    display->drawString(64 + x, 17 + y, "Temp: NO DATA"); yield();
  }
  if(bedroom3hum>0){
    display->drawString(64 + x, 34 + y, "Hum : " + String(bedroom3hum) + " %"); yield();
  }else{
    display->drawString(64 + x, 34 + y, "Hum : NO DATA"); yield();
  }
}

// This array keeps function pointers to all frames
// frames are the single views that slide in
FrameCallback frames[] = { drawFrame0, drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5, drawFrame6 };

// how many frames are there?
int frameCount = 7;

// Overlays are statically drawn on top of a frame eg. a clock
OverlayCallback overlays[] = { msOverlay };
int overlaysCount = 1;

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0'; // Null terminator used to terminate the char array
  String message = (char*)payload;
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println(message);

  if (!manualmode){
    if ((strcmp(topic,"home/thermostat/1/input")==0)){
      if (message == "1" or message == "on") {
        Serial.println("Turning on Thermostat Socket 1");
        client.publish("home/thermostat/1/output", "1");
        digitalWrite(D5, LOW);
      } else if (message == "0" or message == "off") {
        Serial.println("Turning off Thermostat Socket 1");
        client.publish("home/thermostat/1/output", "0");
        digitalWrite(D5, HIGH);
      }
    }
    
    if ((strcmp(topic,"home/thermostat/2/input")==0)){
      if (message == "1" or message == "on") {
        Serial.println("Turning on Thermostat Socket 2");
        client.publish("home/thermostat/2/output", "1");
        digitalWrite(D6, LOW);
      } else if (message == "0" or message == "off") {
        Serial.println("Turning off Thermostat Socket 2");
        client.publish("home/thermostat/2/output", "0");
        digitalWrite(D6, HIGH);
      }
    }
  }
  
//  if(strcmp(topic,"home/thermostat/ota")==0 and message == "1") {
//    otaflag=true;
//  }
//  else if(strcmp(topic,"home/thermostat/ota")==0 and message == "0") {
//    otaflag=false;
//  }
  if(strcmp(topic,"sensor1/temperature")==0) {
    comproomtemp = atof(message.c_str());
  }
  else if(strcmp(topic,"sensor1/humidity")==0) {
    comproomhum = atof(message.c_str());
  }
  else if(strcmp(topic,"sensor2/temperature")==0) {
    bedroom3temp = atof(message.c_str());
  }
  else if(strcmp(topic,"sensor2/humidity")==0) {
    bedroom3hum = atof(message.c_str());
  }
  else if(strcmp(topic,"home/temperature")==0) {
    bedroomtemp = atof(message.c_str());
  }
  else if(strcmp(topic,"home/humidity")==0) {
    bedroomhum = atof(message.c_str());
  }
  else if(strcmp(topic,"home/thermostat/settemp")==0) {
    settemp = atof(message.c_str());
    client.publish("home/thermostat/statustemp", String(settemp).c_str(), true);
  }
  else if(strcmp(topic,"home/thermostat/mode")==0){
    if(message == "1") {
    manualmode=true;
    settemp = 21.1;
    client.publish("home/thermostat/settemp", String(settemp).c_str(), true);
    } else if(message == "0") {
    manualmode=false;
    digitalWrite(D5, HIGH);
    //settemp = 21.1;
    //client.publish("home/thermostat/settemp", String(settemp).c_str(), true);
    }
  }
  else if(strcmp(topic,"home/thermostat/setbtemp")==0){
    if(message == "1") {
      setbtemp=true;
    }else if(message == "0") {
      setbtemp=false;
    }
  }
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) &&
         (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(1500);

  NTP.begin("pool.ntp.org", 1, true);
  NTP.setInterval(63);
  NTP.setTimeZone(-5);
  yield();
}

void setup() {
//  system_update_cpu_freq(80);
  system_update_cpu_freq(160);

  Serial.begin(115200);
  Serial.println();
  Serial.println();

	// The ESP is capable of rendering 60fps in 80Mhz mode
	// but that won't give you much time for anything else
	// run it in 160Mhz mode or just set it to 30 fps
  ui.setTargetFPS(30);

	// Customize the active and inactive symbol
  ui.setActiveSymbol(activeSymbol);
  ui.setInactiveSymbol(inactiveSymbol);

  // You can change this to
  // TOP, LEFT, BOTTOM, RIGHT
  ui.setIndicatorPosition(BOTTOM);

  // Defines where the first frame is located in the bar.
  ui.setIndicatorDirection(LEFT_RIGHT);

  // You can change the transition that is used
  // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
  ui.setFrameAnimation(SLIDE_LEFT);

  // Add frames
  ui.setFrames(frames, frameCount);

  // Add overlays
  //ui.setOverlays(overlays, overlaysCount);

  // Initialising the UI will init the display too.
  ui.init();

  display.flipScreenVertically();
  ////////// End of Display //////////////
  
  setup_wifi();

  NTP.onNTPSyncEvent([](NTPSyncEvent_t ntpEvent) {
    if (ntpEvent) {
      Serial.print("Time Sync error: ");
      if (ntpEvent == noResponse)
        Serial.println("NTP server not reachable");
      else if (ntpEvent == invalidAddress)
        Serial.println("Invalid NTP server address");
    }
    else {
      Serial.print("Got NTP time: ");
      Serial.println(NTP.getTimeDateString(NTP.getLastNTPSync()));
    }
  });
  yield();
  
  client.setServer(MQTT_SERVER_IP, 1883);
  client.setCallback(callback);
  pinMode(DHTPIN, INPUT_PULLUP);
  dht.begin();
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  digitalWrite(D5, HIGH);
  digitalWrite(D6, HIGH);

//  ////OTA////
//  ArduinoOTA.setHostname("Thermostat");
//  ArduinoOTA.onStart([]() {
//    //digitalWrite(BUILTIN_LED, LOW);
//    Serial.println("Start");
//  });
//  ArduinoOTA.onEnd([]() {
//    //digitalWrite(BUILTIN_LED, HIGH);
//    Serial.println("\nEnd");
//  });
//  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
//    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
//  });
//  ArduinoOTA.onError([](ota_error_t error) {
//    Serial.printf("Error[%u]: ", error);
//    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
//    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
//    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
//    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
//    else if (error == OTA_END_ERROR) Serial.println("End Failed");
//  });
//  ArduinoOTA.begin();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("Thermostat")) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      // client.publish("outTopic", "hello world");
      // ... and resubscribe
      //client.subscribe("sensor1/#");
      //client.subscribe("sensor2/#");
      client.subscribe("sensor2/temperature");
      client.subscribe("sensor2/humidity");
      client.subscribe("sensor1/temperature");
      client.subscribe("sensor1/humidity");
      client.subscribe("home/temperature");
      client.subscribe("home/humidity");
      client.subscribe("home/thermostat/#");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {

//  if (otaflag){
//    display.resetDisplay();
//    display.clear();
//    Serial.println("OTA MODE");
//    ArduinoOTA.handle();
//    delay(200);
//  } else {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  int remainingTimeBudget = ui.update(); yield();

  if (remainingTimeBudget > 0) {
    // You can do some work here
    // Don't do stuff if you are below your
    // time budget.
    // delay(remainingTimeBudget);
    
    if(manualmode){
        if(setbtemp){
          tempat=bedroomtemp;
        }else{
          tempat=avgTemp();
        }
        
        if ((settemp-tempat)>-2){
    //    if(tempat<settemp){
          digitalWrite(D5, LOW); yield();
        }else{
          digitalWrite(D5, HIGH); yield();
        }
      }
      
      long now = millis();
      //  Serial.println(now - lastMsg);
      if (now - lastMsg > 10000) {
        lastMsg = now;
    
        float tAvgTemp=avgTemp();
        if (lastPubTemp != tAvgTemp) {
          client.publish("home/thermostat/averagetemp", String(tAvgTemp).c_str(), true); yield();
          lastPubTemp = tAvgTemp;
        }
    
        float tAvgHum=avgHum();
        if (lastPubHum != tAvgHum) {
          client.publish("home/thermostat/averagehum", String(tAvgHum).c_str(), true); yield();
          lastPubHum = tAvgHum;
        }
    
        float ttempat=tempat;
        if (lastPubttemp != ttempat){
          client.publish("home/thermostat/targettemp", String(ttempat).c_str(), true); yield();
          lastPubttemp=ttempat;
        }
        
        float newTemp = dht.readTemperature();
        yield();
        float newHum = dht.readHumidity();
        yield();
        float newHic = dht.computeHeatIndex(newTemp, newHum, false);
        yield();
    
        if (checkBound(newTemp, temp, diff)) {
          temp = newTemp;
          //Serial.print("New temperature:");
          //Serial.println(String(temp).c_str());
          client.publish("home/thermostat/temperature", String(temp).c_str(), true); yield();
        }
        Serial.print(String(newTemp).c_str());
        Serial.print(" ");
        if (checkBound(newHum, hum, diff)) {
          hum = newHum;
          //Serial.print("New humidity:");
          //Serial.println(String(hum).c_str());
          client.publish("home/thermostat/humidity", String(hum).c_str(), true); yield();
        }
        Serial.print(String(newHum).c_str());
        Serial.print(" ");
        if (checkBound(newHic, hic, diff)) {
          hic = newHic;
          //Serial.print("New Heat Index:");
          //Serial.println(String(hic).c_str());
          client.publish("home/thermostat/heatindex", String(hic).c_str(), true); yield();
        }
        Serial.println(String(newHic).c_str());
      }
    }
//  }
}

