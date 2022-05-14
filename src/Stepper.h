#include <Arduino.h>

class Stepper
{
  public:
  enum eMODE{CONTINUES,TOTARGET};
    Stepper(int d,int e,int p,bool invert){
      dir = d;
      ena = e;
      pul = p;
      pinMode(dir,OUTPUT);
      pinMode(ena,OUTPUT);
      pinMode(pul,OUTPUT);
      digitalWrite(dir,LOW);
      digitalWrite(ena,HIGH);
      digitalWrite(pul,LOW);
      invertDir = invert;
    }
    void enable(bool e){writePin(ena,!e);}
    bool isEnable(){return !lastEna;}
    void setTargetPosition(long int newPosition){
      if(abs(newPosition-currentPosition)>maxSpeedDist)return;
      targetPosition = newPosition;
    }
    long int getTargetPosition(){return targetPosition;}
    void setCurrentPosition(long int newPosition){currentPosition=newPosition;}
    long int getCurrentPosition(){return currentPosition;}
    void setSpeed(int newSpeed){//speed = rotations per minute
        speed = newSpeed;
        long int minInMicros = 60000000;
        long int stepsPerMinute = (long int)200 * (long int)newSpeed;
        stepInterval =  minInMicros/stepsPerMinute;
    }
    int getSpeed(){return speed;}
    void setMode(eMODE newMode){
        if(newMode==TOTARGET)targetPosition=currentPosition;
        mode = newMode;
    }
    eMODE getMode(){return mode;}

    void doEvents(){
      if(mode == TOTARGET){
        if(targetPosition!=currentPosition){
            if (micros() - timeOfLastStep >= abs(stepInterval)){
                int absPosDiv = abs(targetPosition - currentPosition);
                int newSpeed = absPosDiv;
                if(absPosDiv<minSpeed)newSpeed = minSpeed;
                if(absPosDiv>maxSpeedDist)newSpeed = maxSpeedDist;
                setSpeed(newSpeed);
                step(targetPosition > currentPosition);
            }  
        }
      }else{
        if(speed == 0) enable(false);
        else enable(true);
        
        if (micros() - timeOfLastStep >= abs(stepInterval)){
            step(stepInterval > 0);
        }
        targetPosition = currentPosition;
      }
    }

  private:
    int dir;
    int ena;
    int pul;
    bool invertDir = false;
    bool lastDir = LOW;
    bool lastEna = HIGH;
    bool lastPul = LOW;
    long int targetPosition = 0;
    long int currentPosition = 0;
    unsigned long timeOfLastStep = 0;
    long int stepInterval = 1000;//TODO: replace with some kinde of speed
    int speed = 1;
    const int maxSpeedDist = 300;
    const int minSpeed = 30;
    eMODE mode = TOTARGET;

    void writePin(int pin,bool level){
      if(pin == dir && lastDir != level){
        digitalWrite(pin,level);
        lastDir = level;
      } 
      else if(pin == ena && lastEna != level){
        digitalWrite(pin,level);
        lastEna = level;
      } 
      else if(pin == pul && lastPul != level){
        digitalWrite(pin,level);
        lastPul = level;
      } 
    }
    void step(bool direction){
      //writePin(ena,LOW);
      //manage position counting
      if(direction) currentPosition++;
      else currentPosition--;
      
      writePin(dir, invertDir==false ? direction: !direction);
      oneStep();
      timeOfLastStep = micros();

    }
    void oneStep(){
        writePin(pul,LOW);
        writePin(pul,HIGH);
        delay(1);
    }
};