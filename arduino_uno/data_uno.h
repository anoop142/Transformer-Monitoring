#ifndef _DATA_H
#define _DATA_H

#define BAUDRATE 115200
#define I2CADRR 8



#define TEMP_PIN A0
#define CURRENT_PIN A1
#define ECHO_PIN 9 // attach pin D2 Arduino to pin Echo of HC-SR04
#define TRIG_PIN 8 //attach pin D3 Arduino to pin Trig of HC-SR04


#define TEMP_THRESHOLD 40
#define OIL_THRESHOLD_PERCENT 40 // in percentage
#define OIL_FULL_LEVEL_DISTANCE 3 // in cm
#define CURRENT_THRESHOLD 0.25 // in Ampere
#define TEMP_TRIGGER_TIME 20000 


#endif
