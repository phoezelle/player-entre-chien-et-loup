#include <stddef.h>
#include <avr/pgmspace.h>

#include <SPI.h>
#include <EEPROM.h>

#include "printf.h"

#include "nRF24L01.h"
#include "RF24.h"


#include <SdFat.h>
#include <SdFatUtil.h> 

#include <ThePlayer.h>

//#include <MemoryFree.h>

#include <avr/sleep.h>
#include <avr/wdt.h>

// watchdog interrupt
ISR (WDT_vect) 
{
   wdt_disable();  // disable watchdog
}  // end of WDT_vect

const byte theInitID=0;

ThePlayer myPlayer;




void setup() {
  Serial.begin(115200);
  delay(20);
  Serial.println("ON");
  printf_begin();
  printf_P(PSTR("printf ok\n\r"));
  delay(100);
  myPlayer.init(theInitID); //init the player
}

void loop(){

  myPlayer.routine();
  //delay(500);
  //printf(".");

}
