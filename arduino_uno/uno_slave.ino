
/* Arduino UNO SLAVE 
  Copyright  Anoop S
*/

#include <Wire.h>
#include "data_uno.h"


static struct structdata{
  float temp = 0;
  float oil = 0;
  float current = 0;

  bool temp_alert = 0;
  bool oil_alert = 0;
  bool current_alert = 0;
   
}sensorData;

long previousMillis = 0;



static void initSerial(void){
 Serial.begin(BAUDRATE);  
 Serial.println("UNO - Slave started");
  
}

static void initPins(void){
   /* Initialize pins */
 pinMode(TEMP_PIN,INPUT);
 
 pinMode(ECHO_PIN, INPUT); 
 pinMode(TRIG_PIN, OUTPUT); 
  
}

static void callHome(String sensor){
  Serial.print("Alerting..");
  Serial.println(sensor);
  Serial.println(sensorData.oil);
  
}

static void getTemperature(void){
  float temp=0.0, sample=0.0;
  for(int x=0; x <100; x++){
    temp = analogRead(TEMP_PIN) * 0.48828125;
    sample += temp;
    delay(3);
  }
  sensorData.temp = sample / 100; /* ((5/1024) * 1000) / 10 */
  
}

static float getVPP(void)
{
  float result;
  int readValue;             
  int maxValue = 0;         
  int minValue = 1023;         
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(CURRENT_PIN);
       if (readValue > maxValue) 
       {
           maxValue = readValue;
       }
       if (readValue < minValue) 
           /*record the minimum sensor value*/
           minValue = readValue;
       }
   }
   
   result = ((maxValue - minValue) * 5.0)/1023.0;
      
   return result;
 }

static void getCurrent(void){
  int mVperAmp = 66;
  const float error_correction  = 0.16;
  double Voltage = 0;
  double VRMS = 0;
  double current_rms = 0;

 Voltage = getVPP();
 VRMS = (Voltage/2.0) *0.707;  
 current_rms = (VRMS * 1000)/mVperAmp - error_correction; // to make current zero on no load
 sensorData.current = abs(current_rms); // get absolute value of current
}

static void getOilLevel(void){
  long duration;
  float distance;
  float percent;
  
    // Clears the TRIG_PIN condition
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  
  // Sets the TRIG_PIN HIGH (ACTIVE) for 10 microseconds
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Reads the ECHO_PIN, returns the sound wave travel time in microseconds
  duration = pulseIn(ECHO_PIN, HIGH);
  // Calculating the distance
  distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
 
  // Hack to fix too close object measured as 1000+ cm
  if( distance < 1000){
    percent = ( OIL_FULL_LEVEL_DISTANCE / distance) * 100;  // in percentage
    // Another Hack
    if(percent <= 100){ // to prevent percentage more than 100.
      sensorData.oil = percent;
    }
  }
}



static void printSensorData(void){
 Serial.println(sensorData.temp);
 Serial.println(sensorData.current);
 Serial.print(sensorData.oil);
 Serial.println();
 Serial.println(sensorData.temp_alert);
 Serial.println(sensorData.current_alert);
 Serial.println(sensorData.oil_alert);
 
}

// Get sensor values
static void getAllSensors(void){
  getTemperature();
  getCurrent();
  getOilLevel();
}



static void validateOilLevel(void){
  if(sensorData.oil_alert == 0 && sensorData.oil <= OIL_THRESHOLD_PERCENT){
    sensorData.oil_alert = 1;
    callHome("OIL");
  }
  /* reset alert status if working as normal */
  else if(sensorData.oil >= OIL_THRESHOLD_PERCENT){
     sensorData.oil_alert = 0;
    
  }
}

static void validateTemp(void){
  if(sensorData.temp_alert == 0 && sensorData.temp >0 && sensorData.temp >= TEMP_THRESHOLD && sensorData.temp <= 150 ){
    // current time
    unsigned long currentMillis = millis();
     if(currentMillis - previousMillis  > TEMP_TRIGGER_TIME) { 
      previousMillis = currentMillis;
      sensorData.temp_alert = 1;
      callHome("Temperature");
     }
  }
  /* reset alert status if working as normal */
  else if(sensorData.temp >0 && sensorData.temp < TEMP_THRESHOLD  ){
    sensorData.temp_alert = 0;
    
  }
}

static void validateCurrent(void){
  if(sensorData.current_alert == 0 && sensorData.current >= CURRENT_THRESHOLD){
    sensorData.current_alert = 1;
    callHome("Current");
  }
  /* reset alert status if working as normal */
   else if(sensorData.current <= 0.21 ){
    sensorData.current_alert = 0;
    
  }
}

// Validate sensor values
static void validateAllSesnors(void){
  validateOilLevel();
  validateTemp();
  validateCurrent();
}

void receiveEvent(void){
  /* dummy function , slave shouldn't recieve from master for now */
  Serial.println("Recieve event");
}


// function that executes whenever data is requested from master
void requestEvent() {
  Wire.write((byte *) &sensorData,sizeof(sensorData));
}

static void wireSetup(void){
	Wire.begin(I2CADRR);                /* join i2c bus with address  */
	Wire.onReceive(receiveEvent); /* register receive event */
	Wire.onRequest(requestEvent); /* register request event */
}


void setup() {
  
  initSerial();
  
  initPins();
  
  getAllSensors();   
  
  wireSetup();
 
}

void loop() {
  
  getAllSensors();
  
  validateAllSesnors();

 printSensorData();
  
  delay(100);
}
