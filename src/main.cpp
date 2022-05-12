#include <Arduino.h>
#include <Encoder.h>
#include <Stepper.h>
#include <Wire.h>              //for ESP8266 use bug free i2c driver https://github.com/enjoyneering/ESP8266-I2C-Driver
#include <LiquidCrystal_I2C.h>


#define COLUMS 16
#define ROWS   2

#define S1 15
#define EncT1 14
#define Tmode 6
#define Tpositiv 5
#define Tnegative 4

bool oldTpositiv = HIGH;
bool oldTnegative = HIGH;
bool oldTmode = HIGH;
bool oldEncT1 = HIGH;

enum eSelected:int{eFirstSelected,eXachse,eYachse,eSpindelSpeed,eFeedSpeed,eManuelOverride,lastSelected};
enum eMode:int{eEconderUse,ePowerFeed,eMenueSelection};
 
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01);
// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
Encoder myEnc(2, 3);
Stepper XStepper(25,23,27);
Stepper YStepper(31,29,33);

eSelected selected = eXachse;
eMode operateMode = eNormal;
int spindelSpeed = 0;
int manuelOverride = 20;
int feedSpeed = 10;
long oldPosition  = 0;
unsigned long displayUpdate = 0;

//   avoid using pins with LEDs attached


void basicTest(){
  if(digitalRead(S1)){
    Serial.println("S1 on");
  }else{
    Serial.println("S1 off");
  }

    if(digitalRead(EncT1)){
    Serial.println("EncT1 on");
  }else{
    Serial.println("EncT1 off");
  }

    if(digitalRead(Tmode)){
    Serial.println("Tmode on");
  }else{
    Serial.println("Tmode off");
  }

    if(digitalRead(Tpositiv)){
    Serial.println("Tpositiv on");
  }else{
    Serial.println("Tpositiv off");
  }

    if(digitalRead(Tnegative)){
    Serial.println("Tnegative on");
  }else{
    Serial.println("Tnegative off");
  }
  long newPosition = myEnc.read()/4;
  Serial.println("");
  Serial.println("");
  Serial.println("");
  lcd.clear();
  lcd.home();
  lcd.setCursor(0,0);
  lcd.print("Enc:");
  lcd.print(newPosition);
  delay(500);
}

void updateDisplay(){
  lcd.clear();
  lcd.home();
  lcd.setCursor(0,0);
  lcd.print(selected==eSpindelSpeed?"SS:":"ss:");
  lcd.print(spindelSpeed);
  lcd.setCursor(6,0);
  lcd.print(selected==eFeedSpeed?"FS:":"fs:");
  lcd.print(feedSpeed);
  lcd.setCursor(11,0);
  lcd.print(selected==eManuelOverride?"OV:":"ov:");
  lcd.print(manuelOverride);
  lcd.setCursor(1,1);
  lcd.print(selected==eXachse?"X:":"x:");
  lcd.print(XStepper.getTargetPosition());
  lcd.setCursor(8,1);
  lcd.print(selected==eYachse?"Y:":"y:");
  lcd.print(YStepper.getTargetPosition());
}

void menueSelection(){
  if(!digitalRead(Tmode)){
    myEnc.readAndReset();
    int encOld = myEnc.read()/4;
    while(!digitalRead(Tmode)){
      int encNew = myEnc.read()/4;
      if(encNew != encOld){
        int temp = selected;
        if(encNew > encOld){
          temp++;
        }else{
          temp--;
        }
        if(temp <= eFirstSelected){
          temp = lastSelected;
          temp--;
        }
        if(temp >= lastSelected){
          temp = eFirstSelected;
          temp++;
        }
        selected = static_cast<eSelected>(temp);
        encOld = encNew; 
        updateDisplay();
      }
    }
    switch(selected){
      case eXachse:
        myEnc.write(XStepper.getCurrentPosition()*4);
      break;
      case eYachse:
        myEnc.write(YStepper.getCurrentPosition()*4);
      break;
      case eSpindelSpeed:
        myEnc.write(spindelSpeed*4);
        break;
      case eFeedSpeed:
        myEnc.write(feedSpeed*4);
        break;
      case eManuelOverride:
        myEnc.write(manuelOverride*4);
        break;
    }
    oldPosition = myEnc.read()/4;
  }
}

void encoderOperation(){
   long newPosition = myEnc.read()/4;
  if (newPosition != oldPosition) {
    switch(selected){
      case eXachse:
        if(oldPosition < newPosition)XStepper.setTargetPosition(XStepper.getTargetPosition()+manuelOverride);
        else XStepper.setTargetPosition(XStepper.getTargetPosition()-manuelOverride);
        break;
      case eYachse:
        if(oldPosition < newPosition)YStepper.setTargetPosition(YStepper.getTargetPosition()+manuelOverride);
        else YStepper.setTargetPosition(YStepper.getTargetPosition()-manuelOverride);
        break;
      case eSpindelSpeed:
        if(oldPosition < newPosition)spindelSpeed++;
        else spindelSpeed--;
        break;
      case eFeedSpeed:
        if(oldPosition < newPosition)feedSpeed++;
        else feedSpeed--;
        if(feedSpeed<0)feedSpeed=0;
        break;
      case eManuelOverride:
        if(oldPosition < newPosition)manuelOverride++;
        else manuelOverride--;
        if(manuelOverride<0)manuelOverride=0;
        break;
      default:
        Serial.println("default case should not happen");
    }
    oldPosition = newPosition;
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(S1,INPUT_PULLUP);
  pinMode(EncT1,INPUT_PULLUP);
  pinMode(Tmode,INPUT_PULLUP);
  pinMode(Tpositiv,INPUT_PULLUP);
  pinMode(Tnegative,INPUT_PULLUP);

  XStepper.enable(true);
  XStepper.setMode(Stepper::TOTARGET);
  YStepper.enable(true);
  YStepper.setMode(Stepper::TOTARGET);

  lcd.begin(COLUMS,ROWS,LCD_5x8DOTS);
  lcd.backlight();
  lcd.home();
}

void loop() {
  if(digitalRead(S1)){
    XStepper.doEvents();
    YStepper.doEvents();
  }

  //basicTest();
  //return;
  switch (operateMode)
  {
    case eEconderUse:
      encoderOperation
      break;
    case ePowerFeed:
    
      break;
    case eMenueSelection:
      menueSelection();
      break;
  }

  

 
  //TODO powerfeed on button pressed


  if(oldTnegative != digitalRead(Tnegative)){
    oldTnegative = digitalRead(Tnegative);
    if(digitalRead(Tnegative)){
      YStepper.setMode(Stepper::TOTARGET);
      Serial.println("totarget");
    }
    else{
      YStepper.setSpeed(feedSpeed);
      YStepper.setMode(Stepper::CONTINUES);
      Serial.println("continues");
    }
  }


  //TODO zeroing by pressing top right button over 2 sek
  //TODO rapid power feed if encoder switch is pressed durring power feed
  //TODO go to select menue by tapping encoder taster
 
/*   
  if(!digitalRead(Tmode)){
    if(XStepper.getMode()==Stepper::CONTINUES){
      XStepper.setMode(Stepper::TOTARGET);
      Serial.println("toTarget");
    }
    else{
       XStepper.setMode(Stepper::CONTINUES);
       Serial.println("continues");
    }
    delay(200);
    while(!digitalRead(Tmode));
  }
*/
    if (micros() - displayUpdate >= abs(500000)){
      updateDisplay();
      displayUpdate = micros();
    }
}