#include "SevSeg.h"
#include <DS1302.h>
#include <Button.h>
#include <ButtonEventCallback.h>
#include <BasicButton.h>

#define buzzerPin A3

SevSeg sevseg;  
DS1302 rtc(A0, A1, A2);
BasicButton btnTime = BasicButton(A4);
BasicButton btnAlarm = BasicButton(A5);

const uint8_t sVISTA = 1;
const uint8_t sONOFF = 2;
const uint8_t sIMPOSTAORA = 3;
const uint8_t sIMPOSTAALLARME = 4;

uint8_t statoSveglia = sVISTA;

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

int counter = 0;
char charsDisplay[5];
bool enableAlarm = false;
int oreTMPALLARME = 0;
int minutiTMPALLARME = 0;
int oreTMPSVEGLIA = 0;
int minutiTMPSVEGLIA = 0;
String alarmTime = "00.00";





/////////// SETUP ///////////
void setup() {
  initButtons();
  initBuzzer();
  initRTC();
  initDisplay();

  // Set serial comm (debug)
  Serial.begin(9600);
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
                statoSveglia = sVISTA;
                counter = 0;

                Serial.println("Sveglia OFF");
              } else {
                sprintf(charsDisplay, " OFF");
              }
            }
            break;
          case sSHOWON: {
              if(counter >= 1500) {
                statoSveglia = sVISTA;
                counter = 0;
                // statoOnOff = sSHOWALARM;
                // counter = 0;

                Serial.println("Sveglia ON");
              } else {
                sprintf(charsDisplay, " ON ");
              }
            }
            break;
          case sSHOWALARM: {
              if(counter >= 3000) {
                statoSveglia = sVISTA;
                counter = 0;

                Serial.print("Sveglia alle ");
                Serial.println(alarmTime);
              } else {
                //char tmpShow[5];
                //alarmTime.toCharArray(tmpShow, 6);
                //sprintf(charsDisplay, "%s", alarmTime);
                sprintf(charsDisplay, "%02d.%02d", oreTMPALLARME, oreTMPALLARME);
                //alarmTime.toCharArray(charsDisplay, 6);
                Serial.print("[");
                Serial.print(counter);
                Serial.println("]");
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
  }

  sevseg.setChars(charsDisplay);
  sevseg.refreshDisplay();

  

  switch (statoSveglia) {
    // case sIMPOSTAALLARME:
    //   counter += 250;
    //   delay(250);
    //   break;
    default:
      counter++;
      delay(1);
      break;
  }
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
    case sIMPOSTAORA: {
        switch (statoImpostaOra) {
          case sORESVEGLIA: {
              counter = 0;
              if(oreTMPSVEGLIA == 23) {
                oreTMPSVEGLIA = 0;
              } else {
                oreTMPSVEGLIA++;
              }
            }
            break;
          case sMINUTISVEGLIA: {
              counter = 0;
              if(minutiTMPSVEGLIA == 59) {
                minutiTMPSVEGLIA = 0;
              } else {
                minutiTMPSVEGLIA++;
              }
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
          //case sSHOWALARM:
          case sSHOWON: {
              counter = 0;
              statoOnOff = sSHOWOFF;
              enableAlarm = false;
            }
            break;
          case sSHOWOFF: {
              counter = 0;
              statoOnOff = sSHOWON;
              enableAlarm = true;
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
    case sIMPOSTAALLARME: {
        switch (statoImpostaAllarme) {
          case sOREALLARME: {
              counter = 0;
              if(oreTMPALLARME == 23) {
                oreTMPALLARME = 0;
              } else {
                oreTMPALLARME++;
              }
            }
            break;
          case sMINUTIALLARME: {
              counter = 0;
              if(minutiTMPALLARME == 59) {
                minutiTMPALLARME = 0;
              } else {
                minutiTMPALLARME++;
              }
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
        Time now = rtc.getTime();
        oreTMPALLARME = now.hour;
        minutiTMPALLARME = now.min;

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

  oreTMPALLARME = 0;
  minutiTMPALLARME = 0;

  Serial.print("Sveglia impostata alle ");
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