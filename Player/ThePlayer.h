#ifndef __PLAYER_H__
#define __PLAYER_H__

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//includes
#include "ThePlayerDef.h"
//#include "printf.h"
#include <SPI.h>
#include <SdFat.h>
//#include <SdFatUtil.h>
#include <EEPROM.h>
#include "nRF24L01.h"
#include "RF24.h"

//#include "printf.h"

/**
 * VVboard running software
 */

class ThePlayer {
  
  /**
   * defife tralala
   */
#define tralala 1

private:
  /**
   * @name sdfat lib instance
   * Create the variables to be used by SdFat Library
   */
  /** \breif instance of SdFat to manage uSD card*/
  SdFat sd;
  /**@{*/
	//Sd2Card card; /**< instance of Sd2Card to manage the micro SD*/
	//SdVolume volume; /**< instance of SdVolume , open the volume on the micro SD*/
	//SdFile root;/**< instance of SdFile, open the root directory*/
  /** instance of SdFile the file we open for read the audio file*/
	SdFile track;
  //ofstream logfile;
  
  
  /**@}*/
  
  /**
   * @name attributes for leading communication
   *
   *  Privates methods to manage communication between base, player and remote
   */
  /**@{*/
  
  uint64_t ackT;/**< acknoledgement message - is the answer of the receiver in RF communication.
                 *usefull for remote because it never be a receiver
                 */
  uint64_t lastReceivedMessage;
  byte counterGoodMessage;
  
   /**@}*/
  
  /**
   * @name function for leading communication
   *
   *  Privates methods to manage communication between base, player and remote
   */
  /**@{*/

  /**
   * player calls for base
   *
   * schearch for the next base, to get the infos for the next track and
   * go if the communication is good (i.e. the player is in the good range)
   */
  void playerTryToConnectBase();
  
  /**
   * check incomming RF of RF chip in receiving mode
   *
   * very important function, dispatch incomming message by code to manage them 
   * could be frome remote, player or base (depends on role of the card)
   * include this function in loops : routine() & inPlayAction()
   * @return the ID of the sending card (usefull for base)
   */
  byte checkIncommingRF();
  
  
  //make config
  /**
   * update the sending adress of RF chip
   *
   * @param suffix is the adress of the ID to connect for BASE role.
   */
  void updateSendingAddress(byte suffix=0);
  
  /**
   * update the listenning adress of RF chip
   *
   * @param suffix is the adress of the card to connect for BASE role.
   */
  void updateListeningAddress();
  
  /**
   * set the acknoledgment message to answer in RF receiving mode to the remote
   */
	void setAckforRemote();
  
  /**
   * set the acknoledgment message to answer in RF receiving mode to the the base
   *
   * @param code to say the player state to the Base. could be : 
   * - CODE_ACK_PLAYER_WAITING player is waiting for go message
   * - CODE_ACK_PLAYER_GO player received go message and allready schearch for next base
   */
  void setAckforBase(byte code);
  
  /**
   * build the acknoledgment message to answer in RF receiving mode to the remote
   *
   * this function collect the infos in the value[] array and send gather it in order to
   * send it (by acknoledgment data) to the remote
   *
   * @param code to say the player state to the Base. could be :
   * - CODE_ACK_RESPONSE remote received simple answer
   * - CODE_ACK_RESPONSE_SET_OK remote received the updating value confirmation
   */
	void updateAckforRemote(byte code = CODE_ACK_RESPONSE);
  void updateAckforRemoteForMemory(byte code);
  
  /**
   * set the power of the RF chip
   */
  void setRF24Power();
  
  /**
   * simple print byte in bit function 
   */
  void printfByte(uint8_t b);
  
  /**
   * print the message wich is 64 bit variable
   *
   * @param m the message to print
   */
  void printMessage(uint64_t &m);
  
  /**
   * putting data in a structured message
   *
   * @param message the message.
   * @param value the value.
   * @param sizeInBit size of the value in bit. (depend on the maximum the value could reach)
   * @param shift define if the message is shifted before value is writting on the N lower bits (N depends on sizeInBit).
   */
  byte addValueToMessage(uint64_t &message, byte &value,byte sizeInBit,bool shift=true);

  /**
   * putting an indexed data from the datastack "value" in a structured message
   *
   * @param message the message.
   * @param index index of the value in value.
   * @param define if the message is shifted before value is writting on the N lower bits (N depends on MaxValue of the indexed value).
   */
  byte addIndexedValueToMessage(uint64_t &message, byte index,bool shift=true);
  /**
   * extracting data from structured message
   *
   * @param the message.
   * @param size of the value in bit. (depend on the maximum the value could reach)
   * @param define if the message is shifted before value is writting on the N lower bits (N depends on sizeInBit).
   */
  byte getValueFromMessage(uint64_t &message, byte sizeInBit,bool shift=true);
  /**
   * extracting idexed data from structured message and put it in the indexed data stack "value"
   *
   * @param the message.
   * @param index of the value.
   * @param define if the message is shifted before value is writting on the N lower bits (N depends on MaxValue of the indexed value).
   */
  byte getIndexedValueFromMessage(uint64_t &message, byte index,bool shift=true);
  /**
   * return indexed data from structured message
   *
   * @param the message.
   * @param index of the value.
   * @param define if the message is shifted before value is writting on the N lower bits (N depends on MaxValue of the indexed value).
   */
	byte getIndexedValueFromMessageWithoutStore(uint64_t &message, byte index);

 /**@}*/

	
	/**
   * @name init functions
   *
   *  Privates methods to manage communication between base, player and remote
   */
  /**@{*/
  
  /**
   * init the IO configuration for the Atmega
   */
  void initPin();
  
  /**
   * init the role (depend on switch)
   */
	void initRole();
  
  /**
   * init the SD card communication
   */
	void initSD();
  
  /**
   * init the VLSI CODEC CHIP communication
   *
   * this part of software was inspired by  Spark Fun Electronics - Nathan Seidle
   * Please refer to:
   * @li <a href="https://www.sparkfun.com/tutorials/295">Tutorial for MP3 shield</a>
   */
	void initMP3();
  
  /**
   * init the NORDIC NLRF24+ chip tranceiver
   *
   * this software use Driver for nRF24L01(+) 2.4GHz Wireless Transceiver
   * from James Coliz, Jr
   * Please refer to:
   * @li <a href="http://maniacbug.github.com/RF24/">Documentation Main Page</a>
   * @li <a href="http://maniacbug.github.com/RF24/classRF24.html">RF24 Class Documentation</a>
   * @li <a href="https://github.com/maniacbug/RF24/">Source Code</a>
   * @li <a href="https://github.com/maniacbug/RF24/archives/master">Downloads Page</a>
   * @li <a href="http://www.nordicsemi.com/files/Product/data_sheet/nRF24L01_Product_Specification_v2_0.pdf">Chip Datasheet</a>
   */
	void initRF24();
	 /**@}*/
	
	/**
   * @name data stack 
   *
   *  all the data are gather in array of byte for saving memory and quick building message
   *  to each data corresponding a maxValue to saving bit in the RF message (64bits max)
   */
  /**@{*/

	/** data stack for the value 
   *see .ThePlayerDef.h for details
   */
	byte value[NB_VALUE_TOTAL];
	byte maxValue[NB_VALUE_TOTAL]; /**< const data stack of the max value */
  /** data stack for the current playing value
   *see .ThePlayerDef.h for details
   */
  byte currentPlayingInfo[NB_INFO_TRACK]; 
  byte nextPlayingInfo[NB_INFO_TRACK]; /**< data stack for the next playing value  */
  byte nextTryPlaying[NB_INFO_TRACK];  /**< data stack for the next try playing value  */

	/**@}*/
  
  /**
   * @name eeprom functions
   *
   *  some data from the value data stack are saved and recall at init in eeprom
   * 
   */
  /**@{*/
  
  /**
   * read all eeprom data and put it in the data stack
   *
   * @see value
   */
	void eepromReadAll();
  
  /**
   * return the eeprom value
   *
   * @param address of the data in the eeprom
   */
	byte eepromRead(byte address);
  
  /**
   * write the eeprom value
   *
   * @param address of the data in the eeprom
   * the value get the same index (address) in the value data stack
   */
	void eepromWrite(byte address);
  
  /**
   * write the eeprom value
   *
   * @param address of the data in the eeprom
   * @param value
   * @param init
   * - true : the writing is in the init phase, do not modify more
   * - false : the writing is in the running phase (from remote update) so we need updating RF address , volume, RF power
   */
    void eepromWrite(byte address, byte data, bool init=false);
  
  /**
   * set a default config
   *
   * this config is writing the first time the software run and find native eeprom state
   */
    void setDefaultConfig();

  /**@}*/
  
	//debug variable-------------------------
	/*byte messageToByte(String &s);
	byte SerialCount;
	byte verbose;
	boolean mtest;
   
   //void serialDebug();
   //void processMessage();
   */

  
	/**
   * @name Base functions 
   *
   *  When the base Role is selected, we entering in base loop mode
   */
  /**@{*/
  
  /**
   * Base function
   */
	void runBaseLoop();
  
  /**
   * tryconnect player (new for est-tu-la)
   * @param id of the player to reach
   * @param the current count of succesfull dialog
   * @param message of base
   */
  byte tryconnectPlayer(byte playerID, byte count, uint64_t &message);
  
  /**
   * put the micro controller in light sleeping mode
   *
   * not used in the short version
   * @param longsleep
   - true : sleep for 8s
   - false : sleep for 32ms
   */
  
  
  void baseSleep();
  
  /**
   * put the micro controller in heavy sleeping mode
   * and call baseSleep(true);
   */
  void baseSleepTotal();
  
  /**
   * awake when the base in sleepTotalmode get RF message from remote
   */
  void activateBase();
  
  /**
   * return the millis corrected by the millisDerive
   */
  unsigned long millisCorrected();
/**@}*/
  
  /**
   * @name Base attributes
   *
   *  When the base Role is selected, we entering in base loop mode
   */
  /**@{*/
  
  unsigned long millisDerive;/**< millisDerive for corrected the millis timer lap because of sleep*/
  int timeStartOnBase;/**< the time of the starting base in minute*/
  
  /** the time before the base totalsleep in minutes
   * define to 720 (12h x 60 min)
   */
  int timeBeforeTotalSleep;
  /**@}*/
  
	
  /**
   * @name Players functions
   *
   *  
   */
  /**@{*/  
  
  /** routine call during the play loop when mcu is idle in audio stream
   */
	void inPlayActions();
  
  /** routime for changing volume
   */
  void changeVolume();
  
  /** routine call during the play loop when mcu is idle in audio stream
   *
   *@param volume in level as in the datastack and in the remote 0-14
   */
  void startChangeVolume(int volume);

  /** play track from the datastack info
   *
   * build the name lxx_tyy.mp3 where
   * - xx two digit for the language
   * - yy two digit for the track number
   */
	void playCurrentTrack();
  
  void logInSD(byte start=1);
  
  /** play track name
   *
   *this function contains a loop running until the end of mp3 file or force by stop()
   */
	void play(char* fileName);
  
  /** stopping the current play
   *
   * close the track and stop the decoding on the MP3 chip
   */
	void stop();
  
  /** feed the buffer
   *
   * read from the file in the sd card
   */
	byte feedBuffer(int &need_data);
  
  /** check if an other track is ready to play
   *
   * play the next file if the mintime is good
   */
  void checkToPlayTrack(bool print=false);
  
	void Mp3SetVolume(unsigned char leftchannel, unsigned char rightchannel);
	unsigned int Mp3ReadRegister(unsigned char addressbyte);
	void Mp3WriteRegister(unsigned char addressbyte, unsigned char highbyte,unsigned char lowbyte);
/**@}*/
  
  /**
   * @name Players attributes
   *
   *  When the base Role is selected, we entering in base loop mode
   */
  /**@{*/


  byte playerMemory[3];
  int tempsIntersceneJoue;
  unsigned long tryConnectBaseTime;/**< millis from the last message sending to the next base  */
  unsigned long delayTryConnectBase;/**< delay between two message  */
	bool playing;/**< is playing  */
	bool startPlayGoHome;/**<   */
  bool changingVolume;/**< volume is in change  */
  bool endFading;/**< fading is finish  */
  bool goplay;/**< we got next track to play  */
	unsigned long playingStartTime;/**< millis from the start time of playing  */
  unsigned long changeVolumeStartTime;/**< millis from the start of the changing volume  */
  byte changeVolumeCount;/**< the number of increment for changing volume  */
  byte volumeToReach;/**< volume to reah in raw level  */
  byte currentVolume;/**< current volume in raw level  */
  int changeVolumeTimeIncrement;/**< time between two change  */
  int minTimeToPlay;/**< min time to play in sec  */
  bool twoNextBaseConnect;/**< select if we try to connect the two next base  */
  byte twoNext;/**< 0 or 1  */
  bool minTimePassedGoCheckTwoNextBase;/**< after min time, we try to reach the two next base  */
  bool playerRegisseur;
  bool playerRegisseurSansOrdre;
  /**@}*/
  
  /**
   * @name Hardware function
   *
   *
   */
  /**@{*/
  
  /** get the battery status
   *
   * active the mosfet and check analog value
   */
  byte getBatteryStatus();/**< min time to play in sec  */
  int getAnalogBatteryValue();
  /**@}*/  


public:
	ThePlayer();
	~ThePlayer();
	void routine(); //include this in loop;
	void init(byte initID=0);

};

#endif // __PLAYER_H__
