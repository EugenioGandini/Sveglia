#include "SevSeg.h"
#include <DS1302.h>
#include <Button.h>
#include <ButtonEventCallback.h>
#include <BasicButton.h>
#include <EEPROM.h>

#define buzzerPin A3

SevSeg sevseg;  
DS1302 rtc(A0, A1, A2);
BasicButton btnTime = BasicButton(A4);
BasicButton btnAlarm = BasicButton(A5);


// MAIN STATE MACHINE
const uint8_t sVIEW = 1;
const uint8_t sONOFF = 2;
const uint8_t sSETTIME = 3;
const uint8_t sSETALARM = 4;
const uint8_t sSPLASH = 5;

uint8_t statusStateAlarm = sSPLASH;

// SUB STATE MACHINE sVIEW
const uint8_t sNOSOUND = 1;
const uint8_t sSOUND = 2;
const uint8_t sSOUNDOFF = 3;

uint8_t statusAlarmSound = sNOSOUND;

// SUB STATE MACHINE sSETTIME
const uint8_t sHOURSTIME = 1;
const uint8_t sMINUTESTIME = 2;

uint8_t statusSetTime = sHOURSTIME;

// SUB STATE MACHINE sSETALARM
const uint8_t sHOURSALARM = 1;
const uint8_t sMINUTESALARM = 2;

uint8_t stateSetAlarm = sHOURSALARM;

// SUB STATE MACHINE sONOFF
const uint8_t sSHOWOFF = 1;
const uint8_t sSHOWON = 2;
const uint8_t sSHOWALARM = 3;

uint8_t stateOnOff = sSHOWOFF;

// APP VERSION 
const uint8_t APP_VERSION = 3;

long counter = 0;
char charsDisplay[10];
bool enableAlarm = false;

// values (HH:MM) for the alarm
uint8_t hourAlarm = 0;
uint8_t minAlarm = 0;

// values (HH:MM) for the time
uint8_t hourTime = 0;
uint8_t minTime = 0;

// EEPROM
int addressCheckEEPROM = 200;       // tells that I've wrote something before into the EEPROM
const char DATA_SAVED = "S";        // Char that sign something has been saved before into EEPROM

int addressHourAlarm = 0;           // ADDRESS of hour alarm
int addressMinAlarm = 1;            // ADDRESS of minutes alarm
int addressStatusAlarm = 2;         // ADDRESS of enable/disable
bool eeprom_first_save = true;      // flag to disable if EEPROM was previously wrote into.






/////////// SETUP ///////////
void setup() {
  // Set serial comm (debug)
  Serial.begin(9600);

  initButtons();
  initBuzzer();
  initRTC();
  initDisplay();

  checkEEPROM();
}

/////////// LOOP ///////////
void loop() {
  btnTime.update();  
  btnAlarm.update();

  switch(statusStateAlarm) {
    case sVIEW:
      if(counter >= 1000) {
        Time now = rtc.getTime();
        hourTime = now.hour;
        minTime = now.min;
        sprintf(charsDisplay, "%02d.%02d", hourTime, minTime);
        counter = 0;

        if(enableAlarm) {
          switch (statusAlarmSound) {
            case sNOSOUND:
              if(hourTime == hourAlarm && minTime == minAlarm) {
                statusAlarmSound = sSOUND;
              }
              break;

            case sSOUND:
              startAlarmSound();
              break;

            case sSOUNDOFF:
              if(hourTime != hourAlarm || minTime != minAlarm) {
                Serial.println("Alarm ready for next round...");
                statusAlarmSound = sNOSOUND;
              }
              break;

          }
        }
      }
      break;

    case sONOFF:
      switch (stateOnOff) {
        case sSHOWOFF:
          if(counter >= 3000) {
            counter = 0;
            statusStateAlarm = sVIEW;

            Serial.println("Alarm OFF");
          } else {
            sprintf(charsDisplay, " OFF");
          }
          break;

        case sSHOWON:
          if(counter == 1500) {
            counter = 0;
            stateOnOff = sSHOWALARM;

            Serial.println("Alarm ON");
          } else {
            sprintf(charsDisplay, " ON ");
          }
          break;

        case sSHOWALARM:
          if(counter >= 3000) {
            counter = 0;
            statusStateAlarm = sVIEW;

            Serial.println(String("Alarm at ") + hourAlarm + ":" + minAlarm);
          } else {
            sprintf(charsDisplay, "%02d.%02d", hourAlarm, minAlarm);
          }
          break;

      }
      break;

    case sSETALARM:
      switch (stateSetAlarm) {
        case sHOURSALARM:
          if(counter > 5000) {
            stateSetAlarm = sMINUTESALARM;
            counter = 0;
            
            Serial.println("Set now the minutes...");
          } else {
            sprintf(charsDisplay, " %02d ", hourAlarm);
          }
          break;

        case sMINUTESALARM:
          if(counter > 5000) {
            Serial.println("Setting the alarm...");

            setAllarm();

            confirmSound();
      
            statusStateAlarm = sVIEW;
            counter = 0;
          } else {
            sprintf(charsDisplay, " %02d ", minAlarm);
          }
          break;

      }
      break;

    case sSETTIME:
      switch (statusSetTime) {
        case sHOURSTIME:
          if(counter > 5000) {
            statusSetTime = sMINUTESTIME;
            counter = 0;
            
            Serial.println("Set now the minutes...");
          } else {
            sprintf(charsDisplay, " %02d ", hourTime);
          }
          break;

        case sMINUTESTIME:
          if(counter > 5000) {
            Serial.println("Setting the time...");

            setTime();

            confirmSound();
      
            statusStateAlarm = sVIEW;
            counter = 0;
          } else {
            sprintf(charsDisplay, " %02d ", minTime);
          }
          break;

      }
      break;

    case sSPLASH:
      if(counter >= 3000) {
        statusStateAlarm = sVIEW;
      } else {
        sprintf(charsDisplay, "V. %02d", APP_VERSION);
      }
      break;
      
  }

  sevseg.setChars(charsDisplay);
  sevseg.refreshDisplay();

  counter++;
  delay(1);
}







/////////////////////////////////////
//        INIT FUNCTIONS
/////////////////////////////////////

/**
 * Init RTC module and retrieve first time.
 */
void initRTC() {
  rtc.halt(false);
  rtc.writeProtect(false);
}

/**
 * Init Buzzer module.
 */
void initBuzzer() {
  pinMode(buzzerPin, OUTPUT);
}

/**
 * Init buttons.
 */
void initButtons() {  
  btnTime.onPress(onBtnTimePressed);
  btnAlarm.onPress(onBtnAlarmPressed);

  btnTime.onHold(5000, onBtnTimeHold);
  btnAlarm.onHold(5000, onBtnAlarmHold);
}

/**
 * Init 7 segment 4 digit module display
 */
void initDisplay() {
  byte numDigits = 4;
  byte digitPins[] = {10, 11, 12, 13};
  byte segmentPins[] = {9, 2, 3, 5, 6, 8, 7, 4};

  bool resistorsOnSegments = true; 
  bool updateWithDelaysIn = true;
  byte hardwareConfig = COMMON_CATHODE; 
  sevseg.begin(hardwareConfig, numDigits, digitPins, segmentPins, resistorsOnSegments);
  sevseg.setBrightness(10);
}




/**
 * Callback when btn time is pressed
 */
void onBtnTimePressed(Button& btn) {
  Serial.println("On btn time...");

  switch(statusStateAlarm) {
    case sVIEW:
      switch (statusAlarmSound) {
        case sSOUND:
          statusAlarmSound = sSOUNDOFF;
          Serial.println("Alarm stopped");
          break;
        
        default:
          // nothing for now...
          break;
      }
      break;
    
    // increse HOUR or MINUTES for ALLARM
    case sSETALARM:
      switch (stateSetAlarm) {
        case sHOURSALARM: {
            counter = 0;
            increaseHour(&hourAlarm);
          }
          break;
        case sMINUTESALARM: {
            counter = 0;
            increaseMinutes(&minAlarm);
          }
          break;
      }
      break;

    // increse HOUR or MINUTES for TIME
    case sSETTIME:
      switch (statusSetTime) {
        case sHOURSTIME: {
            counter = 0;
            increaseHour(&hourTime);
          }
          break;
        case sMINUTESTIME: {
            counter = 0;
            increaseMinutes(&minTime);
          }
          break;
      }
      break;

  }
}

/**
 * Callback called when btn alarm is pressed
 */
void onBtnAlarmPressed(Button& btn) {
  Serial.println("On btn alarm...");

  switch(statusStateAlarm) {
    case sONOFF:
      switch (stateOnOff) {
        case sSHOWALARM:
        case sSHOWON: {
            counter = 0;
            enableAlarm = false;
            EEPROM.put(addressStatusAlarm, enableAlarm);    // save EEPROM only when switch
            stateOnOff = sSHOWOFF;
          }
          break;
        case sSHOWOFF: {
            counter = 0;
            enableAlarm = true;
            EEPROM.put(addressStatusAlarm, enableAlarm);    // save EEPROM only when switch
            stateOnOff = sSHOWON;
          }
          break;
      }

      Serial.println(String("Alarm status: ") + enableAlarm);
      break;

    case sVIEW:
      switch (statusAlarmSound) {
          case sSOUND:
            statusAlarmSound = sSOUNDOFF;
            Serial.println("Alarm stopped");
            break;
      }

      counter = 0;
      if(enableAlarm) stateOnOff = sSHOWON;
      else stateOnOff = sSHOWOFF;
      statusStateAlarm = sONOFF;
      break;

    // decrease HOUR or MINUTES for ALLARM
    case sSETALARM:
      switch (stateSetAlarm) {
        case sHOURSALARM:
          counter = 0;
          decreaseHour(&hourAlarm);
          break;

        case sMINUTESALARM:
          counter = 0;
          decreaseMinutes(&minAlarm);
          break;

      }
      break;

    // decrease HOUR or MINUTES for TIME
    case sSETTIME:
      switch (statusSetTime) {
        case sHOURSTIME:
          counter = 0;
          decreaseHour(&hourTime);
          break;

        case sMINUTESTIME:
          counter = 0;
          decreaseMinutes(&minTime);
          break;

      }
      break;

  }
}

/**
 * Callback when long pressing time btn.
 */
void onBtnTimeHold(Button& btn, uint16_t timeHold) {
  Serial.println("Set RTC...");
  switch(statusStateAlarm) {
    case sVIEW: {
        counter = 0;
        statusSetTime = sHOURSTIME;
        statusStateAlarm = sSETTIME;

        Serial.println("Set now the hours...");
      }
      break;
  }
}

/**
 * Callback when long pressing alarm btn.
 */
void onBtnAlarmHold(Button& btn, uint16_t timeHold) {
  Serial.println("Set Alarm...");

  switch(statusStateAlarm) {
    case sONOFF: 
    case sVIEW: {
        counter = 0;
        stateSetAlarm = sHOURSALARM;
        statusStateAlarm = sSETALARM;

        Serial.println("Set now the hours...");
      }
      break;
  }
}

/**
 * Start sound
 */
void startAlarmSound() {
  tone(buzzerPin, 1000, 500);
  Serial.println("WAKE UP !!!!");
}

/**
 * Set the alarm time.
 */
void setAllarm() {
  if(!enableAlarm) enableAlarm = true;

  EEPROM.put(addressHourAlarm, hourAlarm);
  EEPROM.put(addressMinAlarm, minAlarm);
  EEPROM.put(addressStatusAlarm, enableAlarm);

  Serial.println(String("Alarm set and saved at ") + hourAlarm + ":" + minAlarm);
}

/**
 * Set the actual time.
 */
void setTime() {
  rtc.setTime(hourTime, minTime, 0);

  Serial.print("Set time completed!");
}

/**
 * Confirm sound played for confirmating some operation...
 */
void confirmSound() {
  tone(buzzerPin, 1000, 300);
  delay(500);
  tone(buzzerPin, 800, 300);
}






/**
 * Check if in the EEPROM is saved something...
 * If yes load all the data.
 */
void checkEEPROM() {
  char tmpCheckWrited;
  EEPROM.get(addressCheckEEPROM, tmpCheckWrited);

  if(tmpCheckWrited == DATA_SAVED) {
    eeprom_first_save = false;

    EEPROM.get(addressStatusAlarm, enableAlarm);
    EEPROM.get(addressHourAlarm, hourAlarm);
    EEPROM.get(addressMinAlarm, minAlarm);

    Serial.println(String("Alarm read from EEPROM ") + hourAlarm + ":" + minAlarm + " status " + enableAlarm);
  } else {
    Serial.println(String("Char S not found but ") + tmpCheckWrited);

    initEEPROM();
  }
}

/**
 * Init the EEPROM at first boot
 */
void initEEPROM() {
  EEPROM.put(addressHourAlarm, hourAlarm);
  EEPROM.put(addressMinAlarm, minAlarm);
  EEPROM.put(addressStatusAlarm, enableAlarm);
  EEPROM.put(addressCheckEEPROM, DATA_SAVED);
}




/*
  Increase the hour passed in by 1.
*/
void increaseHour(uint8_t *hour){
  if(*hour == 23) *hour = 0;
  else *hour = *hour + 1;
}

/*
  Increase the minutes passed in by 1.
*/
void increaseMinutes(uint8_t *minutes){
  if(*minutes == 59) *minutes = 0;
  else *minutes = *minutes + 1;
}

/*
  Decrease the hour passed in by 1.
*/
void decreaseHour(uint8_t *hour){
  if(*hour == 0) *hour = 23;
  else *hour = *hour - 1;
}

/*
  Decrease the minutes passed in by 1.
*/
void decreaseMinutes(uint8_t *minutes){
  if(*minutes == 0) *minutes = 59;
  else *minutes = *minutes - 1;
}