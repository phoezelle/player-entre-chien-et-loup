#include <stddef.h>
#include <avr/pgmspace.h>

#include <SPI.h>
//Add EEPROM librarie
#include <EEPROM.h>

#include "printf.h"

#include <SoftwareSerial.h>
#include "Bounce.h"
#include <TheRemote.h>




#include "nRF24L01.h"
#include "RF24.h"

//#include <Menu.h>

//#include <LCD.h>

#define TELECO_ID 251



TheRemote myRemote;

void setup() {                
  Serial.begin(9600);
  delay(20);
  Serial.println("STARTUP COCO");
    printf_begin();
  printf_P(PSTR("printf ok\n\r"));
  delay(100);
  myRemote.init(TELECO_ID);
  attachInterrupt(0,doEncoderA, FALLING);

}

void loop() {
  myRemote.routine();    
}

void doEncoderA(){
  myRemote.doEncoderA();
}
