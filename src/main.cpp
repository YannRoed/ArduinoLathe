#include <Arduino.h>
#include <Encoder.h>
#include <Stepper.h>
#include <Wire.h>              //for ESP8266 use bug free i2c driver https://github.com/enjoyneering/ESP8266-I2C-Driver
#include <LiquidCrystal_I2C.h>


#define COLUMS 16
#define ROWS   2

LiquidCrystal_I2C lcd(PCF8574_ADDR_A21_A11_A01);


// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
Encoder myEnc(3, 2);
Stepper myStepper(11,12,10);
//   avoid using pins with LEDs attached

void updateDisplay(){
  lcd.clear();
  lcd.home();
  lcd.setCursor(0,0);
  lcd.print("SS:");
  lcd.print(myStepper.getSpeed());
  lcd.setCursor(7,0);
  lcd.print("FS:");
  lcd.print(myStepper.getSpeed());
  lcd.setCursor(1,1);
  lcd.print("X:");
  lcd.print(myStepper.getTargetPosition());
  lcd.setCursor(8,1);
  lcd.print("Y:");
  lcd.print(myStepper.getCurrentPosition());
}



void setup() {
  Serial.begin(9600);
  Serial.println("Basic Encoder Test:");
  pinMode(4,INPUT);
  myStepper.enable(true);

  lcd.begin(COLUMS,ROWS,LCD_5x8DOTS);
  lcd.backlight();
  lcd.home();
}

long oldPosition  = -999;

void loop() {
  myStepper.doEvents();

  long newPosition = myEnc.read()/4;
  if(!digitalRead(4)){
    /* myStepper.enable(!myStepper.isEnable());
    if(myStepper.isEnable()){Serial.println("enable stepper");}
    else {Serial.println("disable stepper");} */

    if(myStepper.getMode()==Stepper::CONTINUES){
      myStepper.setMode(Stepper::TOTARGET);
      Serial.println("toTarget");
    }
    else{
       myStepper.setMode(Stepper::CONTINUES);
       Serial.println("continues");
    }
    delay(200);
    while(!digitalRead(4));
  }
  if (newPosition != oldPosition) {
    if(myStepper.getMode()==Stepper::CONTINUES){
      myStepper.setSpeed(newPosition);
      Serial.print("Speed:");
      Serial.println(myStepper.getSpeed());
    }else{
      if(oldPosition < newPosition)myStepper.setTargetPosition(myStepper.getTargetPosition()+20);
      else myStepper.setTargetPosition(myStepper.getTargetPosition()-20);
    }
    
    updateDisplay();
    oldPosition = newPosition;
    Serial.println(newPosition);
  }
}