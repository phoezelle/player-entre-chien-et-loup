/*! \file ThePlayerDef.h
 \brief settings
 
 */

#ifndef __ThePlayerDef_h__
#define __ThePlayerDef_h__



/**
 * @name general settings
 *
 */
/**@{*/
#define NB_LANGAGE_MAX 10
#define NB_TRACK_MAX 99

#define DEFAULT_VOLUME 11 /**< \brief volume is set default each startup*/

#define TIME_BEFORE_BASE_SLEEP 720 /**< \brief in minutes default 720min*/

/**@}*/

// *******************************************
/**
 * @name hardware IO pin settings
 *
 */
/**@{*/

//IO pin
#define MASTER_DEVICE 3  /**< \brief sensor or actuator for further use*/
#define MASTER_BTEST 4 /**< \brief OUTPUT active resitor bridge for battery test plus a led*/

#define MASTER_RF24_CE 5 /**< \brief RF24 pin chip enable*/
#define MASTER_RF24_CS 6 /**< \brief RF24 pin chip select*/

//MP3 Player Shield pin mapping. See the schematic
#define MP3_XCS 7 /**< \brief Control Chip Select Pin (for accessing SPI Control/Status registers)*/
#define MP3_XDCS 8 /**< \brief Data Chip Select / BSYNC Pin*/
#define MP3_DREQ 2 /**< \brief Data Request Pin: Player asks for more data*/
#define MP3_RESET 9 /**< \brief Reset is active low*/

#define SD_CS 10 /**< \brief Control uSD card Chip Select Pin (for accessing SPI read to SD card)*/
//Analog pin
#define MASTER_ABATT 0 /**< \brief analog pin for battery test*/
#define MASTER_AROLE1 1 /**< \brief analog pin for type select switch (sation = off / base = on)*/
#define MASTER_AROLE2 2 /**< \brief analog pin for verbose and serial debugging switch (active = on)*/

/**@}*/

// ********************************************
/**
 * @name dipswitch state on boot
 *
 */
/**@{*/

//State depends of switch
#define PLAYER 0x00
#define BASE 0x01
#define PLAYER_AUTO 0x02
#define PLAYER_CUSTOM 0x03

/**@}*/
/**
 * @name batterie analog value
 *
 */
/**@{*/

//BATTERY ANALOG CORECTION
#define MASTER_ABATT_5 598
#define MASTER_ABATT_4 589
#define MASTER_ABATT_3 580
#define MASTER_ABATT_2 577
#define MASTER_ABATT_1 564

/**@}*/

//other
#define TRUE  0
#define FALSE  1

/**
 * @name VS1033 address register
 *
 */
/**@{*/

//VS10xx SCI Registers
#define SCI_MODE 0x00
#define SCI_STATUS 0x01
#define SCI_BASS 0x02
#define SCI_CLOCKF 0x03
#define SCI_DECODE_TIME 0x04
#define SCI_AUDATA 0x05
#define SCI_WRAM 0x06
#define SCI_WRAMADDR 0x07
#define SCI_HDAT0 0x08
#define SCI_HDAT1 0x09
#define SCI_AIADDR 0x0A
#define SCI_VOL 0x0B
#define SCI_AICTRL0 0x0C
#define SCI_AICTRL1 0x0D
#define SCI_AICTRL2 0x0E
#define SCI_AICTRL3 0x0F

/**@}*/


//globals
#define BASE_POSITION_RF_ADRESS_OFFSET 0xC8 //200

//********************************************
/**
 * @name RF communication code
 *
 */
/**@{*/
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

/**@}*/

//********************************************
/**
 * @name index and maximum for value array storage
 *
 */
/**@{*/
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



//other data

#define ROLE 11

#define OK_SD 12

#define OK_RF24 13

#define OK_MP3 14


#define NB_VALUE_TOTAL 15

#define NB_VALUE_STORED 11

/**@}*/





//second tab of value for base or
/**
 * @name second value array for base
 *
 */
/**@{*/

#define PLAYING_BASE_POSITION 0
#define PLAYING_TRACK 1
#define PLAYING_MIN_TIME 2
#define PLAYING_FADE_OUT_TIME 3
#define PLAYING_ID_BASE 4

#define NB_INFO_TRACK 5


#define PLAYER 0x00
#define BASE 0x01
#define PLAYER_AUTO 0x02
#define PLAYER_CUSTOM 0x03

/**@}*/


#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d" 
/** macro for display binary value*/
#define BYTETOBINARY(byte)  \
(byte & 0x80 ? 1 : 0), \
(byte & 0x40 ? 1 : 0), \
(byte & 0x20 ? 1 : 0), \
(byte & 0x10 ? 1 : 0), \
(byte & 0x08 ? 1 : 0), \
(byte & 0x04 ? 1 : 0), \
(byte & 0x02 ? 1 : 0), \
(byte & 0x01 ? 1 : 0) 





#endif
