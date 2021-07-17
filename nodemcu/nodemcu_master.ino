/* copyright Anoop S  

sendEmail()  got from somewhere else

*/

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <base64.h>
#include "data_mcu.h"




static struct structdata{
  float temp = 0;
  float oil = 0;
  float current = 0;

  bool temp_alert = 0;
  bool oil_alert = 0;
  bool current_alert = 0;
   
}sensorData;

// true if we have alerted oil once
static bool hasAlertedOil = 0;
// true if i2c is working
// time
long previousMillis = 0;

ESP8266WebServer server(80);
WiFiClientSecure client;

static void initSerial(void){
 Serial.begin(BAUDRATE); /* begin serial for debug */
 Serial.println("Master- NodeMCU started!");
}

byte sendEmail()
{
  client.setInsecure(); 
  Serial.println("Attempting to connect to GMAIL server");
  if (client.connect(_GMailServer, 465) == 1) {
    Serial.println(F("Connected"));
  } else {
    Serial.print(F("Connection failed:"));
    return 0;
  }
  if (!response())
    return 0;

  Serial.println(F("Sending Extended Hello"));
  client.println("EHLO gmail.com");
  if (!response())
    return 0;
  
  Serial.println(F("Sending auth login"));
  client.println("auth login");
  if (!response())
    return 0;

  Serial.println(F("Sending User"));
  // Change to your base64, ASCII encoded user
  client.println(base64::encode(DEVICE_EMAIL));
  if (!response())
    return 0;

  Serial.println(F("Sending Password"));
  // change to your base64, ASCII encoded password
  client.println(base64::encode(_mailPassword));
  if (!response())
    return 0;

  Serial.println(F("Sending From"));
  // your email address (sender) - MUST include angle brackets
  client.println(F("MAIL FROM: <",DEVICE_EMAIL,">"));
  if (!response())
    return 0;

  // change to recipient address - MUST include angle brackets
  Serial.println(F("Sending To"));
  client.println(F("RCPT To: <",EMAIL_To_ALERT,">"));
  // Repeat above line for EACH recipient
  if (!response())
    return 0;

  Serial.println(F("Sending DATA"));
  client.println(F("DATA"));
  if (!response())
    return 0;

  Serial.println(F("Sending email"));
  // recipient address (include option display name if you want)
  client.println(F("To: ", EMAIL_To_ALERT));

  // change to your address
  client.println(F("From: ",DEVICE_EMAIL));
  client.println(F("Subject: Alert\r\n"));
   if(sensorData.temp_alert == 1){
    client.println(F("Overheating "));
    client.print(String(sensorData.temp));
    client.print(F("C"));
    client.println(F("\n"));
   }
   if(sensorData.oil_alert == 1){
    client.println(F("Oil need refilling "));
    client.print(F(" Oil Level: "));
    client.print(String(sensorData.oil));
    client.print(F("%"));
    client.println(F("\n"));
    hasAlertedOil = 1;
   }
   if(sensorData.current_alert == 1){
    client.println(F("Overcurrent "));
    client.print(String(sensorData.current));
    client.print(F("A"));
    client.println(F("\n"));
   }
  // IMPORTANT you must send a complete line containing just a "." to end the conversation
  // So the PREVIOUS line to this one must be a prinln not just a print
  client.println(F("."));
  if (!response())
    return 0;

  Serial.println(F("Sending QUIT"));
  client.println(F("QUIT"));
  if (!response())
    return 0;

  client.stop();
  Serial.println(F("Disconnected"));
  return 1;
}

// Check response from SMTP server
byte response()
{
  // Wait for a response for up to X seconds
  int loopCount = 0;
  while (!client.available()) {
    delay(1);
    loopCount++;
    // if nothing received for 10 seconds, timeout
    if (loopCount > 10000) {
      client.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }

  // Take a snapshot of the response code
  byte respCode = client.peek();
  while (client.available())
  {
    Serial.write(client.read());
  }

  if (respCode >= '4')
  {
    Serial.print("Failed in eRcv with response: ");
    Serial.print(respCode);
    return 0;
  }
  return 1;
}


void handle_OnConnect() {
  
  server.send(200, "text/html", SendHTML()); 
}

String SendHTML(void){

  String temp_status = "";
  String oil_status = "";
  String current_status = "";

  if(sensorData.temp_alert != 0){
    temp_status = "<p><span style=\"color: #ff0000;\">Overheating</span></p>\n";
  }

   if(sensorData.oil_alert != 0){
    oil_status = "<p><span style=\"color: #ff0000;\">Need Refilling</span></p>\n";
  }

   if(sensorData.current_alert != 0){
    current_status = "<p><span style=\"color: #ff0000;\">Overload</span></p>\n";
  }
  
  
  String page = "<!DOCTYPE html> <html>\n";
  page +="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  page += "<meta http-equiv=\"refresh\" content=\"3\">";
  page +="<title>Transformer Live Report</title>\n";
  page +="<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}\n";
  page +="body{margin-top: 50px;} h1 {color: #444444;margin: 50px auto 30px;}\n";
  page +="p {font-size: 24px;color: #444444;margin-bottom: 10px;}\n";
  page +="</style>\n";
  page +="</head>\n";
  page +="<body>\n";
  page +="<div id=\"webpage\">\n";
  page +="<h1>Transformer Live Report</h1>\n";
  
  page +="<p>Temperature: ";
  page +=(float)sensorData.temp;
  page +="&#8451</p>";
  page += temp_status;
  page += "<p>&nbsp;</p>";
  
  page +="<p>Oil Level Percentage: ";
  page +=(float)sensorData.oil;
  page +="&#37</p>";
  page+= oil_status;
  page += "<p>&nbsp;</p>";
  
  page +="<p>Current: ";
  page +=(float)sensorData.current;
  page +="A</p>";
  page+= current_status;
  
  page +="</div>\n";
  page +="</body>\n";
  page +="</html>\n";
  
  return page;
}


void handle_NotFound(){
  server.send(404, "text/plain", "Not found");
}

static void wireSetup(void){

  Wire.begin(D1, D2); /* join i2c bus with SDA=D1 and SCL=D2 of NodeMCU */
  
}

static bool wireRecieveData(void){
   Wire.requestFrom(I2CADRR, sizeof(sensorData)); 

   bool i2c_status = 0;
 
  while(Wire.available() > 0)
  {
    i2c_status = 1;
    Wire.readBytes((byte *) &sensorData, sizeof(sensorData));
 }

 return i2c_status; 
}

static void wifiSetup(void){
  Serial.println("Connecting to ");
  Serial.println(ssid);

  //connect to your local wi-fi network
  WiFi.begin(ssid, password);

  //check wi-fi is connected to wi-fi network
  while (WiFi.status() != WL_CONNECTED) {
  delay(1000);
  Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected..!");
  Serial.print("Got IP: ");  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");

}

static void evaluateSensorData(void){
   unsigned long currentMillis = millis();
     if(currentMillis - previousMillis  > EMAIL_INTERVAL) { 
      previousMillis = currentMillis;
        if(sensorData.temp_alert ||  sensorData.current_alert || (sensorData.oil_alert && !hasAlertedOil)){
          Serial.println("SENDING EMAIL");
          sendEmail();
        }
     }
     // reset hasAlertedOIl if oil aert is false
     if(sensorData.oil_alert == 0){
      hasAlertedOil = 0;
     }
}



static void printSensorData(void){
 Serial.println();
 Serial.println("Sensor values");
 Serial.println("=============");
 Serial.printf("TEMP : %f\n",sensorData.temp);
 Serial.printf("CURRENT : %f\n",sensorData.current);
 Serial.printf("OIL : %f\n",sensorData.oil);
 Serial.println();
  Serial.println("Alert Status");
 Serial.println("=============");
 Serial.printf("TEMP ALERT STATUS : %d\n",sensorData.temp_alert);
 Serial.printf("CURRENT ALERT STATUS : %d\n",sensorData.current_alert);
 Serial.printf("OIL ALERT STATUS : %d\n",sensorData.oil_alert);
}

// returns true if arduino is found



/* Setup functions */

void setup() {

 initSerial();
  
 wireSetup();

 wifiSetup();

 evaluateSensorData();
 

}

/* Loop functions */

void loop() { 
  
  bool i2c_status = wireRecieveData();
 
  server.handleClient(); 
  // Check if i2c is available
  if(i2c_status){
    printSensorData();
    evaluateSensorData();
    delay(100);
  }
  else{
        Serial.println("Waiting dor Arduino..");
  }
}
