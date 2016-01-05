#ifndef _TheRemote_h
#define _TheRemote_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "Bounce.h"

#include <SPI.h>
#include "TheRemoteDef.h"
//Add RF24 radio module Libraries
#include "nRF24L01.h"
#include "RF24.h"

#include <SoftwareSerial.h>
#include <avr/pgmspace.h>

//#include "LCD.h"

/**
 * VVboard running remote soft
 */

class TheRemote {

private:
	//global variable
	boolean rotaryAState;
	boolean rotaryBState;
	long startConnectTime;

	//init functions

	void initPin();
  void initRole();
  byte role;

	//Draw menu function
	byte currentMenu;
	byte parrent;
	byte count;
	boolean editValue;
	boolean needRefresh;
    byte looseSync; // 0==OK ; 1==justlooseSync ; 2 loose sync between data in player/borne and data on teleco
	//define for each menu
	byte cursorPos[10];
	byte link[10];
	byte value[NB_VALUE_TOTAL];
	byte maxValue[NB_VALUE_STORED+1];
	byte maxCursorPos;
  byte maxCursorPosDisconnect;

	void initMenu(bool withValue, bool raz=false);
	byte getLink();
	void displayAllValue();
	void displayValue(byte n);
  void writeL(byte c);
  void displayValueConnect();
	void setCursorPos(byte b0 = 0, byte b1 = 0, byte b2 = 0, byte b3 = 0,
			byte b4 = 0, byte b5 = 0, byte b6 = 0, byte b7 = 0, byte b8 = 0,
			byte b9 = 0);
	void setLink(byte b0 = 0, byte b1 = 0, byte b2 = 0, byte b3 = 0,
			byte b4 = 0, byte b5 = 0, byte b6 = 0, byte b7 = 0, byte b8 = 0,
			byte b9 = 0);
	void setValue(byte b0 = 0, byte b1 = 0, byte b2 = 0, byte b3 = 0, byte b4 =
			0, byte b5 = 0, byte b6 = 0, byte b7 = 0, byte b8 = 0, byte b9 = 0,
			byte b10 = 0, byte b11 = 0, byte b12 = 0);

	//RF24 functions
  byte ID_teleco;
	void initRF24();
	boolean RF24Ok;
  //to talk to dispo
	void sendValue(bool manual=true);
  void sendID();
  void getValue(bool manual=true);
  bool sendMessage(byte code);
  void checkIncommingRF();
  void printfByte(uint8_t b);
  void printMessage(uint64_t &m);
  byte addValueToMessage(uint64_t &message, byte &value,byte sizeInBit);
  byte addIndexedValueToMessage(uint64_t &message, byte index);
  byte getValueFromMessage(uint64_t &message, byte sizeInBit);
  unsigned long getValueFromMessage(uint64_t &message);
  byte getIndexedValueFromMessage(uint64_t &message, byte index);
	byte getIndexedValueFromMessageWithoutStore(uint64_t &message, byte index);

  void connect(boolean getValue, boolean sendValue);
	void updateSendingAddress();
  void setListeningAddress();
  void setAck();
	void processValueFromPlayer();
	void processValueFromBase();
  bool connecting;

	//LCD functions
	void addAllCharToLCD();
	void selectLineOne(byte n);
	void selectLineTwo(byte n);
	void clearLCD();
	void boxCursorLCD(boolean b);
	void sendCommandLCD();
	//void checkLightOnLCD();
	void initLCD();
	void setSplashScreen();
	int lightValue;

	//Rotary functions
	void checkRotary();
	int encoder0Pos;
	boolean A_set;
	boolean B_set;

	//button
	bool buttonRoutine();
	boolean Bclignotement;
	boolean BholdOK;
    
    //595
    byte register595;
    void change595(byte pin, byte state);
    void fire595();
  
  byte getBatteryStatus();
  
  //keypad functions
  void checkSerialforkeypad();
  void restartkeypad();
  void changeState(byte idtoChange);
  byte selectedID[10];
  byte countindex;
  int selectedL;
  int selectedV;
  byte selectedValue;
  byte chosenModeForID;
  byte chosenAction;
  
  //EEPROM
  byte eepromRead(byte address);
	void eepromWrite(byte address);
  void eepromWrite(byte address, byte data);

public:
	TheRemote();
	~TheRemote();
	void routine(); //include this in loop;
	void init(byte b);

	void doEncoderA();

};

#endif
