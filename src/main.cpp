#include <Arduino.h>
#include <Encoder.h>
#include <Stepper.h>
#include <Wire.h>              //for ESP8266 use bug free i2c driver https://github.com/enjoyneering/ESP8266-I2C-Driver
#include <LiquidCrystal_I2C.h>
#include<avr/io.h>
#include<avr/interrupt.h>
#include <PID_v1.h>


#define COLUMS 16
#define ROWS   2

#define S1 15
#define EncT1 14
#define Tmode 6
#define Tpositiv 5
#define Tnegative 4
#define SpindelEnc 19
#define SpindelPWM 8

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


eSelected selected = eSpindelSpeed;
eMode operateMode = eEconderUse;

//Specify the links and initial tuning parameters
double Setpoint, Input, PidOutput;
double Kp=0.2, Ki=1.5, Kd=0;
PID myPID(&Input, &PidOutput, &Setpoint, Kp, Ki, Kd, P_ON_E,DIRECT);

int spindelSpeed = 250;
float spindelCurrentSpeed = 0;
unsigned int spindelEncoderCount = 0;
unsigned long timeLastPid = 0;

int manuelOverride = 20;
int feedSpeed = 10;
long oldPosition  = 0;
unsigned long displayUpdate = 0;
unsigned long debounce = 0;

ISR (TIMER1_OVF_vect) { // Timer1 ISR
  XStepper.doEvents();
  YStepper.doEvents();
  ZStepper.doEvents();
  TCNT1 = 65472;//65536-64;
}

void SpindelEncoderInterrupt(){
  spindelEncoderCount++;
}

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
    lcd.print(selected==eSpindelSpeed?spindelSpeed:(int)spindelCurrentSpeed);
    lcd.setCursor(6,0);
    lcd.print(selected==eFeedSpeed?"FS":"fs");
    lcd.print(feedSpeed);
    lcd.setCursor(11,0);
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
  //select the value to change when in menue mode
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
  //used to change values for the selected option
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
        if(spindelSpeed<0)spindelSpeed=0;
        if(spindelSpeed>999)spindelSpeed=999;
        Setpoint = spindelSpeed;
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
  //constantly move the selected aches
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
  if(!digitalRead(EncT1))usedSpeed = 350;
  if(!digitalRead(Tnegative))usedSpeed = -usedSpeed;
  XStepper.setSpeed(usedSpeed);
  YStepper.setSpeed(usedSpeed);
  ZStepper.setSpeed(usedSpeed);
}

void handleModeButton(){
// zeroing by pressing top right button (Mode) in menue mode
//switch between Z and X by pressing Mode over 50ms
if(!digitalRead(Tmode)){
  unsigned long pressedTimer = abs(micros() - debounce);
  if(pressedTimer >= 50000 && operateMode == eMenueSelection){
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
      updateDisplay();
      while(!digitalRead(Tmode)){}
    }
    if(pressedTimer >= 50000 && operateMode == eEconderUse){
      if(selected == eXachse){
        selected = eZachse;
      }else{
        selected = eXachse;
      }
      while(!digitalRead(Tmode)){}
    }
  }else{
    debounce = micros();
    return;
  }
}

void enableSelectedMotor(){
  //reduce power use and motor heating by disable not used motors
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

void spindelControle(){
  //PID stuff for spindel controll

  unsigned long timeNow = micros();
  if(abs(timeNow-timeLastPid)>50000){ 
    int saveCount = spindelEncoderCount;
    int countsPerRot = 40;
    spindelEncoderCount = 0;
    float timeSinceLastPid = abs(timeNow-timeLastPid);
    float roundsSinceLastPid = saveCount/countsPerRot;
    spindelCurrentSpeed = (1000000.0/timeSinceLastPid) * roundsSinceLastPid ;
    Input = spindelCurrentSpeed;
    myPID.Compute();

    if(digitalRead(S1)){
      Setpoint = spindelSpeed;
    }else{
      Setpoint = 0;
      PidOutput = 0;
    }

    analogWrite(SpindelPWM,PidOutput);
    timeLastPid = timeNow;
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(S1,INPUT_PULLUP);
  pinMode(EncT1,INPUT_PULLUP);
  pinMode(Tmode,INPUT_PULLUP);
  pinMode(Tpositiv,INPUT_PULLUP);
  pinMode(Tnegative,INPUT_PULLUP);
  pinMode(SpindelEnc,INPUT_PULLUP);
  pinMode(SpindelPWM,OUTPUT);

  attachInterrupt(digitalPinToInterrupt(SpindelEnc), SpindelEncoderInterrupt, CHANGE);
  myPID.SetOutputLimits(0,255);
  myPID.SetSampleTime(50);
  myPID.SetMode(AUTOMATIC);
  Setpoint = spindelSpeed;
  
  XStepper.enable(false);
  XStepper.setMode(Stepper::TOTARGET);
  YStepper.enable(false);
  YStepper.setMode(Stepper::TOTARGET);
  ZStepper.enable(false);
  ZStepper.setMode(Stepper::TOTARGET);

  lcd.begin(COLUMS,ROWS,LCD_5x8DOTS);
  lcd.backlight();
  lcd.home();
  operateMode = eMenueSelection;
  updateDisplay();
  operateMode = eEconderUse;

//setup timer interrupt for stepper moto movements
  TCNT1 = 65472;//65536-64;   // for 1 sec at 16 MHz	
	TCCR1A = 0x00;
	TCCR1B = (0<<CS10)|(1 << CS11)|(0<<CS12);  // Timer mode with 1024 prescler
	TCCR1C = 0x00;
  TIMSK1 = (1 << TOIE1) ;   // Enable timer1 overflow interrupt(TOIE1)
  sei();
}

void loop() {

  //basicTest();
  //return;
  spindelControle();
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