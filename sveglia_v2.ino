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

const uint8_t sVISTA = 1;
const uint8_t sONOFF = 2;
const uint8_t sIMPOSTAORA = 3;
const uint8_t sIMPOSTAALLARME = 4;
const uint8_t sSPLASH = 5;

uint8_t statoSveglia = sSPLASH;

const uint8_t sNOSUONO = 1;
const uint8_t sSUONA = 2;
const uint8_t sSPENTA = 3;

uint8_t statoSuonoSveglia = sNOSUONO;

const uint8_t sORESVEGLIA = 1;
const uint8_t sMINUTISVEGLIA = 2;

uint8_t statoImpostaOra = sORESVEGLIA;

const uint8_t sOREALLARME = 1;
const uint8_t sMINUTIALLARME = 2;

uint8_t statoImpostaAllarme = sOREALLARME;

const uint8_t sSHOWOFF = 1;
const uint8_t sSHOWON = 2;
const uint8_t sSHOWALARM = 3;

uint8_t statoOnOff = sSHOWOFF;

// APP VERSION 
const uint8_t APP_VERSION = 3;

long counter = 0;
char charsDisplay[50];
bool enableAlarm = false;
int oreTMPALLARME = 0;
int minutiTMPALLARME = 0;
int oreTMPSVEGLIA = 0;
int minutiTMPSVEGLIA = 0;
String alarmTime = "00.00";

// EEPROM
int addressCheckEEPROM = 200;       // tells that I've wrote something before into the EEPROM
char writed_eeprom = "S";           // Char to write into addressCheckEEPROM

int addressTimeAlarm = 0;           // ADDRESS of ALARM
int addressStatusAlarm = 210;       // ADDRESS of enable/disable
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

  switch(statoSveglia) {
    case sVISTA: {
        if(counter >= 1000) {
          Time now = rtc.getTime();
          sprintf(charsDisplay, "%02d.%02d", now.hour, now.min);
          counter = 0;

          if(enableAlarm) {
            switch (statoSuonoSveglia) {
              case sNOSUONO: {
                  if(String(charsDisplay).equals(alarmTime)) {
                    statoSuonoSveglia = sSUONA;
                  }
                }
                break;
              case sSUONA: {
                  startAlarmSound();
                }            
                break;
              case sSPENTA: {
                  if(!String(charsDisplay).equals(alarmTime)) {
                    Serial.println("Alarm ready for next round...");
                    statoSuonoSveglia = sNOSUONO;
                  }
                }
                break;
            }
          }
        }
      }
      break;
    case sONOFF: {
        switch (statoOnOff) {
          case sSHOWOFF: {
              if(counter >= 3000) {
                counter = 0;
                statoSveglia = sVISTA;

                Serial.println("Sveglia OFF");
              } else {
                sprintf(charsDisplay, " OFF");
              }
            }
            break;
          case sSHOWON: {
              if(counter == 1500) {
                counter = 0;
                statoOnOff = sSHOWALARM;

                Serial.println("Sveglia ON");
              } else {
                sprintf(charsDisplay, " ON ");
              }
            }
            break;
          case sSHOWALARM: {
              if(counter >= 3000) {
                counter = 0;
                statoSveglia = sVISTA;

                Serial.print("Sveglia alle ");
                Serial.println(alarmTime);
              } else {
                alarmTime.toCharArray(charsDisplay, alarmTime.length() + 1);
              }
            }
            break;

        }
      }
      break;
    case sIMPOSTAALLARME: {
        switch (statoImpostaAllarme) {
          case sOREALLARME: {
              if(counter > 5000) {
                statoImpostaAllarme = sMINUTIALLARME;
                counter = 0;
                
                Serial.println("Imposto ora i minuti...");
              } else {
                sprintf(charsDisplay, " %02d ", oreTMPALLARME);
              }
            }
            break;
          case sMINUTIALLARME: {
              if(counter > 5000) {
                Serial.println("Imposto l'allarme...");

                impostaAllarme();
                enableAlarm = true;
                writeStringToEEPROM(addressTimeAlarm, alarmTime);
                EEPROM.put(addressStatusAlarm, enableAlarm);

                confirmSound();
          
                statoSveglia = sVISTA;
                counter = 0;
              } else {
                sprintf(charsDisplay, " %02d ", minutiTMPALLARME);
              }
            }
            break;
        }
      }
      break;
    case sIMPOSTAORA: {
        switch (statoImpostaOra) {
          case sORESVEGLIA: {
              if(counter > 5000) {
                statoImpostaOra = sMINUTISVEGLIA;
                counter = 0;
                
                Serial.println("Imposto ora i minuti...");
              } else {
                sprintf(charsDisplay, " %02d ", oreTMPSVEGLIA);
              }
            }
            break;
          case sMINUTISVEGLIA: {
              if(counter > 5000) {
                Serial.println("Imposto l'orario...");

                impostaOra();

                confirmSound();
          
                statoSveglia = sVISTA;
                counter = 0;
              } else {
                sprintf(charsDisplay, " %02d ", minutiTMPSVEGLIA);
              }
            }
            break;
        }
      }
      break;
    case sSPLASH: {
        if(counter >= 3000) {
          statoSveglia = sVISTA;
        } else {
          sprintf(charsDisplay, "V. %02d", APP_VERSION);
        }
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

  btnTime.onHold(3000, onBtnTimeHold);
  btnAlarm.onHold(3000, onBtnAlarmHold);
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
  sevseg.setBrightness(100);
}





void onBtnTimePressed(Button& btn) {
  //Serial.println("On btn 1...");

  switch(statoSveglia) {
    case sVISTA: {
        switch (statoSuonoSveglia) {
          case sSUONA:
            statoSuonoSveglia = sSPENTA;
            Serial.println("Alarm stopped");
            break;
          
          default:
            // niente per ora...
            break;
        }
      } 
      break;
    // increse HOUR or MINUTES for ALLARM
    case sIMPOSTAALLARME: {
        switch (statoImpostaAllarme) {
          case sOREALLARME: {
              counter = 0;
              increaseHour(&oreTMPALLARME);
            }
            break;
          case sMINUTIALLARME: {
              counter = 0;
              increaseMinutes(&minutiTMPALLARME);
            }
            break;
        }
      }
      break;
    // increse HOUR or MINUTES for TIME
    case sIMPOSTAORA: {
        switch (statoImpostaOra) {
          case sORESVEGLIA: {
              counter = 0;
              increaseHour(&oreTMPSVEGLIA);
            }
            break;
          case sMINUTISVEGLIA: {
              counter = 0;
              increaseMinutes(&minutiTMPSVEGLIA);
            }
            break;
        }
      }
      break;
  }
}

void onBtnAlarmPressed(Button& btn) {
  Serial.println("On btn 2...");

  switch(statoSveglia) {
    case sONOFF: {
        switch (statoOnOff) {
          case sSHOWALARM:
          case sSHOWON: {
              counter = 0;
              enableAlarm = false;
              EEPROM.put(addressStatusAlarm, enableAlarm);    // save EEPROM only when switch
              statoOnOff = sSHOWOFF;
            }
            break;
          case sSHOWOFF: {
              counter = 0;
              enableAlarm = true;
              EEPROM.put(addressStatusAlarm, enableAlarm);    // save EEPROM only when switch
              statoOnOff = sSHOWON;
            }
            break;
        }

        Serial.print("Alarm status: ");
        Serial.println(enableAlarm);
      }
      break;
    case sVISTA: {
        switch (statoSuonoSveglia) {
            case sSUONA:
              statoSuonoSveglia = sSPENTA;
              Serial.println("Alarm stopped");
              break;
        }

        counter = 0;
        if(enableAlarm) statoOnOff = sSHOWON;
        else statoOnOff = sSHOWOFF;
        statoSveglia = sONOFF;
      }
      break;
    // decrease HOUR or MINUTES for ALLARM
    case sIMPOSTAALLARME: {
        switch (statoImpostaAllarme) {
          case sOREALLARME: {
              counter = 0;
              decreaseHour(&oreTMPALLARME);
            }
            break;
          case sMINUTIALLARME: {
              counter = 0;
              decreaseMinutes(&minutiTMPALLARME);
            }
            break;
        }
      }
      break;
    // decrease HOUR or MINUTES for TIME
    case sIMPOSTAORA: {
        switch (statoImpostaOra) {
          case sORESVEGLIA: {
              counter = 0;
              decreaseHour(&oreTMPSVEGLIA);
            }
            break;
          case sMINUTISVEGLIA: {
              counter = 0;
              decreaseMinutes(&minutiTMPSVEGLIA);
            }
            break;
        }
      }
      break;
  }
}

void onBtnTimeHold(Button& btn, uint16_t timeHold) {
  Serial.println("Set RTC...");
  switch(statoSveglia) {
    case sVISTA: {
        Time now = rtc.getTime();
        oreTMPSVEGLIA = now.hour;
        minutiTMPSVEGLIA = now.min;

        counter = 0;
        statoImpostaOra = sORESVEGLIA;
        statoSveglia = sIMPOSTAORA;

        Serial.println("Imposta ora le ore...");
      }
      break;
  }
}

void onBtnAlarmHold(Button& btn, uint16_t timeHold) {
  Serial.println("Set Alarm...");

  switch(statoSveglia) {
    case sONOFF: 
    case sVISTA: {
        counter = 0;
        statoImpostaAllarme = sOREALLARME;
        statoSveglia = sIMPOSTAALLARME;

        Serial.println("Imposta ora le ore...");
      }
      break;
  }
}

void startAlarmSound() {
  tone(buzzerPin, 1000, 500);
  Serial.println("SVEGLIA !!!!");
}

void impostaAllarme() {
  char tmpAlarm[5];
  sprintf(tmpAlarm, "%02d.%02d", oreTMPALLARME, minutiTMPALLARME);
  alarmTime = String(tmpAlarm);

  Serial.print("Sveglia impostata e salvata alle ");
  Serial.println(alarmTime);
}

void impostaOra() {
  rtc.setTime(oreTMPSVEGLIA, minutiTMPSVEGLIA, 0);

  Serial.print("Orario impostato!");
}

void confirmSound() {
  tone(buzzerPin, 1000, 300);
  delay(500);
  tone(buzzerPin, 800, 300);
}


void checkEEPROM() {
  char tmpCheckWrited;
  EEPROM.get(addressCheckEEPROM, tmpCheckWrited);
  if(tmpCheckWrited == writed_eeprom) {
    eeprom_first_save = false;
    alarmTime = readStringFromEEPROM(addressTimeAlarm);
    EEPROM.get(addressStatusAlarm, enableAlarm);
    Serial.print("Alarm read from EEPROM ");
    Serial.print(alarmTime);
    Serial.print(" status ");
    Serial.println(enableAlarm);
  } else {
    Serial.print("Char S not found but ");
    Serial.println(tmpCheckWrited);
  }
}

void writeStringToEEPROM(int addrOffset, const String &strToWrite)
{
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++)
  {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }

  if(eeprom_first_save) {
    EEPROM.put(addressCheckEEPROM, writed_eeprom);
  }
}

String readStringFromEEPROM(int addrOffset)
{
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++)
  {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  //char needed for string comparison
  data[newStrLen] = '\0';
  return String(data);
}





/*
  Increase the hour passed in by 1.
*/
void increaseHour(int *hour){
  if(*hour == 23) *hour = 0;
  else *hour = *hour + 1;
}

/*
  Increase the minutes passed in by 1.
*/
void increaseMinutes(int *minutes){
  if(*minutes == 59) *minutes = 0;
  else *minutes = *minutes + 1;
}

/*
  Decrease the hour passed in by 1.
*/
void decreaseHour(int *hour){
  if(*hour == 0) *hour = 23;
  else *hour = *hour - 1;
}

/*
  Decrease the minutes passed in by 1.
*/
void decreaseMinutes(int *minutes){
  if(*minutes == 0) *minutes = 59;
  else *minutes = *minutes - 1;
}