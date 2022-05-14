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

enum eSelected:int{eFirstSelected,eXachse,eYachse,eZachse,eSpindelSpeed,eFeedSpeed,eManuelOverride,lastSelected};
enum eMode:int{eEconderUse,ePowerFeed,eMenueSelection};
 
LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01);
// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
Encoder myEnc(2, 3);
Stepper XStepper(25,23,27,true);
Stepper YStepper(37,35,39,false);
Stepper ZStepper(31,29,33,true);


eSelected selected = eXachse;
eMode operateMode = eEconderUse;
int spindelSpeed = 0;
int manuelOverride = 20;
int feedSpeed = 10;
long oldPosition  = 0;
unsigned long displayUpdate = 0;
unsigned long debounce = 0;

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
    lcd.print(operateMode==eMenueSelection?"M":" ");
    lcd.setCursor(1,0);
    lcd.print(selected==eSpindelSpeed?"SS":"ss");
    lcd.print(spindelSpeed);
    lcd.setCursor(7,0);
    lcd.print(selected==eFeedSpeed?"FS":"fs");
    lcd.print(feedSpeed);
    lcd.setCursor(12,0);
    lcd.print(selected==eManuelOverride?"OV":"ov");
    lcd.print(manuelOverride);
    lcd.setCursor(0,1);
    lcd.print(selected==eXachse?"X":"x");
    lcd.print(XStepper.getTargetPosition());
    lcd.setCursor(5,1);
    lcd.print(selected==eYachse?"Y":"y");
    lcd.print(YStepper.getTargetPosition());
    lcd.setCursor(11,1);
    lcd.print(selected==eZachse?"Z":"z");
    lcd.print(ZStepper.getTargetPosition());
}

void menueSelection(){
  long newPosition = myEnc.read()/4;
  if(newPosition != oldPosition){
    int temp = selected;
    if(newPosition > oldPosition){
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
    oldPosition = newPosition; 
    updateDisplay();
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
      case eZachse:
        if(oldPosition < newPosition)ZStepper.setTargetPosition(ZStepper.getTargetPosition()+manuelOverride);
        else ZStepper.setTargetPosition(ZStepper.getTargetPosition()-manuelOverride);
        break;
      case eSpindelSpeed:
        if(oldPosition < newPosition)spindelSpeed++;
        else spindelSpeed--;
        if(spindelSpeed<-99)spindelSpeed=-99;
        if(spindelSpeed>99)spindelSpeed=99;
        break;
      case eFeedSpeed:
        if(oldPosition < newPosition)feedSpeed++;
        else feedSpeed--;
        if(feedSpeed<0)feedSpeed=0;
        if(feedSpeed>99)feedSpeed=99;
        break;
      case eManuelOverride:
        if(oldPosition < newPosition)manuelOverride++;
        else manuelOverride--;
        if(manuelOverride<0)manuelOverride=0;
        if(manuelOverride>99)manuelOverride=99;
        break;
      default:
        Serial.println("default case should not happen");
    }
    oldPosition = newPosition;
  }
}

void powerFeedControle(){
  int usedSpeed = feedSpeed;
  switch (selected)
  {
    case eXachse:
      if(XStepper.getMode() == Stepper::TOTARGET)
        XStepper.setMode(Stepper::CONTINUES);
      break;
    case eYachse:
      if(YStepper.getMode() == Stepper::TOTARGET)
        YStepper.setMode(Stepper::CONTINUES);  
      break;
    case eZachse:
      if(ZStepper.getMode() == Stepper::TOTARGET)
        ZStepper.setMode(Stepper::CONTINUES);  
      break;
  }
  //speed override
  if(!digitalRead(EncT1))usedSpeed = 100;
  if(!digitalRead(Tnegative))usedSpeed = -usedSpeed;
  XStepper.setSpeed(usedSpeed);
  YStepper.setSpeed(usedSpeed);
  ZStepper.setSpeed(usedSpeed);
}

void handleModeButton(){
// zeroing by pressing top right button over 1 sek
  if(digitalRead(Tmode)){
    debounce = micros();
  }
 // if(!digitalRead(Tmode)){
    if(micros()-debounce >= abs(1000000)){
      switch(selected){
        case eXachse:
          XStepper.setTargetPosition(0);
          XStepper.setCurrentPosition(0);
          break;
        case eYachse:
          YStepper.setTargetPosition(0);
          YStepper.setCurrentPosition(0);
          break;
        case eZachse:
          ZStepper.setTargetPosition(0);
          ZStepper.setCurrentPosition(0);
          break;
        case eSpindelSpeed:
          spindelSpeed = 0;
          break;
        case eFeedSpeed:
          feedSpeed = 0;
          break;
        case eManuelOverride:
          manuelOverride=0;
          break;
        default:
          Serial.println("default case should not happen");
      }
    }
  //}
  if(micros()-debounce < abs(1000000) &&micros()-debounce >= abs(50000)){
    if(selected == eXachse)selected = eZachse;
    else selected = eXachse;
  }
}

void enableSelectedMotor(){
  if(operateMode == eMenueSelection){
    XStepper.enable(false);
    YStepper.enable(false);
    ZStepper.enable(false);
    return;
  }
  if(selected == eXachse && !XStepper.isEnable()){
    XStepper.enable(true);
    YStepper.enable(false);
    ZStepper.enable(false);
  }
  if(selected == eYachse && !YStepper.isEnable()){
    XStepper.enable(false);
    YStepper.enable(true);
    ZStepper.enable(false);
  }
  if(selected == eZachse && !ZStepper.isEnable()){
    XStepper.enable(false);
    YStepper.enable(false);
    ZStepper.enable(true);
  }
}

void operationModeSwitch(){
  //switch between encoder use and menue selected
  if(!digitalRead(EncT1) && operateMode != ePowerFeed){
    if(operateMode == eEconderUse){
      operateMode = eMenueSelection;
    }
    else if(operateMode == eMenueSelection){
       operateMode = eEconderUse;
    }
    delay(200);
    while(!digitalRead(EncT1));
  }

//switch powerfeed to notpowerfeed from mode
  if(operateMode == eEconderUse && (!digitalRead(Tpositiv) || !digitalRead(Tnegative))){
    operateMode =  ePowerFeed;
  }else if(operateMode == ePowerFeed && digitalRead(Tpositiv) && digitalRead(Tnegative)){
    operateMode = eEconderUse;
    XStepper.setMode(Stepper::TOTARGET);
    YStepper.setMode(Stepper::TOTARGET);
    ZStepper.setMode(Stepper::TOTARGET);
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
  ZStepper.enable(true);
  ZStepper.setMode(Stepper::TOTARGET);

  lcd.begin(COLUMS,ROWS,LCD_5x8DOTS);
  lcd.backlight();
  lcd.home();
  operateMode = eMenueSelection;
  updateDisplay();
  operateMode = eEconderUse;
}

void loop() {

  //basicTest();
  //return;
  
  XStepper.doEvents();
  YStepper.doEvents();
  ZStepper.doEvents();

  enableSelectedMotor();
  handleModeButton();
  operationModeSwitch();

  switch (operateMode)
  {
    case eEconderUse:
      encoderOperation();
      break;
    case ePowerFeed:
      powerFeedControle();
      break;
    case eMenueSelection:
      menueSelection();
      break;
  }

//update display ever 500ms
  if (micros() - displayUpdate >= abs(500000)){
    updateDisplay();
    displayUpdate = micros();
  }
}