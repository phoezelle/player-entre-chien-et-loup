#include "ThePlayer.h"
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

//Add the SdFat Libraries
#include <SdFat.h>
//#include <SdFatUtil.h>

//Add EEPROM librarie
#include <EEPROM.h>

//Add RF24 radio module Libraries
#include "nRF24L01.h"
#include "RF24.h"
  

#include <MemoryFree.h>

#include <avr/sleep.h>
#include <avr/wdt.h>

//For lighter program, if undef M_SERIAL_DEBUG, do not include debug code line.
//debug info
#undef M_SERIAL_DEBUG
#ifdef M_SERIAL_DEBUG
#define M_IF_SERIAL_DEBUG(x) ({x;})
#else
#define M_IF_SERIAL_DEBUG(x)
#endif

//For lighter program, if undef I_SERIAL_DEBUG, do not include info code line.
//user info
#define I_SERIAL_DEBUG
#ifdef I_SERIAL_DEBUG
#define I_IF_SERIAL_DEBUG(x) ({x;})
#else
#define I_IF_SERIAL_DEBUG(x)
#endif

/*//#ifndef NATIVE
#undef PROGMEM
#define PROGMEM __attribute__(( section(".progmem.data") ))
#undef PSTR
#define PSTR(s) (__extension__({static prog_char __c[] PROGMEM = (s); &__c[0];}))
//#endif*/

#define error(s) sd.errorHalt_P(PSTR(s))

// *************************************************************************************
//          Constructor, destructor and global variable
// *************************************************************************************
const uint64_t sendingPrefix = 0xF0FEE00000LL;

RF24 radio(MASTER_RF24_CE,MASTER_RF24_CS);/**< instance of RF24 class for manage the Nordic NLRF24plus*/


//prefix for adress
//player  : reading pipe 1 pref+id (response for telecomande)
//          writing pipe 1 pref+pos (talk to player)
//base    : reading pipe 1 pref+id (response for telecomande)
//          reading pipe 2 pref+pos (response for player)

ThePlayer::ThePlayer() {
	Serial.begin(115200);
}

ThePlayer::~ThePlayer() {
}

// *************************************************************************************
//          Routine to include to the loop function
// *************************************************************************************

void ThePlayer::routine() {
  
	//checkReceivedFromTelecomand();
	//serialDebug();
  
	if (value[ROLE] == PLAYER) {
    //play("l01_t01.ogg"); //forced testing play
    //play("l00_t00.mp3"); //forced testing play vs1033
    inPlayActions();
    if(goplay) playCurrentTrack();
    else {
      logInSD(2);
      play("end.mp3");
      minTimeToPlay=1;
    }
  }
  
  if (value[ROLE] == BASE) runBaseLoop(); //go in base mode
  
  if (value[ROLE] == PLAYER_AUTO) {
    goplay=false;
    play("auto.mp3");
    delay(1000);
  }
  
  if (value[ROLE] == PLAYER_CUSTOM){
    goplay=false;
    //niort device
    pinMode(MASTER_DEVICE, INPUT);
    delay(100);
    while(digitalRead(MASTER_DEVICE)==LOW){
      M_IF_SERIAL_DEBUG(printf_P(PSTR("wait\n\r"))); //waiting device HIGH
      delay(100);}
      
    //en niort device
    play("custom.mp3");
    delay(1000);
  }
  
  // listen to serial com, usefull for debugging
	/*if (value[ROLE] == PLAYER && value[OK_MP3] && value[OK_RF24]
   && value[OK_SD]) {
   //checkRadioForBase();
   //if (false)
   checkToPlayTrack();
   }
   /*if(value[ROLE]==BASE && value[OK_RF24]){
	 sendRadioForStation();
	 }*/
  
}

// *************************************************************************************
//          INIT functions
// *************************************************************************************

//big init for the whole player
void ThePlayer::init(byte initID) {
	//printf_begin();
  
  //setDefaultConfig();//for debugging use
  
  currentPlayingInfo[PLAYING_BASE_POSITION]=0;
  nextPlayingInfo[PLAYING_BASE_POSITION]=0;
  currentPlayingInfo[PLAYING_TRACK]=0;
  currentPlayingInfo[PLAYING_MIN_TIME]=30;
  minTimeToPlay=2*currentPlayingInfo[PLAYING_MIN_TIME];
  currentPlayingInfo[PLAYING_FADE_OUT_TIME]=1;
  nextTryPlaying[PLAYING_BASE_POSITION]=1;
  //tryConnectBase = true;
  playerMemory[0]=0;
  playerMemory[1]=0;
  playerMemory[2]=0;
  playerRegisseur = false;
  playerRegisseurSansOrdre = false;
  endFading=false;
  changingVolume = false;
  goplay = true;
  millisDerive=0;
  counterGoodMessage=0;
  delayTryConnectBase=1600;
  twoNextBaseConnect=true;
  twoNext=0;
	maxValue[VALUE_ID_DISPO] = MAX_VALUE_ID;
	maxValue[VALUE_VOLUME] = MAX_VALUE_VOLUME;
	maxValue[VALUE_LANG] = MAX_VALUE_LANG;
	maxValue[VALUE_TRACK] = MAX_VALUE_TRACK;
	maxValue[VALUE_MIN_TIME] = MAX_VALUE_MIN_TIME;
	maxValue[VALUE_FADE_OUT_TIME] = MAX_VALUE_FADE_OUT_TIME;
	maxValue[VALUE_BASE_POSITION] = MAX_VALUE_TRACK;
	maxValue[VALUE_BASE_POWER] = MAX_VALUE_BASE_POWER;
	maxValue[VALUE_BASE_CONSECUTIVE_TRANSMIT_TO_GO] = MAX_VALUE_BASE_CONSECUTIVE_TRANSMIT_TO_GO;
  maxValue[VALUE_BATTERY] = MAX_VALUE_BATTERY;
  maxValue[VALUE_DAY] = MAX_VALUE_DAY;
  
	delay(2);
	I_IF_SERIAL_DEBUG(printf_P(PSTR("VVdispo V2.1 24/04/16")));
  
	initPin();
	initRole();
	eepromReadAll();
  if (initID!=0){
    eepromWrite(VALUE_ID_DISPO,initID,true);
  }
  I_IF_SERIAL_DEBUG(printf_P(PSTR("ID %u\n\r"),value[VALUE_ID_DISPO]));
  M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rfree memory = %u \n\r"),freeMemory()););
  
	delay(2);
	if (value[ROLE] != BASE) {
		initSD();
		initMP3();
    while (!value[OK_SD] || !value[OK_MP3]) {baseSleepTotal();}
  }
	if (value[ROLE] == BASE || value[ROLE] == PLAYER) {
    initRF24();
    while (!value[OK_RF24]) {baseSleepTotal();}
  }
	
  tryConnectBaseTime=millis();
  
}

void ThePlayer::initPin() {
	pinMode(MP3_DREQ, INPUT);
	pinMode(MP3_XCS, OUTPUT);
	pinMode(MP3_XDCS, OUTPUT);
	pinMode(MP3_RESET, OUTPUT);
	//pinMode(MASTER_DEVICE, INPUT);
	pinMode(MASTER_BTEST, OUTPUT);
	pinMode(MASTER_RF24_CE, OUTPUT);
	pinMode(MASTER_RF24_CS, OUTPUT);
  
	digitalWrite(MP3_XCS, HIGH); //Deselect Control
	digitalWrite(MP3_XDCS, HIGH); //Deselect Data
	digitalWrite(MP3_RESET, LOW); //Put VS1053 into hardware reset
	digitalWrite(MASTER_BTEST, LOW);
}

void ThePlayer::initRole() {
	//M_IF_SERIAL_DEBUG(printf_P(PSTR("check analog switch\n\r")));
	byte b = 0;
	if (analogRead(MASTER_AROLE1) > 200)
		b += 1;
	if (analogRead(MASTER_AROLE2) > 200)
		b += 2;
	value[ROLE] = b;
  if (value[ROLE]==3)setDefaultConfig();
	I_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rROLE IS = %u (0p, 1b, 2pa, 3pc)\n\r"),value[ROLE], analogRead(MASTER_AROLE1), analogRead(MASTER_AROLE2))); //(0=player, 1=base, 2=playerauto, 3=playercustom)
}

void ThePlayer::initSD() {
	M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rinit SD card\n\r")));
	value[OK_SD] = 1;
  /*
	//Setup SD card interface
	if (!card.init(SPI_FULL_SPEED, SD_CS)) {
    err++;
	} //Initialize the SD card and configure the I/O pins.
	if (!volume.init(&card)) {
    err++;
	} //Initialize a volume on the SD card.
	if (!root.openRoot(&volume)) {
    err++;
	}//Open the root directory in the volume.
   */
  if (!sd.begin(SD_CS,SPI_FULL_SPEED)) {
    //sd.initErrorHalt();
    value[OK_SD]=0;
  }
  

	I_IF_SERIAL_DEBUG(printf_P(PSTR("init uSDcard %s\n\r"), value[OK_SD] ? "OK" : "PROBLEM"););// 1 volume ; 2 I/O on SD ; 3 init
  logInSD(0);
}

void ThePlayer::initMP3() {
	delay(20);
  value[OK_MP3] = 1;
	M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rinit MP3 chip\n\r")));
	//We have no need to setup SPI for VS1053 because this has already been done by the SDfatlib
	//From page 12 of datasheet, max SCI reads are CLKI/7. Input clock is 12.288MHz.
	//Internal clock multiplier is 1.0x after power up.
	//Therefore, max SPI speed is 1.75MHz. We will use 1MHz to be safe.
	SPI.setClockDivider(SPI_CLOCK_DIV8); //Set SPI bus speed to 1MHz (8MHz / 8 = 1MHz)
	SPI.transfer(0xFF); //Throw a dummy byte at the bus
  //Initialize VS1053 chip
	digitalWrite(MP3_RESET, HIGH); //Bring up VS1053
  delay(20); //We don't need this delay because any register changes will check for a high DREQ
  
  currentVolume=(15-value[VALUE_VOLUME])*5;
  if(value[ROLE] != PLAYER)currentVolume=10;
	Mp3SetVolume(currentVolume,currentVolume); //Set initialvolume (20 = -10dB) Manageable
  
	//Let's check the status of the VS1053
	int MP3Mode = Mp3ReadRegister(SCI_MODE);
	if (MP3Mode == 0x0000)value[OK_MP3] = 0;
	int MP3Status = Mp3ReadRegister(SCI_STATUS);
	int MP3Clock = Mp3ReadRegister(SCI_CLOCKF);
  
	int vsVersion = (MP3Status >> 4) & 0x000F; //Mask out only the four version bits
	M_IF_SERIAL_DEBUG(printf_P(PSTR("SCI_Mode (0x4800) = 0x%x\n\rSCI_Status (0x48) = 0x%x\n\rVS Version (VS1033 is 5) = %u\n\r"),MP3Mode,MP3Status,vsVersion););
  delay(2);
	//Now that we have the VS1053 up and running, increase the internal clock multiplier and up our SPI rate
	Mp3WriteRegister(SCI_CLOCKF, 0xe0, 0x00); //Set multiplier to 5.0x
  
	//From page 12 of datasheet, max SCI reads are CLKI/7. Input clock is 12.288MHz.
	//Internal clock multiplier is now 5x.
	//Therefore, max SPI speed is 8,7MHz. 8MHz will be safe.
	SPI.setClockDivider(SPI_FULL_SPEED); //Set SPI bus speed to 8MHz
	MP3Clock = Mp3ReadRegister(SCI_CLOCKF);
	//M_IF_SERIAL_DEBUG(printf_P(PSTR("SCI_ClockF = 0x%x\n\r"),MP3Clock););
  
	playing = false;
	I_IF_SERIAL_DEBUG(printf_P(PSTR("init MP3 %s\n\r"), value[OK_MP3] ? "OK" : "PROBLEM"););
  
}

void ThePlayer::initRF24() {
	value[OK_RF24] = 0;
	M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rinit RF24 chip\n\r")));
	//radio.setPin(MASTER_RF24_CE, MASTER_RF24_CS);
	radio.begin();
	radio.setRetries(1,3); // 3 try delay 500us each
	radio.setPayloadSize(8);
	radio.enableAckPayload();
	radio.setChannel(125);
	//radio.setDataRate(RF24_250KBPS);
	setRF24Power();
  updateSendingAddress();
  updateListeningAddress();
	if (radio.isPVariant())value[OK_RF24] = 1;
	updateAckforRemote();
	setAckforRemote();
	//radio.printDetails();
	radio.startListening();
	I_IF_SERIAL_DEBUG(printf_P(PSTR("init RF24 %s\n\r"), value[OK_RF24] ? "OK" : "PROBLEM"););
}


// *************************************************************************************
//          RF COMUNICATIONs functions
// *************************************************************************************




void ThePlayer::playerTryToConnectBase(){
  /*if(!playerRegisseurSansOrdre){
    if(twoNextBaseConnect && minTimePassedGoCheckTwoNextBase && nextTryPlaying[PLAYING_BASE_POSITION]==currentPlayingInfo[PLAYING_TRACK]+1){
      if (twoNext==0) twoNext=1;
      else twoNext=0;
    }
    I_IF_SERIAL_DEBUG(printf_P(PSTR("%u "),nextTryPlaying[PLAYING_BASE_POSITION]+twoNext));
    M_IF_SERIAL_DEBUG(if(minTimePassedGoCheckTwoNextBase)printf_P(PSTR(" + %u"),twoNext));
    M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\r")));
    updateSendingAddress();
    radio.stopListening();
    uint64_t message = 0;
    addIndexedValueToMessage(message,VALUE_ID_DISPO);
    byte code = CODE_DISPO_TRY_CONNECT;
    addValueToMessage(message,code,4);
    radio.write(&message,sizeof(message));
    radio.startListening();
  }*/
  
  radio.startListening();
}


byte ThePlayer::checkIncommingRF(){
  if(radio.available()){
    M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rRF message ")));
    uint64_t received = 0;
    //M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rfree memory = %u \n\r"),freeMemory()););
    boolean done=false;
    int unlockCount=0;
    while (!done && unlockCount<20) {
      //M_IF_SERIAL_DEBUG(printf_P(PSTR(".")));
      // Fetch the payload, and see if this was the last one.
      done = radio.read(&received, sizeof(received));
      unlockCount++;
      //delay(20);//debug
    }
    M_IF_SERIAL_DEBUG(printf_P(PSTR("[%u] - "),unlockCount));
    if (unlockCount>18){
      M_IF_SERIAL_DEBUG(printf_P(PSTR("BAD RF\n\r")));
      radio.powerDown();
      delay(10);
      radio.startListening();
      return 0;

    }
    //printMessage(received);
    byte code = getValueFromMessage(received,4);
    M_IF_SERIAL_DEBUG(printf_P(PSTR("code %X - "),code));
    //message from remote
    if(code == CODE_SET_VALUE || code == CODE_SET_ID || code == CODE_GET_VALUE || code == CODE_GET_MEMORY){
      M_IF_SERIAL_DEBUG(printf_P(PSTR("remote message")));
        if(value[ROLE] == BASE) activateBase();  //restart timer for sleeping base
        if (code == CODE_SET_VALUE || code == CODE_SET_ID) {
          //we need to check the message
          if(lastReceivedMessage==received)counterGoodMessage++;else counterGoodMessage=0;
          M_IF_SERIAL_DEBUG(printf_P(PSTR(" (good = %u)\n\r"),counterGoodMessage));
          lastReceivedMessage=received;
          //get 5 times the same message => ok.
          if(counterGoodMessage==5){
            counterGoodMessage=0;
            for (byte i = NB_VALUE_STORED; i > 0; i--) {
              byte j=i-1;
              if(j!=VALUE_ID_DISPO && code==CODE_SET_VALUE){
                getIndexedValueFromMessage(received,j);
                eepromWrite(j);
              }else{
                getIndexedValueFromMessageWithoutStore(received,j);
              }
            }
            updateAckforRemote(CODE_ACK_RESPONSE_SET_OK);
          }
        }
        else {
          if (code==CODE_GET_MEMORY)updateAckforRemoteForMemory(CODE_ACK_RESPONSE_MEMORY); else updateAckforRemote();
        }
      setAckforRemote();
    }else{
      //message from player
      if(value[ROLE]==BASE  && code == CODE_DISPO_TRY_CONNECT){
        M_IF_SERIAL_DEBUG(printf_P(PSTR("player message\n\r")));
        byte IDtoAdd = getIndexedValueFromMessageWithoutStore(received,VALUE_ID_DISPO);
        return IDtoAdd;
      }
      //message from base
      if(value[ROLE]==PLAYER) {
        byte receivedPlayingInfo[NB_INFO_TRACK];
        receivedPlayingInfo[PLAYING_BASE_POSITION] = getIndexedValueFromMessageWithoutStore(received,VALUE_BASE_POSITION);
        receivedPlayingInfo[PLAYING_FADE_OUT_TIME] = getIndexedValueFromMessageWithoutStore(received,VALUE_FADE_OUT_TIME);
        receivedPlayingInfo[PLAYING_MIN_TIME] = getIndexedValueFromMessageWithoutStore(received,VALUE_MIN_TIME);
        receivedPlayingInfo[PLAYING_TRACK] = getIndexedValueFromMessageWithoutStore(received,VALUE_TRACK);
        receivedPlayingInfo[PLAYING_ID_BASE] = getIndexedValueFromMessageWithoutStore(received,VALUE_ID_DISPO);
        M_IF_SERIAL_DEBUG(printf_P(PSTR("base message : rbp=%u (wbp=%u%s) - "),receivedPlayingInfo[PLAYING_BASE_POSITION],nextTryPlaying[PLAYING_BASE_POSITION], twoNextBaseConnect && minTimePassedGoCheckTwoNextBase ? " and +1" : ""));
        
        bool conditional = (nextTryPlaying[PLAYING_BASE_POSITION] == receivedPlayingInfo[PLAYING_BASE_POSITION]||(nextTryPlaying[PLAYING_BASE_POSITION]+1 == receivedPlayingInfo[PLAYING_BASE_POSITION] && twoNextBaseConnect && minTimePassedGoCheckTwoNextBase)||(playerRegisseurSansOrdre && nextPlayingInfo[PLAYING_BASE_POSITION] != receivedPlayingInfo[PLAYING_BASE_POSITION]));
        
        if (nextPlayingInfo[PLAYING_BASE_POSITION] == receivedPlayingInfo[PLAYING_BASE_POSITION]){
          M_IF_SERIAL_DEBUG(printf_P(PSTR("last base say ok\n\r")));
          setAckforBase(CODE_ACK_PLAYER_GO);
        }
        if (code == CODE_BASE_SAY_OK && conditional) {
          M_IF_SERIAL_DEBUG(printf_P(PSTR("base say ")));
          setAckforBase(CODE_ACK_PLAYER_GO);
          for (byte i=0; i<NB_INFO_TRACK; i++) {
            nextPlayingInfo[i]=receivedPlayingInfo[i];
          }
          nextTryPlaying[PLAYING_BASE_POSITION]=nextPlayingInfo[PLAYING_BASE_POSITION]+1;
          I_IF_SERIAL_DEBUG(printf_P(PSTR("go next%u - want track%u\n\r"),nextPlayingInfo[PLAYING_BASE_POSITION],nextPlayingInfo[PLAYING_TRACK]));
          twoNext=0;
          //tryConnectBase=true;
          //save info to nextPlayingInfo and enable try player for next base
        }
        if (code == CODE_BASE_TRY_CONNECT && conditional) {
          //tryConnectBase=false;
          tryConnectBaseTime=millis();
          delayTryConnectBase=10000;
          M_IF_SERIAL_DEBUG(printf_P(PSTR("base try\n\r")));
          setAckforBase(CODE_ACK_PLAYER_WAITING);
          //disable try player
        }
      }
    }
  }
  
  return 0;
}


void ThePlayer::updateSendingAddress(byte suffix){
  if(value[ROLE]==PLAYER){
    uint64_t address = sendingPrefix | (nextTryPlaying[PLAYING_BASE_POSITION]+BASE_POSITION_RF_ADRESS_OFFSET+twoNext);
    radio.openWritingPipe(address);
    //M_IF_SERIAL_DEBUG(printf_P(PSTR("update writting address to sufix %u\n\r"),lowByte(address)));
  }
  if(value[ROLE]==BASE){
    uint64_t address = sendingPrefix | suffix;
    radio.openWritingPipe(address);
    //M_IF_SERIAL_DEBUG(printf_P(PSTR("update writting adress to sufix %u\n\r"),lowByte(address)));
  }
}

void ThePlayer::updateListeningAddress(){
  uint64_t address = sendingPrefix | value[VALUE_ID_DISPO];
  radio.openReadingPipe(1, address);
  //M_IF_SERIAL_DEBUG(printf_P(PSTR("update listening adress 1 to sufix %u\n\r"),lowByte(address)));
  if(value[ROLE]==BASE){
    address = sendingPrefix | (value[VALUE_BASE_POSITION]+BASE_POSITION_RF_ADRESS_OFFSET);
    radio.openReadingPipe(2, address);
    //M_IF_SERIAL_DEBUG(printf_P(PSTR("update listening adress 2 to sufix %u\n\r"),lowByte(address)));
  }
  radio.startListening();
}

//put ACK reply in the RF chip stack
void ThePlayer::setAckforRemote() {
  M_IF_SERIAL_DEBUG(printf_P(PSTR("ACK for remote\n\r")));
  radio.writeAckPayload(1, &ackT, sizeof(ackT));
}

//put ACK reply in the RF chip stack + update reply whith the good code for base
void ThePlayer::setAckforBase(byte code) {
  M_IF_SERIAL_DEBUG(printf_P(PSTR("ACK for base\n\r")));
  uint64_t ack = 0 | code;
  radio.writeAckPayload(1, &ack, sizeof(ack));
}

void ThePlayer::setRF24Power() {
  radio.stopListening();
  M_IF_SERIAL_DEBUG(printf_P(PSTR("set RFpow=%u \n\r"),value[VALUE_BASE_POWER]));
  switch (value[VALUE_BASE_POWER]) {
    case 0:
      radio.setPALevel(RF24_PA_MIN);
      break;
    case 1:
      radio.setPALevel(RF24_PA_LOW);
      break;
    case 2:
      radio.setPALevel(RF24_PA_HIGH);
      break;
    default:
      radio.setPALevel(RF24_PA_MAX);
      break;
  }
  radio.startListening();
}

//update ACK reply after remote change on the value
void ThePlayer::updateAckforRemote(byte code) {
  M_IF_SERIAL_DEBUG(printf_P(PSTR("update ACK code %X\n\r"),code));
  //ack to reply to remote
  ackT = 0;
  byte countM = 0;
  value[VALUE_BATTERY]=getBatteryStatus();
  for (byte i = 0; i < NB_VALUE_STORED; i++) {
    countM+=addIndexedValueToMessage(ackT,i);
    //printMessage(ackT);
  }
  addValueToMessage(ackT,value[ROLE],2);//printMessage(ackT);
  addValueToMessage(ackT,value[OK_SD],1);//printMessage(ackT);
  addValueToMessage(ackT,value[OK_MP3],1);//printMessage(ackT);
  addValueToMessage(ackT,value[OK_RF24],1);//printMessage(ackT);
  addValueToMessage(ackT,code,4);
  //printMessage(ackT);
  M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\r")));
  //printMessage(ackT);
  M_IF_SERIAL_DEBUG(printf_P(PSTR(" ACK message size : %u with code %x\n\r"),countM, code));
}

//update ACK reply after remote change on the value
void ThePlayer::updateAckforRemoteForMemory(byte code) {
  M_IF_SERIAL_DEBUG(printf_P(PSTR("update ACK code %X\n\r"),code));
  //ack to reply to remote
  ackT = 0;
  addValueToMessage(ackT,playerMemory[2],8);//printMessage(ackT);
  addValueToMessage(ackT,playerMemory[1],8);
  addValueToMessage(ackT,playerMemory[0],8);
  addValueToMessage(ackT,code,4);
  //printMessage(ackT);
  M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\r")));
  //printMessage(ackT);
  M_IF_SERIAL_DEBUG(printf_P(PSTR(" ACK memory with code %x\n\r"), code));
}





void ThePlayer::printfByte(uint8_t b){
  M_IF_SERIAL_DEBUG(printf_P(PSTR(BYTETOBINARYPATTERN),BYTETOBINARY(b)));
}

void ThePlayer::printMessage(uint64_t &m){
  
  for (byte i=0; i<sizeof(m); i++) {
    printfByte(lowByte(m>>((sizeof(m)-i-1)*8)));
    M_IF_SERIAL_DEBUG(printf_P(PSTR(".")));
  }
  M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\r")));
}



byte ThePlayer::addValueToMessage(uint64_t &message, byte &thevalue,byte sizeInBit,bool shift){
  if(shift)message <<= sizeInBit;
  message |= thevalue & (0xFF>>(8-sizeInBit));
  //M_IF_SERIAL_DEBUG(printf_P(PSTR("(%uB)no index value %u ; "),sizeInBit,lowByte(message)&(0xFF>>(8-sizeInBit))));
  return sizeInBit;
}

byte ThePlayer::addIndexedValueToMessage(uint64_t &message, byte index,bool shift){
  byte nbit=1;
  uint8_t r=maxValue[index];
  while (r>>=1!=0x00)nbit++;
  if(shift)message <<= nbit;
  message |= value[index]& (0xFF>>(8-nbit));
  //M_IF_SERIAL_DEBUG(printf_P(PSTR("(%uB)index %u value %u-%u ; "),nbit,index,value[index], lowByte(message)&(0xFF>>(8-nbit))));
  return nbit;
}


byte ThePlayer::getValueFromMessage(uint64_t &message, byte sizeInBit,bool shift){
  byte thevalue=lowByte(message)&(0xFF>>(8-sizeInBit));
  //M_IF_SERIAL_DEBUG(printf_P(PSTR("(%uB)get no index value %u ; "),sizeInBit,thevalue));
  if(shift)message >>= sizeInBit;
  return thevalue;
}

byte ThePlayer::getIndexedValueFromMessage(uint64_t &message, byte index,bool shift){
  byte nbit=1;
  uint8_t r=maxValue[index];
  while (r>>=1!=0x00)nbit++;
  value[index] = lowByte(message)&(0xFF>>(8-nbit));
  //M_IF_SERIAL_DEBUG(printf_P(PSTR("(%uB)get index %u value %u ; "),nbit,index,value[index]));
  if(shift)message >>= nbit;
  return value[index];
}

byte ThePlayer::getIndexedValueFromMessageWithoutStore(uint64_t &message, byte index){
  byte nbit=1;
  uint8_t r=maxValue[index];
  while (r>>=1!=0x00)nbit++;
  byte v = lowByte(message)&(0xFF>>(8-nbit));
  //M_IF_SERIAL_DEBUG(printf_P(PSTR("(%uB)get index %u value %u no store; "),nbit,index,value[index]));
  message >>= nbit;
  return v;
}










// *************************************************************************************
//          PLAYER Functions
// *************************************************************************************

void ThePlayer::playCurrentTrack() {
  char trackName[] = "l00_t00.mp3";
  sprintf(trackName, "l%02d_t%02d.mp3", value[VALUE_LANG],currentPlayingInfo[PLAYING_TRACK]);
  goplay=false;
  playingStartTime = millis();
  currentVolume=(15-value[VALUE_VOLUME])*5;
  Mp3SetVolume(currentVolume,currentVolume);
  if(currentPlayingInfo[PLAYING_TRACK]<8)playerMemory[0]|= 1<<currentPlayingInfo[PLAYING_TRACK];
  if(currentPlayingInfo[PLAYING_TRACK]>=8 && currentPlayingInfo[PLAYING_TRACK]<16)playerMemory[1]|= 1<<(currentPlayingInfo[PLAYING_TRACK]-8);
  if(currentPlayingInfo[PLAYING_TRACK]>=16 && currentPlayingInfo[PLAYING_TRACK]<24)playerMemory[1]|= 1<<(currentPlayingInfo[PLAYING_TRACK]-16);
  logInSD();
  play(trackName);
}

void ThePlayer::logInSD(byte start){
  M_IF_SERIAL_DEBUG(printf_P(PSTR("Start loging day %u"),value[VALUE_DAY]));
  char name[] = "LOG.TXT";
  ofstream logfile(name, ios::out | ios::app);
  //if (!logfile) error("open failed");
  if(start==0){
    int bat = getAnalogBatteryValue();
    logfile << endl << endl << "ON ID=" << int(value[VALUE_ID_DISPO]) << " day=" << int(value[VALUE_DAY]) << " batt=" << bat <<"	";
  }
  else {
    logfile <<"["<< value[VALUE_LANG] << "]" << int((unsigned long)(millis()/60000))<<"'"<< int((unsigned long)(millis()%60000)/1000)<<" : " ;
    if(start==1){
      if(tempsIntersceneJoue!=0)logfile <<"(+" << tempsIntersceneJoue <<"s) ";
      logfile << int(currentPlayingInfo[PLAYING_TRACK]) << "	- ";
    }
    else logfile << "end	- ";
  }
  tempsIntersceneJoue=0;
  logfile.close();
  M_IF_SERIAL_DEBUG(printf_P(PSTR("- ok\n\r")));
}

void ThePlayer::play(char* fileName) {
  if (!track.open(fileName, O_READ)) { //Open the file in read mode.
    I_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rerror with : %s"),fileName););
    delay(1000); //for serial overflow problem
    return;
  }
  playing = true;
  minTimePassedGoCheckTwoNextBase = false;
  
  I_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rTrack %s open - min time %u - "),fileName,minTimeToPlay););
  
  //track.read(mp3DataBuffer, sizeof(mp3DataBuffer)); //Read the first 32 bytes of the song
  int need_data = TRUE;
  
  M_IF_SERIAL_DEBUG(printf_P(PSTR("Start MP3 decoding\n\r")));
  
  while (1) {
    while (!digitalRead(MP3_DREQ)) {
      //take time to do stuff
      if(value[ROLE] == PLAYER)inPlayActions();
    }
    if (feedBuffer(need_data) == 0 || goplay)
      break;
    
  }
  track.close();
  playing = false;
  startPlayGoHome = TRUE;
  
}


byte ThePlayer::feedBuffer(int &need_data) {
  uint8_t mp3DataBuffer[32]; //Buffer of 32 bytes. VS1053 can take 32 bytes at a go.
  while (!digitalRead(MP3_DREQ)) {
    Serial.print(".");
  }
  //Once DREQ is released (high) we now feed 32 bytes of data to the VS1053 from our SD read buffer
  if (need_data == TRUE) { //This is here in case we haven't had any free time to load new data
    if (!track.read(mp3DataBuffer, sizeof(mp3DataBuffer))) { //Go out to SD card and try reading 32 new bytes of the song
      //Oh no! There is no data left to read!
      return 0;
    }
    need_data = FALSE;
  }
  digitalWrite(MP3_XDCS, LOW); //Select Data
  for (int y = 0; y < sizeof(mp3DataBuffer); y++) {
    SPI.transfer(mp3DataBuffer[y]); // Send SPI byte
    
  }
  //M_IF_SERIAL_DEBUG(printf_P(PSTR("free memory = %u \n\r"),freeMemory()););
  digitalWrite(MP3_XDCS, HIGH); //Deselect Data
  need_data = TRUE; //We've just dumped 32 bytes into VS1053 so our SD read buffer is empty. Set flag so we go get more data
  return 1;
}

void ThePlayer::stop() {
  while(!digitalRead(MP3_DREQ)) ; //Wait for DREQ to go high indicating transfer is complete
  digitalWrite(MP3_XDCS, HIGH); //Deselect Data
  Mp3WriteRegister(SCI_MODE, 0x08, 0x08);
  track.close(); //Close out this track
  while (1) {
    int MP3Mode = Mp3ReadRegister(SCI_MODE);
    int vsCancel = (MP3Mode >> 3) & 0x0001;
    if(vsCancel==0){
      break;
    }
  }
  delay(25);
}

void ThePlayer::changeVolume(){
  if ((unsigned long)(millis()-changeVolumeStartTime)>(unsigned long)(changeVolumeCount*changeVolumeTimeIncrement)) {
    changeVolumeCount++;
    if (currentVolume>volumeToReach) currentVolume--; else currentVolume++;
    Mp3SetVolume(currentVolume,currentVolume);
  }
  if (currentVolume==volumeToReach) {
    changingVolume=false;
    /*stop();
     delay(2000);
     play("l01_t01.mp3");*/
  }
}
void ThePlayer::startChangeVolume(int volume){
  changeVolumeCount=0;
  changingVolume = true;
  if (volume > 0) changeVolumeTimeIncrement = 50;
  else changeVolumeTimeIncrement = 8 * currentPlayingInfo[PLAYING_FADE_OUT_TIME];
  volumeToReach = (14 - volume) * 5;
  changeVolumeStartTime=millis();
  M_IF_SERIAL_DEBUG(printf_P(PSTR("start changing volume -%u -> -%u timeinc:%u time %us \n\r"),currentVolume,volumeToReach,changeVolumeTimeIncrement,currentPlayingInfo[PLAYING_FADE_OUT_TIME]));
}

void ThePlayer::inPlayActions() {
  checkIncommingRF();
  if (changingVolume) changeVolume();
  if(/*tryConnectBase &&*/ (unsigned long)(millis()-tryConnectBaseTime)>delayTryConnectBase){
    //send message to base in non blocking mode
    delayTryConnectBase=1600;
    tryConnectBaseTime=millis();
    playerTryToConnectBase();
  }
  checkToPlayTrack();
}


void ThePlayer::checkToPlayTrack(bool print) {
  
  int timeOfPlay = (unsigned long)(millis()-playingStartTime)/1000;
  if((timeOfPlay > minTimeToPlay || playerRegisseur) && !minTimePassedGoCheckTwoNextBase && twoNextBaseConnect){
    minTimePassedGoCheckTwoNextBase=true;
    I_IF_SERIAL_DEBUG(printf_P(PSTR("end min time to play (+1) \n\r")));
  }
  if (currentPlayingInfo[PLAYING_BASE_POSITION] != nextPlayingInfo[PLAYING_BASE_POSITION]){
    //if(print)M_IF_SERIAL_DEBUG(printf_P(PSTR(" - scheck next %u: time is %u (min %u) \n\r"),nextPlayingInfo[PLAYING_TRACK],timeOfPlay,minTimeToPlay));
    if(timeOfPlay > minTimeToPlay && !changingVolume && !endFading){
      M_IF_SERIAL_DEBUG(printf_P(PSTR("go next %u: time is %u (min %u) \n\r"),nextPlayingInfo[PLAYING_TRACK],timeOfPlay,minTimeToPlay));
      if(timeOfPlay > minTimeToPlay)tempsIntersceneJoue=timeOfPlay-minTimeToPlay;
      startChangeVolume(-16);
      minTimeToPlay=timeOfPlay+currentPlayingInfo[PLAYING_FADE_OUT_TIME];
      endFading=true;
    }
    if (!playing || (timeOfPlay > minTimeToPlay || playerRegisseur||playerRegisseurSansOrdre)) {
      M_IF_SERIAL_DEBUG(printf_P(PSTR("endfading \n\r")));
      if(playing)stop();
      for (byte i=0; i<NB_INFO_TRACK; i++) {
        currentPlayingInfo[i]=nextPlayingInfo[i];
      }
      endFading=false;
      changingVolume=false;
      minTimeToPlay=2*currentPlayingInfo[PLAYING_MIN_TIME];
      currentVolume=(15-value[VALUE_VOLUME])*5;
      Mp3SetVolume(currentVolume,currentVolume); 
      goplay=true;
    }
  }
}




// *************************************************************************************
//          BASE FUNCTION
// *************************************************************************************

void ThePlayer::runBaseLoop(){

  I_IF_SERIAL_DEBUG(printf_P(PSTR("\n\r ENTERING BASE LOOP MODE\n\r")));
  
  const byte player_ammount = 101;
  millisDerive=0;
  
  //byte playerID_to_send[player_ammount];
  byte playerID_to_send_count[player_ammount];
  for (uint8_t i = 0; i<player_ammount; i++) {
    //playerID_to_send[i]=0;
    playerID_to_send_count[i]=0;
  }
  byte count251 = 0;
  byte count252 = 0;
  byte count253 = 0;
  
  //puts telecommand ID
  //playerID_to_send[0]=251;
  //playerID_to_send[1]=252;
  //playerID_to_send[2]=253;
  
  //off mp3 chip
  digitalWrite(MP3_RESET, LOW);
  M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\r(free memory = %u) - "),freeMemory()););
  //entering the loop
  while (1){

    unsigned long startTime = millisCorrected();
    radio.stopListening();
    
    //make message
    //M_IF_SERIAL_DEBUG(printf_P(PSTR("%u ms - build message\n\r"),(unsigned long)millisCorrected()-startTime));
    uint64_t message=0;
    byte countM=0;
    countM+= addIndexedValueToMessage(message,VALUE_ID_DISPO);
    countM+= addIndexedValueToMessage(message,VALUE_TRACK);
    countM+= addIndexedValueToMessage(message,VALUE_MIN_TIME);
    countM+= addIndexedValueToMessage(message,VALUE_FADE_OUT_TIME);
    countM+= addIndexedValueToMessage(message,VALUE_BASE_POSITION);
    //M_IF_SERIAL_DEBUG(printf_P(PSTR(" / size of message : %u bits without 4bits code\n\r"),countM));
    
    //sending phase
    //M_IF_SERIAL_DEBUG(printf_P(PSTR("%u ms - start sending for players\n\r"),millis()-startTime));
    for (uint8_t i = 0; i < player_ammount; i++) {
      playerID_to_send_count[i]=tryconnectPlayer(i,playerID_to_send_count[i],message);
    }
    count251=tryconnectPlayer(251,count251,message);
    count252=tryconnectPlayer(252,count252,message);
    count253=tryconnectPlayer(253,count253,message);
    
    //delay (2000 to test)
    //M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\rpause\n\r")));
    //delay(2000);
    //startTime+=2000;
    
    
    //M_IF_SERIAL_DEBUG(printf_P(PSTR("%u ms sending phase\n\r"),millisCorrected()-startTime));
    startTime = millisCorrected();
    radio.startListening();
    while ((unsigned long)(millisCorrected()-startTime) < 200) {
      byte IDtoAdd = checkIncommingRF();
      /*if (IDtoAdd !=0){
        M_IF_SERIAL_DEBUG(printf_P(PSTR("adding player %u\n\r"),IDtoAdd));
        for (uint8_t i = 0; i < player_ammount; i++) {
          
          if(playerID_to_send[i]==IDtoAdd)i=player_ammount;
          if(playerID_to_send[i]==0){
            playerID_to_send[i]=IDtoAdd;
            playerID_to_send_count[i]=0;
            break;
          }
        }
      }*/
      
    }
    //M_IF_SERIAL_DEBUG(printf_P(PSTR("%u ms listening phase\n\r"),millisCorrected()-startTime));
    M_IF_SERIAL_DEBUG(printf_P(PSTR("END SEND PLAYER\n\r")));
    
    if ((unsigned long)(millisCorrected()%60000)<600) {
      timeStartOnBase++;
      M_IF_SERIAL_DEBUG(printf_P(PSTR("--> %u min  check sleepTotal\n\r\n\r"),timeStartOnBase));
    }
    if(timeStartOnBase > TIME_BEFORE_BASE_SLEEP)baseSleepTotal();
  }
}


byte ThePlayer::tryconnectPlayer(byte playerID, byte count, uint64_t &message){
if (playerID !=0){
  delay(1);
  updateSendingAddress(playerID);
  byte code=0;
  if(count >= value[VALUE_BASE_CONSECUTIVE_TRANSMIT_TO_GO])code = CODE_BASE_SAY_OK;
    else code = CODE_BASE_TRY_CONNECT;
      addValueToMessage(message,code,4);
      //printMessage(message);
      
      boolean connect = radio.write(&message, sizeof(message));
      message>>=4; //restart code for message.
      byte result = 0;
      //M_IF_SERIAL_DEBUG(printf_P(PSTR("->%s"), connect ? "OK" : "FAIL"););
      //sending ok now check the ACK
      if(connect){
        if (playerID>250)result = CODE_ACK_PLAYER_WAITING;
          //delay(1);
          I_IF_SERIAL_DEBUG(printf_P(PSTR("[%u - %i ->%X] "),playerID,count,code));
          //M_IF_SERIAL_DEBUG(printf_P(PSTR("%u ms - check ACK payload \n\r"),millis()-startTime));
          if (radio.isAckPayloadAvailable()) {
            uint64_t received=0;
            radio.read(&received, sizeof(received));
            result=lowByte(received)&0x0F;
            M_IF_SERIAL_DEBUG(printf_P(PSTR(" ACK %X "),result));
          }
      }
  //process ACK code
  bool forget=false;
  switch (result) {
    case CODE_ACK_PLAYER_WAITING :
      if(count<0)count=0;
        if(count<25)count+=1;//success for transmission add ++ to count
          break;
    case CODE_ACK_PLAYER_GO :
      if(playerID<250){ //if not remote forget this player
        I_IF_SERIAL_DEBUG(printf_P(PSTR("go saved ")));
        forget=true;
      }
      break;
    default:
      //if (count<=0) {
        //if(count>-10)count-=1;
         // }else{
            count=0;
          //}
     // if(count<-8  && playerID<250){
       // forget=true;
     // }
      break;
  }
  if (forget){
    M_IF_SERIAL_DEBUG(printf_P(PSTR(" reset count "),playerID));
    count=0;
    //playerID_to_send[i]=0;
  }
  I_IF_SERIAL_DEBUG(if(connect)printf_P(PSTR(" / ")));
}
  return count;
}

unsigned long ThePlayer::millisCorrected(){
  return millis()+millisDerive;
}


void ThePlayer::baseSleepTotal(){
  M_IF_SERIAL_DEBUG(printf_P(PSTR("   \n\r turnoff all\n\r")));
  delay(3);
  //turn off mp3
  digitalWrite(MP3_RESET, LOW);
  //turn off RF24
  radio.powerDown();
  
  // disable ADC
  byte old_ADCSRA = ADCSRA;
  ADCSRA = 0;
  
  baseSleep();
  
  // cancel sleep as a precaution
  sleep_disable();
  ADCSRA = old_ADCSRA;
  
  //turn on RF24
  radio.powerUp();
  delay(10);
  
  
}

void ThePlayer::baseSleep(){
  millisDerive+=8000;
  Serial.end();
  // clear various "reset" flags
  MCUSR = 0;
  // allow changes, disable reset
  // set interrupt mode and an interval
  // set WDIE, and 32ms second delay _BV (WDP3)|
  wdt_enable(9);
  WDTCSR |= (1 << WDIE);
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_cpu ();
  
  Serial.begin(115200);
  
}

void ThePlayer::activateBase(){
  if(timeStartOnBase > TIME_BEFORE_BASE_SLEEP){
    timeStartOnBase=0;
    asm volatile ("  jmp 0");
  }
  
}






// *************************************************************************************
//          EEPROM READ AN WRITE + MAJ
// *************************************************************************************

void ThePlayer::eepromReadAll() {
  for (byte i = 0; i < NB_VALUE_STORED-1; i++) {
    value[i] = eepromRead(i);
    if(i==VALUE_VOLUME && value[i]!=DEFAULT_VOLUME) eepromWrite(VALUE_VOLUME, DEFAULT_VOLUME,true);
    if(i==VALUE_LANG && value[i]>=1 && value[i]<=3) {
      if(value[VALUE_ID_DISPO]<31 && value[i]!=1)eepromWrite(VALUE_LANG, 1,true);
      if(value[VALUE_ID_DISPO]>30 && value[VALUE_ID_DISPO]<61 && value[i]!=2)eepromWrite(VALUE_LANG, 2,true);
      if(value[VALUE_ID_DISPO]>60 && value[VALUE_ID_DISPO]<110 && value[i]!=3)eepromWrite(VALUE_LANG, 3,true);
    }
    M_IF_SERIAL_DEBUG(printf_P(PSTR("eeprom read : %u on index %u\n\r"),value[i],i));
    if (i == VALUE_ID_DISPO && (value[VALUE_ID_DISPO] == 255 || value[VALUE_ID_DISPO] == 0)) {
      setDefaultConfig();
      break;
    }
  }
}

void ThePlayer::setDefaultConfig() {
  M_IF_SERIAL_DEBUG(printf_P(PSTR("\n\r ****** first compil on card !! \n\r")));
  eepromWrite(VALUE_ID_DISPO, 1,true);
  eepromWrite(VALUE_VOLUME, 12,true);
  eepromWrite(VALUE_LANG, 0,true);
  eepromWrite(VALUE_TRACK, 1,true);
  eepromWrite(VALUE_BASE_POSITION, 5,true);
  eepromWrite(VALUE_MIN_TIME, 30,true);
  eepromWrite(VALUE_FADE_OUT_TIME, 2,true);
  eepromWrite(VALUE_BASE_POWER, 3,true);
  eepromWrite(VALUE_BASE_CONSECUTIVE_TRANSMIT_TO_GO, 6,true);
  value[VALUE_BATTERY]=0;
}

byte ThePlayer::eepromRead(byte address) {
  return EEPROM.read(address);
}

void ThePlayer::eepromWrite(byte address, byte data, bool init) {
  byte previous = eepromRead(address);
  if(data!=previous && address!=VALUE_BATTERY){
    I_IF_SERIAL_DEBUG(printf_P(PSTR("eeprom write : adress %u value %u -> %u\n\r"),address,previous,data));
    EEPROM.write(address, data);
    if(!init){
      switch (address) {
        case VALUE_ID_DISPO:
          updateListeningAddress();
          break;
        case VALUE_VOLUME:
          
          //Mp3SetVolume(data,data);
          startChangeVolume(data);
          break;
        case VALUE_BASE_POSITION:
          updateListeningAddress();
          break;
        case VALUE_BASE_POWER:
          setRF24Power();
          break;
        default:
          break;
      }
    }
  }
}

void ThePlayer::eepromWrite(byte address) {
  eepromWrite(address,value[address]);
}

// *************************************************************************************
//          MP3 UTILS
// *************************************************************************************
void ThePlayer::Mp3WriteRegister(unsigned char addressbyte,
                                 unsigned char highbyte, unsigned char lowbyte) {
  while (!digitalRead(MP3_DREQ))
    ; //Wait for DREQ to go high indicating IC is available
  digitalWrite(MP3_XCS, LOW); //Select control
  
  //SCI consists of instruction byte, address byte, and 16-bit data word.
  SPI.transfer(0x02); //Write instruction
  SPI.transfer(addressbyte);
  SPI.transfer(highbyte);
  SPI.transfer(lowbyte);
  while (!digitalRead(MP3_DREQ))
    ; //Wait for DREQ to go high indicating command is complete
  digitalWrite(MP3_XCS, HIGH); //Deselect Control
}

//Read the 16-bit value of a VS10xx register
unsigned int ThePlayer::Mp3ReadRegister(unsigned char addressbyte) {
  while (!digitalRead(MP3_DREQ))
    ; //Wait for DREQ to go high indicating IC is available
  digitalWrite(MP3_XCS, LOW); //Select control
  
  //SCI consists of instruction byte, address byte, and 16-bit data word.
  SPI.transfer(0x03);  //Read instruction
  SPI.transfer(addressbyte);
  
  char response1 = SPI.transfer(0xFF); //Read the first byte
  while (!digitalRead(MP3_DREQ))
    ; //Wait for DREQ to go high indicating command is complete
  char response2 = SPI.transfer(0xFF); //Read the second byte
  while (!digitalRead(MP3_DREQ))
    ; //Wait for DREQ to go high indicating command is complete
  
  digitalWrite(MP3_XCS, HIGH); //Deselect Control
  
  int resultvalue = response1 << 8;
  resultvalue |= response2;
  return resultvalue;
}

//Set VS10xx Volume Register


void ThePlayer::Mp3SetVolume(unsigned char leftchannel,
                             unsigned char rightchannel) {
  Mp3WriteRegister(SCI_VOL, leftchannel, rightchannel);
  M_IF_SERIAL_DEBUG(
                    printf_P(PSTR(" [set volume : -%udB] "),leftchannel));
}

// *************************************************************************************
//          card utility
// *************************************************************************************

int ThePlayer::getAnalogBatteryValue(){
  digitalWrite(MASTER_BTEST, HIGH);
  delay(20);
  int bat = analogRead(MASTER_ABATT);
  digitalWrite(MASTER_BTEST, LOW);
  return bat;
}

byte ThePlayer::getBatteryStatus() {
  int bat = getAnalogBatteryValue();
  M_IF_SERIAL_DEBUG(printf_P(PSTR("battery (v= x/157) analog read = %u \n\r"),bat));
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
