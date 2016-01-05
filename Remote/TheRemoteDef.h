#ifndef _TheRemoteDef_h
#define _TheRemoteDef_h

// *******************************************
// define PIN config


#define POT_LCD_LIGHT_ON A0

#define ROTARY_A 2
#define ROTARY_B 3
#define PB_OK 4
#define PB_UPDATE 17


//RF24 pin
#define REMOTE_RF24_CE 5
#define REMOTE_RF24_CS 6

#define REMOTE_RF24_SPARKFUN_CE 7
#define REMOTE_RF24_SPARKFUN_CS 8

#define LCD_TX 9

#define PIN595CSN 10

#define MASTER_ABATT 0 //analog pin for battery test
#define MASTER_AROLE1 1 //analog pin for type select switch (sation = off / base = on)
#define MASTER_AROLE2 2 //analog pin for verbose and serial debugging switch (active = on)

//byte correspondance
#define PIN595_BATT 0
#define PIN595_CNCTGREEN 1
#define PIN595_CNCTRED 2
#define PIN595_SYNCGREEN 3
#define PIN595_SYNCRED 4
#define PIN595_BUSY 5

//BATTERY ANALOG CORECTION
#define MASTER_ABATT_5 598
#define MASTER_ABATT_4 589
#define MASTER_ABATT_3 580
#define MASTER_ABATT_2 577
#define MASTER_ABATT_1 564



// *******************************************
// symbol on LCD
#define LCD_SYMBOL_CONNECT 0
#define LCD_SYMBOL_PLAYER 1
#define LCD_SYMBOL_BASE 2
#define LCD_SYMBOL_VOLUME 3
#define LCD_SYMBOL_TRACK 4
#define LCD_SYMBOL_BATTERY 5
#define LCD_SYMBOL_DISCONNECT 6
#define LCD_SYMBOL_MINTIME 7

#define LCDLINE 64

//********************************************
//sending code
#define CODE_GET_VALUE 0x0F
#define CODE_ACK_RESPONSE 0x0E
#define CODE_ACK_RESPONSE_SET_OK 0x0D
#define CODE_ACK_RESPONSE_MEMORY 0x08
#define CODE_GET_MEMORY 0x07
#define CODE_SET_ID 0x0B
#define CODE_SET_VALUE 0x0A
#define CODE_DISPO_TRY_CONNECT 0x05
#define CODE_BASE_TRY_CONNECT 0x04
#define CODE_BASE_SAY_OK 0x03
#define CODE_ACK_PLAYER_WAITING 0x02
#define CODE_ACK_PLAYER_GO 0x01






// *******************************************
// data stored
#define VALUE_ID_DISPO 0
#define MAX_VALUE_ID 255

#define VALUE_VOLUME 1
#define MAX_VALUE_VOLUME 15

#define VALUE_LANG 2
#define MAX_VALUE_LANG 5

#define VALUE_TRACK 3
#define MAX_VALUE_TRACK 30

#define VALUE_MIN_TIME 4
#define MAX_VALUE_MIN_TIME 255

#define VALUE_FADE_OUT_TIME 5
#define MAX_VALUE_FADE_OUT_TIME 10

#define VALUE_BASE_POSITION 6
#define MAX_VALUE_BASE_POSITION 30

#define VALUE_BASE_POWER 7
#define MAX_VALUE_BASE_POWER 4

#define VALUE_BASE_CONSECUTIVE_TRANSMIT_TO_GO 8
#define MAX_VALUE_BASE_CONSECUTIVE_TRANSMIT_TO_GO 7

#define VALUE_DAY 9
#define MAX_VALUE_DAY 31

#define VALUE_BATTERY 10
#define MAX_VALUE_BATTERY 6

#define VALUE_NEW_ID_DISPO 11
#define MAX_VALUE_NEW_ID_DISPO 200



//other data
#define VALUE_SCANPLAYER 12
#define VALUE_SCANBASE 13
#define VALUE_CONNECT 14
#define VALUE_SUPER_CONNECT 15

#define NB_VALUE_STORED 11
#define NB_VALUE_TOTAL 16

//*********************************************
//other
//State depends of switch
#define PLAYER 0x00
#define BASE 0x01
#define PLAYER_AUTO 0x02
#define PLAYER_CUSTOM 0x03


#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d" 
#define BYTETOBINARY(byte)  \
(byte & 0x80 ? 1 : 0), \
(byte & 0x40 ? 1 : 0), \
(byte & 0x20 ? 1 : 0), \
(byte & 0x10 ? 1 : 0), \
(byte & 0x08 ? 1 : 0), \
(byte & 0x04 ? 1 : 0), \
(byte & 0x02 ? 1 : 0), \
(byte & 0x01 ? 1 : 0) 




//*********************************************
//keypad

#define FRESHSTART 0
#define ADDID 1
#define RANGEID 2
#define ALL 3

#define VOLUMESET 1
#define LANGUAGESET 2


#endif
