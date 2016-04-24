#include "TheRemote.h"
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "Bounce.h"

#include <SPI.h>
#include <SoftwareSerial.h>
//Add RF24 radio module Libraries
#include "nRF24L01.h"
#include "RF24.h"
//#include "printf.h"

#include <EEPROM.h>


//For lighter program, if undef M_SERIAL_DEBUG, do not include debug code line.
#define R_SERIAL_DEBUG
#ifdef R_SERIAL_DEBUG
#define R_IF_SERIAL_DEBUG(x) ({x;})
#else
#define R_IF_SERIAL_DEBUG(x)
#endif

//#ifndef NATIVE
//#undef PROGMEM
//#define PROGMEM __attribute__(( section(".progmem.data") ))
//#undef PSTR
//#define PSTR(s) (__extension__({static prog_char __c[] PROGMEM = (s); &__c[0];}))
//#endif

// *************************************************************************************
//          Constructor, destructor and global variable
// *************************************************************************************

const uint64_t sendingPrefix = 0xF0FEE00000LL;
//PREFIX adress for base <=> station communication
//{ station send / base receive , station receive / base send}

RF24 radio(REMOTE_RF24_CE,REMOTE_RF24_CS);/**< instance of RF24 class for manage the Nordic NLRF24plus*/


SoftwareSerial myLCD = SoftwareSerial(-1, LCD_TX);

volatile boolean changeA = false;
Bounce buttonOK = Bounce(PB_OK, 30);
Bounce buttonUPDATE = Bounce(PB_UPDATE, 30);

TheRemote::TheRemote() {
}

TheRemote::~TheRemote() {
}

// *************************************************************************************
//          Routine to include to the loop function
// *************************************************************************************

void TheRemote::routine() {
  radio.stopListening();
  radio.startListening();
	while (true) {
    
		//checkLightOnLCD();
		while (changeA) {
			checkRotary();
		}
		if (editValue && needRefresh) displayValue(count);
    buttonRoutine();
    
		if (value[VALUE_SUPER_CONNECT]>0 && (millis() - startConnectTime) > 2000  && value[VALUE_ID_DISPO] != 0) {
      value[VALUE_SUPER_CONNECT]=0;
      displayValueConnect();
		}
    checkIncommingRF();
    checkSerialforkeypad();
    if(millis()%2000==0 && currentMenu==103){
      selectLineOne(1);
      myLCD.print("               ");
      selectLineOne(cursorPos[count]);
    }
    //check battery of teleco
    if(millis()%10000==0){
      if(getBatteryStatus()>1){
        change595(PIN595_BATT, 0);
        fire595();
      }else{
        change595(PIN595_BATT, 1);
        fire595();
      }
    }
    
	}
  
}

// *************************************************************************************
//          INIT functions
// *************************************************************************************

//big init for the whole remote
void TheRemote::init( byte b) {
	ID_teleco =b;
  encoder0Pos = 0;
	Serial.begin(9600); //Use serial for debugging
  register595=0;
	//printf_begin();
  SPI.begin();
	delay(500);
  
	R_IF_SERIAL_DEBUG(printf_P(PSTR("\n\r--> VV Remote IN PROGRESS \n\r")));
  
	maxValue[VALUE_ID_DISPO] = MAX_VALUE_ID;
	maxValue[VALUE_VOLUME] = MAX_VALUE_VOLUME;
	maxValue[VALUE_LANG] = MAX_VALUE_LANG;
	maxValue[VALUE_TRACK] = MAX_VALUE_TRACK;
	maxValue[VALUE_MIN_TIME] = MAX_VALUE_MIN_TIME;
	maxValue[VALUE_FADE_OUT_TIME] = MAX_VALUE_FADE_OUT_TIME;
	maxValue[VALUE_BASE_POSITION] = MAX_VALUE_BASE_POSITION;
  maxValue[VALUE_BASE_POWER] = MAX_VALUE_BASE_POWER;
	maxValue[VALUE_BASE_CONSECUTIVE_TRANSMIT_TO_GO] = MAX_VALUE_BASE_CONSECUTIVE_TRANSMIT_TO_GO;
  maxValue[VALUE_BATTERY] = MAX_VALUE_BATTERY;
  maxValue[VALUE_NEW_ID_DISPO] = MAX_VALUE_NEW_ID_DISPO;
  maxValue[VALUE_DAY] = MAX_VALUE_DAY;
  
  restartkeypad();
  
  initPin();
  initRole();
  initLCD();
  initRF24();
  currentMenu = 100;
  initMenu(false,true);
  editValue = false;
  connecting = false;
  value[VALUE_DAY]=eepromRead(VALUE_DAY);
  if(value[VALUE_DAY]==255){
    value[VALUE_DAY]=0;
    eepromWrite(VALUE_DAY);
  }
  R_IF_SERIAL_DEBUG(printf_P(PSTR("DAY read : %u\n\r"),value[VALUE_DAY]));
  fire595();
}

void TheRemote::initRole() {
	R_IF_SERIAL_DEBUG(printf_P(PSTR("check analog switch\n\r")));
	byte b = 0;
	if (analogRead(MASTER_AROLE1) > 200)
		b += 1;
	if (analogRead(MASTER_AROLE2) > 200)
		b += 2;
	role = b;
	R_IF_SERIAL_DEBUG(printf_P(PSTR("ROLE IS = %u (0=classic, 1=superTelec, 2=superUser, 3=superTelec&superUser)\n\r"),role));
}

void TheRemote::initPin() {
  pinMode(LCD_TX, OUTPUT);
  //pinMode(LED_CONNECT, OUTPUT);
  pinMode(ROTARY_A, INPUT);
  pinMode(ROTARY_B, INPUT);
  digitalWrite(ROTARY_A, HIGH);
  digitalWrite(ROTARY_B, HIGH);
  pinMode(PB_OK, INPUT);
  pinMode(PIN595CSN, OUTPUT);
  digitalWrite(PIN595CSN, HIGH);
  pinMode(REMOTE_RF24_SPARKFUN_CS,OUTPUT);
  digitalWrite(REMOTE_RF24_SPARKFUN_CS,HIGH);
  pinMode(REMOTE_RF24_SPARKFUN_CE,OUTPUT);
  digitalWrite(REMOTE_RF24_SPARKFUN_CE,HIGH);
  pinMode(PB_UPDATE, INPUT);
  R_IF_SERIAL_DEBUG(printf_P(PSTR("\n\r Init Pin \n\r")));
}

// *************************************************************************************
//          Display Menu function
// *************************************************************************************

void TheRemote::initMenu(bool withValue, bool raz) {
  count = 1;
  clearLCD();
  boxCursorLCD(true);
  selectLineOne(0);
  maxValue[VALUE_ID_DISPO] = MAX_VALUE_ID;
  if (raz)setValue(); //erase all value
  if (currentMenu == 100) { //home menu
    value[VALUE_CONNECT] = 0;
    change595(PIN595_SYNCGREEN,0);
    change595(PIN595_SYNCRED,0);
    change595(PIN595_CNCTGREEN, 0);
    change595(PIN595_CNCTRED, 0);
    fire595();
    count = 0;
    maxCursorPos = 4;
    maxCursorPosDisconnect=maxCursorPos;
    setCursorPos(0, 8, LCDLINE, LCDLINE + 8);
    setLink(101, 102, 103, 104);
    R_IF_SERIAL_DEBUG(printf_P(PSTR("link %u \n\r"),link[1]));
    myLCD.write(LCD_SYMBOL_PLAYER);
    myLCD.print("player ");
    myLCD.write(LCD_SYMBOL_BASE);
    myLCD.print("base");
    selectLineTwo(0);
    myLCD.write((byte) LCD_SYMBOL_CONNECT);
    myLCD.print("infos  ");
    myLCD.write(LCD_SYMBOL_TRACK);
    myLCD.print("setDAY");
  }
  
  if (currentMenu == 101) { //player menu
    maxValue[VALUE_ID_DISPO]=110;
    parrent = 100;
    maxCursorPos = 5;
    maxCursorPosDisconnect=2;
    setCursorPos(0, 2, 15, LCDLINE, LCDLINE + 5, 10, 13);
    setLink(100, VALUE_ID_DISPO, 113, VALUE_VOLUME, VALUE_LANG, VALUE_BATTERY,VALUE_CONNECT);
    myLCD.print("X ");
    myLCD.write(LCD_SYMBOL_PLAYER);
    myLCD.print("---    ");
    myLCD.write(LCD_SYMBOL_BATTERY);
    myLCD.print("-   i");
    selectLineTwo(0);
    myLCD.write(LCD_SYMBOL_VOLUME);
    myLCD.print("--  ");
    myLCD.print("L------");
  }
  
  if (currentMenu == 102) { // base menu
    maxValue[VALUE_ID_DISPO]=181;
    parrent = 100;
    maxCursorPos = 4;
    maxCursorPosDisconnect=2;
    setCursorPos(0, 2, LCDLINE, LCDLINE + 10, 10, 13);
    setLink(100, VALUE_ID_DISPO, 111,112,VALUE_BATTERY, VALUE_CONNECT);
    myLCD.print("X ");
    myLCD.write(LCD_SYMBOL_BASE);
    myLCD.print("---    ");
    myLCD.write(LCD_SYMBOL_BATTERY);
    myLCD.print("-  ");
    selectLineTwo(0);
    myLCD.write(LCD_SYMBOL_BASE);
    myLCD.print("power    ");
    myLCD.write(LCD_SYMBOL_BASE);
    myLCD.print("track");
    
  }
  
  if (currentMenu == 111) { // base power menu
    parrent = 102;
    maxCursorPos = 3;
    maxCursorPosDisconnect=maxCursorPos;
    setCursorPos(0, LCDLINE+3, LCDLINE + 9,13,2);
    setLink(102, VALUE_BASE_POWER, VALUE_BASE_CONSECUTIVE_TRANSMIT_TO_GO, VALUE_CONNECT,VALUE_ID_DISPO);
    myLCD.print("X ");
    myLCD.write(LCD_SYMBOL_BASE);
    selectLineTwo(0);
    myLCD.print("pow");
    myLCD.write(LCD_SYMBOL_BASE);
    myLCD.print("-  nb");
    myLCD.write(LCD_SYMBOL_BASE);
    myLCD.print("-");
    withValue=true;
  }
  
  if (currentMenu == 112) { // base track menu
    parrent = 102;
    maxCursorPos = 5;
    maxCursorPosDisconnect=maxCursorPos;
    setCursorPos(0, 12, LCDLINE, LCDLINE + 4, LCDLINE + 12, 2);
    setLink(102, VALUE_BASE_POSITION, VALUE_TRACK, VALUE_MIN_TIME, VALUE_FADE_OUT_TIME, VALUE_ID_DISPO);
    myLCD.print("X ");
    myLCD.write(LCD_SYMBOL_BASE);
    myLCD.print("---   pos");
    myLCD.write(LCD_SYMBOL_BASE);
    myLCD.print("--");
    selectLineTwo(0);
    myLCD.write(LCD_SYMBOL_TRACK);
    myLCD.print("-- ");
    myLCD.write(LCD_SYMBOL_MINTIME);
    myLCD.print("-:-- f>");
    myLCD.write(LCD_SYMBOL_MINTIME);
    myLCD.print("--");
    withValue=true;
  }
  
  if (currentMenu == 113) { //scan menu
    parrent = 101;
    maxCursorPos = 0;
    maxCursorPosDisconnect=maxCursorPos;
    setCursorPos(LCDLINE+15);
    count=0;
    setLink(101);
    selectLineTwo(0);
    myLCD.print("               X");
    updateSendingAddress();
    sendMessage(CODE_GET_MEMORY);
  }
  
  if (currentMenu == 103) { //scan menu
    parrent = 100;
    maxCursorPos = 0;
    maxCursorPosDisconnect=maxCursorPos;
    setCursorPos(LCDLINE);
    count=0;
    setLink(100);
    myLCD.write(LCD_SYMBOL_BASE);
    selectLineTwo(0);
    myLCD.print("X  ");
    myLCD.write(LCD_SYMBOL_BATTERY);
    myLCD.print("battery ");
    myLCD.print(getBatteryStatus());
  }
  
  if (currentMenu == 104) { //set DAY menu //saved in eeprom
    parrent = 100;
    maxCursorPos = 2;
    maxCursorPosDisconnect=maxCursorPos;
    setCursorPos(0, 5);
    setLink(100, VALUE_DAY);
    myLCD.print("X DAY ");
    displayValue(1);
    
  }
  
  if(withValue)displayAllValue();
  selectLineOne(cursorPos[count]);
  R_IF_SERIAL_DEBUG(printf_P(PSTR("draw menu %u \n\r"),currentMenu));
}

void TheRemote::displayAllValue() {
  boxCursorLCD(false);
  displayValue(0);
  for (byte i = 1; cursorPos[i] != 0; i++) {
    displayValue(i);
  }
  if (!editValue)boxCursorLCD(true);
  selectLineOne(cursorPos[count]);
}

void TheRemote::displayValueConnect(){
  bool ledOnly=false;
  if (currentMenu == 102 || currentMenu == 102 || currentMenu ==104 || currentMenu ==111 || currentMenu==112){
    if (currentMenu==112) {
      ledOnly=true;
    }
    if(!ledOnly)selectLineOne(14);
    //connect
    if (value[VALUE_SUPER_CONNECT] == 0) {
      if(!ledOnly)myLCD.write((byte) LCD_SYMBOL_DISCONNECT);
      change595(PIN595_CNCTGREEN, 0);
      change595(PIN595_CNCTRED, 1);
    }else{
      if(!ledOnly)myLCD.write((byte) LCD_SYMBOL_CONNECT);
    }
    if (value[VALUE_SUPER_CONNECT] == CODE_BASE_SAY_OK) {
      if(!ledOnly)myLCD.write((byte) LCD_SYMBOL_CONNECT);
      change595(PIN595_CNCTGREEN, 1);
      change595(PIN595_CNCTRED, 0);
    }
    if (value[VALUE_SUPER_CONNECT] == CODE_BASE_TRY_CONNECT) {
      if(!ledOnly)myLCD.write((byte) LCD_SYMBOL_DISCONNECT);
      change595(PIN595_CNCTGREEN, 1);
      change595(PIN595_CNCTRED, 1);
      
    }
    if (value[VALUE_SUPER_CONNECT] < 2) {
      if(!ledOnly)myLCD.write(" ");
    }
    fire595();
    if(!editValue && !ledOnly) boxCursorLCD(true);
    if(!ledOnly)selectLineOne(cursorPos[count]);
  }
}


void TheRemote::displayValue(byte n) {
  if(link[n]<100 && (value[VALUE_CONNECT]==1 || link[n]==VALUE_ID_DISPO || link[n]==VALUE_DAY)){
    selectLineOne(cursorPos[n] + 1);
    needRefresh = false;
    if(looseSync==1){
      looseSync=2;
      change595(PIN595_SYNCGREEN, 0);
      change595(PIN595_SYNCRED, 1);
      fire595();
    }
    //display time
    if (link[n] == VALUE_MIN_TIME) {
      float f = value[VALUE_MIN_TIME] * 2;
      myLCD.print((int) f / 60);
      myLCD.print(":");
      if ((value[VALUE_MIN_TIME] * 2) % 60 < 10)
        myLCD.print("0");
      myLCD.print((value[VALUE_MIN_TIME] * 2) % 60);
      return;
    }
    //display lang
    if (link[n] == VALUE_LANG) {
      writeL(value[link[n]]);
      return;
    }
    //connect
    if (link[n] == VALUE_CONNECT) {
      displayValueConnect();
      return;
    }
    if (link[n] == VALUE_BATTERY && value[link[n]]==0) {
      myLCD.print("0!");
      return;
    }
    if (link[n] == VALUE_BATTERY && value[link[n]]==5) {
      myLCD.print("5+");
      return;
    }
    
    //straight numeric
    byte max=maxValue[link[n]];
    if (link[n]==VALUE_NEW_ID_DISPO) max=maxValue[VALUE_ID_DISPO];
    if (value[link[n]] < 100 && max> 99)
      myLCD.print("0");
    if (value[link[n]] < 10 && max > 9)
      myLCD.print("0");
    myLCD.print(value[link[n]]);
  }
}

void TheRemote::writeL(byte c){
  switch (c) {
    case 0:
      myLCD.print(" verenaV");
      break;
    case 1:
      myLCD.print(" eden   ");
      break;
    case 2:
      myLCD.print(" jordan ");
      break;
    case 3:
      myLCD.print(" franck ");
      break;
    case 4:
      myLCD.print(" ??     ");
      break;
    default:
      break;
  }
}


byte TheRemote::getLink() {
  return link[count];
}

void TheRemote::setCursorPos(byte b0, byte b1, byte b2, byte b3, byte b4,
                             byte b5, byte b6, byte b7, byte b8, byte b9) {
  //R_IF_SERIAL_DEBUG(printf_P(PSTR("cursorpos tocopy %u at index %u \n\r"),tocopy[i],i));
  cursorPos[0] = b0;
  cursorPos[1] = b1;
  cursorPos[2] = b2;
  cursorPos[3] = b3;
  cursorPos[4] = b4;
  cursorPos[5] = b5;
  cursorPos[6] = b6;
  cursorPos[7] = b7;
  cursorPos[8] = b8;
  cursorPos[9] = b9;
  //R_IF_SERIAL_DEBUG(printf_P(PSTR("cursorPos copy %u at index %u \n\r"),cursorPos[i],i));
}

void TheRemote::setLink(byte b0, byte b1, byte b2, byte b3, byte b4, byte b5,
                        byte b6, byte b7, byte b8, byte b9) {
  //R_IF_SERIAL_DEBUG(printf_P(PSTR("link tocopy %u at index %u \n\r"),tocopy[i],i));
  link[0] = b0;
  link[1] = b1;
  link[2] = b2;
  link[3] = b3;
  link[4] = b4;
  link[5] = b5;
  link[6] = b6;
  link[7] = b7;
  link[8] = b8;
  link[9] = b9;
  //R_IF_SERIAL_DEBUG(printf_P(PSTR("link copy %u at index %u \n\r"),link[i],i));
}

void TheRemote::setValue(byte b0, byte b1, byte b2, byte b3, byte b4, byte b5,
                         byte b6, byte b7, byte b8, byte b9, byte b10, byte b11, byte b12) {
  ///R_IF_SERIAL_DEBUG(printf_P(PSTR("cursorpos tocopy %u at index %u \n\r"),tocopy[i],i));
  value[0] = b0;
  value[1] = b1;
  value[2] = b2;
  value[3] = b3;
  value[4] = b4;
  value[5] = b5;
  value[6] = b6;
  value[7] = b7;
  value[8] = b8;
  value[9] = b9;
  value[10] = b10;
  value[11] = b11;
  value[12] = b12;
  R_IF_SERIAL_DEBUG(printf_P(PSTR("set value\n\r")));
  //R_IF_SERIAL_DEBUG(printf_P(PSTR("value copy %u at index %u \n\r"),value[i],i));
}

// *************************************************************************************
//          RF24 functions
// *************************************************************************************

void TheRemote::initRF24() {
  
  R_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rinit RF24 chip\n\r")));
  if (role==1 || role==3) {
    radio.ce(REMOTE_RF24_SPARKFUN_CE);
    radio.csn(REMOTE_RF24_SPARKFUN_CS);
  }
  radio.begin();
  radio.setRetries(15, 15); // 15 try delay 4ms each
  radio.setChannel(125);
  //radio.setDataRate(RF24_250KBPS);
  radio.setPayloadSize(8);
  radio.enableAckPayload();
  setListeningAddress();
  radio.setPALevel(RF24_PA_MAX);
  radio.stopListening();
  //radio.printDetails();
  RF24Ok = radio.isPVariant();
  setAck();
  R_IF_SERIAL_DEBUG(printf_P(PSTR("Radio module %s\n\r"), RF24Ok ? "OK" : "PROBLEM"););
}

void TheRemote::updateSendingAddress() {
  uint64_t adress = sendingPrefix | value[VALUE_ID_DISPO];
  radio.openWritingPipe(adress);
  R_IF_SERIAL_DEBUG(printf_P(PSTR("talk to %u ---- "),lowByte(adress)));
}

void TheRemote::setListeningAddress(){
  uint64_t address = sendingPrefix | ID_teleco;
  radio.openReadingPipe(1, address);
  R_IF_SERIAL_DEBUG(printf_P(PSTR("RF in adress %u\n\r"),lowByte(address)));
}

void TheRemote::setAck() {
	R_IF_SERIAL_DEBUG(printf_P(PSTR("define the ACK reponse for base\n\r")));
	uint64_t ack = 0 | CODE_ACK_PLAYER_WAITING;
  radio.writeAckPayload(1, &ack, sizeof(ack));
  //radio.startListening();
}

void TheRemote::sendValue(bool manual){
  clearLCD();
  selectLineOne(0);
  myLCD.write("updating value");
  bool connect = sendMessage(CODE_SET_VALUE);
  if(connect){
    change595(PIN595_SYNCGREEN,1);
    change595(PIN595_SYNCRED,0);
    fire595();
    selectLineTwo(5);
    myLCD.write("OK");
    looseSync=0;
    if(manual)delay(500);
    
  }else{
    change595(PIN595_SYNCGREEN,0);
    change595(PIN595_SYNCRED,1);
    fire595();
  }
  radio.startListening();
  initMenu(true);
}

/*void TheRemote::sendID(){
  clearLCD();
  selectLineOne(0);
  myLCD.write("updating ID");
  if(sendMessage(CODE_SET_ID)){
    selectLineTwo(5);
    myLCD.write("OK");
    delay(500);
  }
  initMenu(true);
}*/

void TheRemote::getValue(bool manual){
  clearLCD();
  selectLineOne(0);
  myLCD.write("try connecting");
  bool connect = sendMessage(CODE_GET_VALUE);
  if (connect){
    change595(PIN595_SYNCGREEN,1);
    change595(PIN595_SYNCRED,0);
    fire595();
    selectLineTwo(5);
    myLCD.write("OK");
    if(manual)delay(500);
    looseSync=0;
  }else{
    change595(PIN595_SYNCGREEN,0);
    change595(PIN595_SYNCRED,1);
    fire595();
    looseSync=0;
  }
  initMenu(connect);
  count++;
  displayValue(1);
}

bool TheRemote::sendMessage(byte code){
  radio.stopListening();
  connecting = true;
  value[VALUE_CONNECT] = 0;
  //build message
  uint64_t message = 0;
  uint64_t returningACK = 0;
  byte countM = 0;
  if (code == CODE_SET_ID) value[VALUE_ID_DISPO] = value[VALUE_NEW_ID_DISPO];
  for (byte i = 0; i < NB_VALUE_STORED; i++) {
    countM+=addIndexedValueToMessage(message,i);
  }
  addValueToMessage(message,code,4);
  printMessage(message);
  R_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rmessage size : %u with code %X\n\r"),countM, code));
  byte replyCode = 0x00;
  //sending phase
  R_IF_SERIAL_DEBUG(printf_P(PSTR("sending to dispo"),message));
  while (true){
    //change595(PIN595_BUSY, 1);
    //fire595();
    R_IF_SERIAL_DEBUG(printf_P(PSTR("."),message));
    bool connect = radio.write(&message, sizeof(message));
    R_IF_SERIAL_DEBUG(if(connect)printf_P(PSTR("c"),message));
    delay(5);
    if (connect && radio.isAckPayloadAvailable()) {
      value[VALUE_CONNECT] = 1;
      value[VALUE_SUPER_CONNECT] = 1;
      radio.read(&returningACK, sizeof(returningACK));
      printMessage(returningACK);
      replyCode = getValueFromMessage(returningACK,4);
      R_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rreceive ACK from player code %X\n\r"),replyCode));
    }
    //exit only if buttonUPDATE hit or receive reply
    if ((replyCode == CODE_ACK_RESPONSE_MEMORY && code == CODE_GET_MEMORY)||(replyCode == CODE_ACK_RESPONSE && code == CODE_GET_VALUE) || (replyCode== CODE_ACK_RESPONSE_SET_OK && (code == CODE_SET_ID || code==CODE_SET_VALUE))) {
      //change595(PIN595_BUSY, 0);
      //fire595();
      break;
    }
    if (buttonRoutine()) {
      connecting = false;
      R_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rexit by hand\n\r")));
      currentMenu=100;
      //change595(PIN595_BUSY, 0);
      //fire595();
      return false;
    }
    //change595(PIN595_BUSY, 0);
    //fire595();
    delay(50);
  }
  if (code==CODE_GET_VALUE && replyCode==CODE_ACK_RESPONSE) {
    //then get values in the ack message
    byte rf24 = getValueFromMessage(returningACK,1);
    R_IF_SERIAL_DEBUG(printf_P(PSTR("1rf24=%u ; "),rf24));
    byte mp3 = getValueFromMessage(returningACK,1);
    R_IF_SERIAL_DEBUG(printf_P(PSTR("1mp3=%u ; "),mp3));
    byte sd = getValueFromMessage(returningACK,1);
    R_IF_SERIAL_DEBUG(printf_P(PSTR("1sd=%u ; "),sd));
    byte role = getValueFromMessage(returningACK,2);
    R_IF_SERIAL_DEBUG(printf_P(PSTR("2Brole=%u ; "),role));
    for (int i = NB_VALUE_STORED; i > 0; i--) {
      getIndexedValueFromMessage(returningACK,i-1);
    }
  }
  if (replyCode == CODE_ACK_RESPONSE_MEMORY && code == CODE_GET_MEMORY && currentMenu==113){
    for(byte j=0; j<3; j++){
      printMessage(returningACK);
      byte memory = lowByte(returningACK);
      returningACK= returningACK>>8;
      for (byte i=0; i<8; i++) {
        selectLineOne(j*8+i);
        if (j*8+i>15) {
          selectLineTwo(j*8+i-16);
        }
        if(memory&(0x01<<i))myLCD.write((byte) LCD_SYMBOL_CONNECT); else myLCD.write("-");
      }
    }
  }
  startConnectTime=millis();
  radio.startListening();
  connecting = false;
  return true;
}


void TheRemote::checkIncommingRF(){
  if(radio.available()){
    change595(PIN595_BUSY, 1);
    fire595();
    R_IF_SERIAL_DEBUG(printf_P(PSTR("incomming message ")));
    uint64_t received = 0;
    boolean done=false;
    int unlockCount=0;
    while (!done && unlockCount<20) {
      //M_IF_SERIAL_DEBUG(printf_P(PSTR(".")));
      // Fetch the payload, and see if this was the last one.
      done = radio.read(&received, sizeof(received));
      unlockCount++;
      //delay(20);//debug
    }
    R_IF_SERIAL_DEBUG(printf_P(PSTR("C[%u] "),unlockCount));
    if (unlockCount>18){
      R_IF_SERIAL_DEBUG(printf_P(PSTR("BAD RF\n\r")));
      radio.powerDown();
      delay(10);
      radio.startListening();
      return;
    }
    //printMessage(received);
    byte code = getValueFromMessage(received,4);
    getIndexedValueFromMessageWithoutStore(received,VALUE_BASE_POSITION);
    getIndexedValueFromMessageWithoutStore(received,VALUE_FADE_OUT_TIME);
    getIndexedValueFromMessageWithoutStore(received,VALUE_MIN_TIME);
    getIndexedValueFromMessageWithoutStore(received,VALUE_TRACK);
    byte id = getIndexedValueFromMessageWithoutStore(received,VALUE_ID_DISPO);
    R_IF_SERIAL_DEBUG(printf_P(PSTR("message code %X from base %u\n\r"),code,id));
    if(currentMenu==103){
      selectLineOne(id-150);
      if (code==3)myLCD.write((byte) LCD_SYMBOL_CONNECT);else myLCD.write((byte) LCD_SYMBOL_DISCONNECT);
      selectLineOne(cursorPos[count]);
    }
    if(id == value[VALUE_ID_DISPO]){
      startConnectTime=millis();
      value[VALUE_SUPER_CONNECT]=code;
      R_IF_SERIAL_DEBUG(printf_P(PSTR(" update superConnect value\n\r"),code,id));
      displayValueConnect();
    }
    //no response
    setAck();
    
    change595(PIN595_BUSY, 0);
    fire595();
  }
}


void TheRemote::printfByte(uint8_t b){
  R_IF_SERIAL_DEBUG(printf_P(PSTR(BYTETOBINARYPATTERN),BYTETOBINARY(b)));
}

void TheRemote::printMessage(uint64_t &m){
  R_IF_SERIAL_DEBUG(printf_P(PSTR("\n\r")));
  for (byte i=0; i<sizeof(m); i++) {
    printfByte(lowByte(m>>((sizeof(m)-i-1)*8)));
    R_IF_SERIAL_DEBUG(printf_P(PSTR(".")));
  }
}

byte TheRemote::addValueToMessage(uint64_t &message, byte &thevalue,byte sizeInBit){
  message <<= sizeInBit;
  message |= thevalue & (0xFF>>(8-sizeInBit));
  R_IF_SERIAL_DEBUG(printf_P(PSTR("(%uB)no index value %u ; "),sizeInBit,lowByte(message)&(0xFF>>(8-sizeInBit))));
  return sizeInBit;
}

byte TheRemote::addIndexedValueToMessage(uint64_t &message, byte index){
  if (index==VALUE_DAY) value[VALUE_DAY]=eepromRead(VALUE_DAY);
  byte nbit=1;
  uint8_t r=maxValue[index];
  while (r>>=1!=0x00)nbit++;
  message <<= nbit;
  message |= value[index]& (0xFF>>(8-nbit));
  R_IF_SERIAL_DEBUG(printf_P(PSTR("(%uB)index %u value %u-%u ; "),nbit,index,value[index], lowByte(message)&(0xFF>>(8-nbit))));
  return nbit;
}


byte TheRemote::getValueFromMessage(uint64_t &message, byte sizeInBit){
  byte thevalue=lowByte(message)&(0xFF>>(8-sizeInBit));
  //printfByte(thevalue);
  //R_IF_SERIAL_DEBUG(printf_P(PSTR("(%uB)get no index value %u ; "),sizeInBit,thevalue));
  message >>= sizeInBit;
  return thevalue;
}

unsigned long TheRemote::getValueFromMessage(uint64_t &message){
  unsigned long thevalue=(unsigned long)message;
  //printfByte(thevalue);
  //R_IF_SERIAL_DEBUG(printf_P(PSTR("(%uB)get no index value %u ; "),sizeInBit,thevalue));
  message >>= 32;
  return thevalue;
}


byte TheRemote::getIndexedValueFromMessageWithoutStore(uint64_t &message, byte index){
  byte nbit=1;
  uint8_t r=maxValue[index];
  while (r>>=1!=0x00)nbit++;
  byte v = lowByte(message)&(0xFF>>(8-nbit));
  //printfByte(value[index]);
  //R_IF_SERIAL_DEBUG(printf_P(PSTR("(%uB)get index %u value %u ; "),nbit,index,value[index]));
  message >>= nbit;
  return v;
}

byte TheRemote::getIndexedValueFromMessage(uint64_t &message, byte index){
  byte nbit=1;
  uint8_t r=maxValue[index];
  while (r>>=1!=0x00)nbit++;
  value[index] = lowByte(message)&(0xFF>>(8-nbit));
  //printfByte(value[index]);
  //R_IF_SERIAL_DEBUG(printf_P(PSTR("(%uB)get index %u value %u ; "),nbit,index,value[index]));
  message >>= nbit;
  return value[index];
}




void TheRemote::connect(boolean getValue, boolean sendValue) {
  /*value[VALUE_CONNECT] = 0;
   startConnectTime = millis();
   delay(10);
   updateSendingAddress();
   
   //ack received (4 bytes);
   
   if (sendValue) {
   byte countM = 8;
   for (byte i = 0; i < NB_VALUE_STORED; i++) {
   if (i != 0 && maxValue[i] < 16) {
   
   }
   if (i != 0 && maxValue[i] < 64 && maxValue[i] > 15) {
   message <<= 6;
   countM += 6;
   }
   if (i != 0 && maxValue[i] > 63) {
   message <<= 8;
   countM += 8;
   }
   message |= value[i];
   if (maxValue[i] < 64)
   R_IF_SERIAL_DEBUG(
   printf_P(PSTR("6B%u=%u ; "),i,lowByte(message)&B00111111));
   if (maxValue[i] > 63)
   R_IF_SERIAL_DEBUG(
   printf_P(PSTR("8B%u=%u ; "),i,lowByte(message)));
   }
   message <<= 4;
   if (count == 0) {
   message |= 0x0B;
   } else {
   message |= 0x0A;
   }
   countM += 4;
   R_IF_SERIAL_DEBUG(printf_P(PSTR("8Bsendvaluecode=0x%X ; "),lowByte(message)&B00001111));
   R_IF_SERIAL_DEBUG(printf_P(PSTR("message size : %u \n\r"),countM));
   R_IF_SERIAL_DEBUG(printf_P(PSTR("sending to dispo %LX \n\r"),message));
   //(id,volume,lang,battery)
   value[VALUE_CONNECT] = radio.write(&message, sizeof(message));
   if (value[VALUE_CONNECT] == 1)
   R_IF_SERIAL_DEBUG(printf_P(PSTR("message write good \n\r")));
   delay(10);
   if (radio.isAckPayloadAvailable()) {
   value[VALUE_CONNECT] = 1;
   radio.read(&message, sizeof(message));
   R_IF_SERIAL_DEBUG(printf_P(PSTR("receive from player \n\r")));
   }
   delay(50);
   }
   message = 0x0FLL;
   R_IF_SERIAL_DEBUG(
   printf_P(PSTR("8Bsendvaluecode=0x%X \n\r"),lowByte(message)&B00001111));
   
   value[VALUE_CONNECT] = radio.write(&message, sizeof(message));
   if (value[VALUE_CONNECT] == 1)
   R_IF_SERIAL_DEBUG(printf_P(PSTR("message write good \n\r")));
   delay(10);
   if (radio.isAckPayloadAvailable()) {
   value[VALUE_CONNECT] = 1;
   radio.read(&message, sizeof(message));
   R_IF_SERIAL_DEBUG(printf_P(PSTR(" receive from player \n\r")));
   if (getValue && value[VALUE_CONNECT] == 1) {
   byte rf24 = lowByte(message) & B00000001;
   message >>= 1;
   R_IF_SERIAL_DEBUG(printf_P(PSTR("1rf24=%u ; "),rf24));
   byte mp3 = lowByte(message) & B00000001;
   message >>= 1;
   R_IF_SERIAL_DEBUG(printf_P(PSTR("1mp3=%u ; "),mp3));
   byte sd = lowByte(message) & B00000001;
   message >>= 1;
   R_IF_SERIAL_DEBUG(printf_P(PSTR("1sd=%u ; "),sd));
   byte role = lowByte(message) & B00000011;
   message >>= 2;
   R_IF_SERIAL_DEBUG(printf_P(PSTR("2Brole=%u ; "),role));
   
   for (int i = NB_VALUE_STORED; i >= 0; i--) {
   if (maxValue[i] < 64) {
   value[i] = lowByte(message) & B00111111;
   R_IF_SERIAL_DEBUG(printf_P(PSTR("6B%u=%u ; "),i,value[i]));
   message >>= 6;
   }
   if (maxValue[i] > 63) {
   value[i] = lowByte(message);
   R_IF_SERIAL_DEBUG(printf_P(PSTR("8B%u=%u ; "),i,value[i]));
   message >>= 8;
   }
   }
   value[VALUE_ID_DISPO_TO_CONNECT] = value[VALUE_ID_DISPO];
   
   if (role == PLAYER && currentMenu != 101) {
   currentMenu = 101;
   count = 1;
   initMenu();
   }
   if (role == BASE && currentMenu != 102) {
   currentMenu = 102;
   count = 1;
   initMenu();
   }
   }
   }
   displayAllValue();
   if (value[VALUE_CONNECT] == 1) {
   digitalWrite(LED_CONNECT, HIGH);
   } else {
   digitalWrite(LED_CONNECT, LOW);
   }*/
  
}

void TheRemote::processValueFromPlayer() {
  value[VALUE_BATTERY] = 9;
  value[VALUE_VOLUME] = 15;
  value[VALUE_LANG] = 0;
  R_IF_SERIAL_DEBUG(printf_P(PSTR("simul connect \n\r")));
  
}

void TheRemote::processValueFromBase() {
  
}

// *************************************************************************************
//          LCD functions
// *************************************************************************************

void TheRemote::initLCD() {
  myLCD.begin(9600);
  delay(100);
  setSplashScreen();
  addAllCharToLCD();
  delay(50);
  R_IF_SERIAL_DEBUG(printf_P(PSTR("\n\r startup LCD \n\r")));
  
}

void TheRemote::selectLineOne(byte n) { //puts the cursor at line 1 char n.
  sendCommandLCD();
  myLCD.write((128 + n)); //position
  //R_IF_SERIAL_DEBUG(printf_P(PSTR("go pos %u \n\r"),n));
}

void TheRemote::selectLineTwo(byte n) { //puts the cursor at line 2 char n.
  sendCommandLCD();
  myLCD.write((192 + n)); //position
}

void TheRemote::clearLCD() {
  sendCommandLCD();
  myLCD.write(0x01); //clear command.
  delay(50);
}

void TheRemote::boxCursorLCD(boolean b) {
  sendCommandLCD();
  if (b) {
    myLCD.write(0x0D);
  } else {
    myLCD.write(0x0C);
  }
}

void TheRemote::setSplashScreen() {
  boxCursorLCD(false);
  myLCD.write(0x7C); //special command flag
  myLCD.write(0x09);
  clearLCD();
  selectLineOne(0);
  myLCD.write((byte) LCD_SYMBOL_CONNECT);
  myLCD.write((byte) LCD_SYMBOL_CONNECT);
  myLCD.print(" Verena Velvet");
  selectLineTwo(0);
  myLCD.print("entrechienetloup");
  delay(1000);
}

void TheRemote::addAllCharToLCD() {
  boxCursorLCD(false);
  byte character_data[8][8] = {
    { B00000, B01010, B11111, B11111, B11111,B01110, B00100, B00000 },
    { B00100, B01110, B10001, B11011, B01010,B00000, B01111, B11111 },
    { B01001, B10000, B10010, B01010, B00010,B01111, B11111, B11111 },
    { B00000, B00010, B00110, B11110, B11110,B00110, B00010, B00000 },
    { B00001, B00111, B01101, B01001, B01001,B01011, B11000, B00000 },
    { B01110, B11111, B10001, B10001, B10001,B10001, B10001, B11111 },
    { B00000, B01010, B11111, B11100, B10001,B00111, B01110, B00100 },
    { B10101, B00000, B00100, B01110, B10011,B10101, B10001, B01110 } };
  for (byte i = 0; i < 8; i++) {
    for (byte j = 0; j < 8; j++) {
      sendCommandLCD();
      myLCD.write(0x40 | (i << 3) | j); // Set the CGRAM address
      myLCD.write(character_data[i][j]); // Set the character
      myLCD.write(0x80); // Set the cursor back to DDRAM
    }
    delay(10);
    //clearLCD();
    //selectLineOne(i);
    //myLCD.write(i);
    //delay(10);
  }
}

//TODO with a pot to get one IOPin for LED

/*void TheRemote::checkLightOnLCD() {
 if (abs(analogRead(POT_LCD_LIGHT_ON) - lightValue) > 30) {
 byte l =
 (byte) (128 + map(analogRead(POT_LCD_LIGHT_ON), 0, 1024, 0, 29));
 l=157;//do not use pot
 R_IF_SERIAL_DEBUG(
 printf_P(PSTR("change light (%u,%u) -> %u\n\r"),lightValue,analogRead(POT_LCD_LIGHT_ON),l));
 lightValue = analogRead(POT_LCD_LIGHT_ON);
 myLCD.write(0x7C); //special command flag
 myLCD.write(l);
 
 }
 }*/

void TheRemote::sendCommandLCD() {
  myLCD.write(0xFE); // command flag
}

// *************************************************************************************
//          Rotary functions
// *************************************************************************************

void TheRemote::checkRotary() {
  while (changeA) {
    delay(2);
    // Test transition
    if (digitalRead(ROTARY_A) == digitalRead(ROTARY_B)) {
      encoder0Pos++;
    } else {
      encoder0Pos--;
    }
    changeA = false;
  }
  
  if (encoder0Pos != 0 && currentMenu!=103) {
    if (!editValue) {
      byte max=maxCursorPos;
      if(value[VALUE_CONNECT]==0) max=maxCursorPosDisconnect;
      count = (count + max + encoder0Pos) % max;
      if (value[VALUE_CONNECT]==1 && count==1 && currentMenu>100 ) {
        count = (count + max + encoder0Pos) % max;
      }
      selectLineOne(cursorPos[count]);
    } else {
      int d=0;
      if(currentMenu==102)d=150;
      value[getLink()] = ((value[getLink()]-d) + (maxValue[getLink()]-d)
                          + encoder0Pos) % (maxValue[getLink()]-d) + d;
      //if(currentMenu==102 && value[VALUE_ID_DISPO]<150)value[VALUE_ID_DISPO]=180;
      needRefresh = true;
      if(looseSync==0 && getLink() != VALUE_ID_DISPO) looseSync=1;
    }
    
    //R_IF_SERIAL_DEBUG(printf_P(PSTR("%u count %u inc %i max %u cursorpos %u and link %u\n\r"),currentMenu,count[currentMenu],encoder0Pos,maxCursorPos, cursorPos[count[currentMenu]], link[count[currentMenu]]));
    encoder0Pos = 0;
  }
}

void TheRemote::doEncoderA() {
  changeA = true;
  //R_IF_SERIAL_DEBUG(printf_P(PSTR("interA \n\r")));
}

// *************************************************************************************
//          Push functions
// *************************************************************************************

bool TheRemote::buttonRoutine() {
  buttonOK.update();
  buttonUPDATE.update();
  
  if (buttonOK.fallingEdge()) {
    if (connecting) return true;
    if (getLink() > 99) {
      //change menu
      R_IF_SERIAL_DEBUG(printf_P(PSTR("link to %u \n\r"),getLink()));
      byte lastMenu = currentMenu;
      currentMenu = getLink();
      if(currentMenu==102 && lastMenu==100)value[VALUE_ID_DISPO]=150;
      initMenu(lastMenu>105,currentMenu==100);
    } else {
      //on value
      if (!editValue) {
        editValue = true;
        boxCursorLCD(false);
        R_IF_SERIAL_DEBUG(printf_P(PSTR("edit value to %u \n\r"),getLink()));
      } else {
        editValue = false;
        if (getLink() == VALUE_DAY) eepromWrite(VALUE_DAY);
        if (getLink() == VALUE_ID_DISPO && !value[VALUE_CONNECT] == 1) {
          change595(PIN595_SYNCGREEN, 1);
          change595(PIN595_SYNCRED, 1);
          fire595();
          //connecting et try get value
          updateSendingAddress();
          getValue();
        }
        boxCursorLCD(true);
        selectLineOne(cursorPos[count]);
        R_IF_SERIAL_DEBUG(printf_P(PSTR("exit edit value to %u \n\r"),getLink()));
      }
    }
    return true;
  }
  
  if (buttonUPDATE.fallingEdge()) {
    R_IF_SERIAL_DEBUG(printf_P(PSTR("buttonUPDATE pressed \n\r")));
    //uptade value on player/base
    if (connecting) return true;
    //if(value[VALUE_CONNECT]==1 && currentMenu==104 && !editValue) {updateSendingAddress() ; sendID();return true;}
    if(value[VALUE_CONNECT]==1 && !editValue) {updateSendingAddress(); sendValue();}
  }
  return false;
}


// *************************************************************************************
//          74HC595 functions
// *************************************************************************************

void TheRemote::change595(byte pin, byte state){
  if (state == HIGH){
    byte t=B00000001<<pin;
    register595|=t;
  }
  if (state == LOW){
    byte t=B00000001<<pin;
    t=B11111111^t;
    register595&=t;
  }
  //printf(" - update register 595 to %u=", register595);
  //printf(BYTETOBINARYPATTERN,BYTETOBINARY(register595));
}

void TheRemote::fire595(){
  digitalWrite(PIN595CSN, LOW);
  //printf(" - update 595\n\r");
  SPI.transfer(register595);
  digitalWrite(PIN595CSN, HIGH);
}


// *************************************************************************************
//          card utility
// *************************************************************************************

byte TheRemote::getBatteryStatus() {
  delay(2);
  int bat = analogRead(MASTER_ABATT);
  R_IF_SERIAL_DEBUG(printf_P(PSTR("battery (v= x/157) analog read = %u \n\r"),bat));
  if (bat > MASTER_ABATT_5)
    return 5;
  if (bat > MASTER_ABATT_4)
    return 4;
  if (bat > MASTER_ABATT_3)
    return 3;
  if (bat > MASTER_ABATT_2)
    return 2;
  if (bat > MASTER_ABATT_1)
    return 1;
  return 0;
}


// *************************************************************************************
//          keypad
// *************************************************************************************

void TheRemote::checkSerialforkeypad(){
  if (Serial.available()){
    char c = (char)Serial.read();
    R_IF_SERIAL_DEBUG(printf_P(PSTR("keypad %s"),c));
    switch (c) {
      case 'A':
        //add a new id
        if(chosenAction==FRESHSTART && chosenModeForID!=RANGEID && countindex<5){
          chosenModeForID = ADDID;
          countindex++;
        }
        break;
      case 'B':
        //select the end of range
        if(chosenAction==FRESHSTART && chosenModeForID==FRESHSTART){
          chosenModeForID = RANGEID;
          countindex++;
        }
        break;
        
      case 'F':
        //select volume
        chosenAction = VOLUMESET;
        
        break;
      case 'E':
        //select language
        chosenAction = LANGUAGESET;
        
        break;
        
      case 'D':
        //correction
        switch (chosenAction) {
          case VOLUMESET:
            selectedV=-1;
            chosenAction=FRESHSTART;
            break;
            
          case LANGUAGESET:
            selectedL=-1;
            chosenAction=FRESHSTART;
            break;
            
          default:
            restartkeypad();
            break;
        }
        
        break;
        
      case 'C':
        //enter
        R_IF_SERIAL_DEBUG(printf_P(PSTR("ENTER !!")));
        if (chosenModeForID==RANGEID) {
          for (byte i = selectedID[0];i<=selectedID[1];i++){
            changeState(i);
          }
        }else{
          for (byte i=0; i<=countindex; i++) {
            changeState(selectedID[i]);
          }
        }
        restartkeypad();
        currentMenu==100;
        initMenu(false,true);
        break;
        
        
      default:
        //numeric
        if(c>='0' && c<='9'){
          c-=48;
          switch (chosenAction) {
            case FRESHSTART:
              if(selectedID[countindex]<100)selectedID[countindex]=selectedID[countindex]*10+c;
              break;
            case VOLUMESET:
              if (selectedV==-1)selectedV=0;
              if(selectedV<10)selectedV=selectedV*10+c;
              break;
            case LANGUAGESET:
              selectedL=c;
              break;
          }
          
          
          
        }
        break;
    }
    if(c!='C'){
      //draw on LCD
    clearLCD();
    boxCursorLCD(false);
    
    selectLineOne(0);
    myLCD.write(LCD_SYMBOL_PLAYER);
    myLCD.print(selectedID[0]);
    if (chosenModeForID==RANGEID) {
      myLCD.write("->");
      myLCD.print(selectedID[1]);
    }else{
      for (byte i=1; i<=countindex; i++) {
        myLCD.write("+");
        myLCD.print(selectedID[i]);
      }
    }
    
    selectLineTwo(0);
    myLCD.write(LCD_SYMBOL_VOLUME);
    if (selectedV>-1) {
      myLCD.print(selectedV);
    }else{
      myLCD.write("  ");
    }
    myLCD.write("  L");
    if (selectedL>-1) {
      writeL(selectedL);
    }
    }

    R_IF_SERIAL_DEBUG(printf_P(PSTR("state [%u,%u,%u,%u,%u] type%u action%u - vol%i lan%i \n\r"),selectedID[0],selectedID[1],selectedID[2],selectedID[3],selectedID[4],chosenModeForID,chosenAction,selectedV,selectedL));
    
  }
}


void TheRemote::restartkeypad(){
  //all value to 0
  for (byte i=0;i<10;i++){
    selectedID[i]=0;
  }
  selectedL=-1;
  selectedV=-1;
  chosenModeForID=FRESHSTART;
  chosenAction=FRESHSTART;
  countindex=0;
}

void TheRemote::changeState(byte idtoChange){
  //send info to an id player
  R_IF_SERIAL_DEBUG(printf_P(PSTR("change state %u"),idtoChange));
  currentMenu==101;
  initMenu(false,true);
  value[VALUE_ID_DISPO]=idtoChange;
  change595(PIN595_SYNCGREEN, 1);
  change595(PIN595_SYNCRED, 1);
  fire595();
  //connecting et try get value
  updateSendingAddress();
  getValue(false);
  //update the value if needed
  if (selectedV>-1) value[VALUE_VOLUME]=selectedV;
  if (selectedL>-1) value[VALUE_LANG]=selectedL;
  sendValue(false);
  value[VALUE_ID_DISPO]=0;
  updateSendingAddress();
}


// *************************************************************************************
//          EEPROM
// *************************************************************************************

//for day only


void TheRemote::eepromWrite(byte address) {
  eepromWrite(address,value[address]);
}

byte TheRemote::eepromRead(byte address) {
  return EEPROM.read(address);
}

void TheRemote::eepromWrite(byte address, byte data) {
  byte previous = eepromRead(address);
  if(data!=previous && address!=VALUE_BATTERY){
    R_IF_SERIAL_DEBUG(printf_P(PSTR("eeprom write : adress %u value %u -> %u\n\r"),address,previous,data));
    EEPROM.write(address, data);
  }
}
