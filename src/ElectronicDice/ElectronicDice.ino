#include "LedControl.h"
#include <RBD_Timer.h>
#include <RBD_Button.h>
#include "pitches.h"

 // pin 9 is connected to the DataIn 
 // pin 8 is connected to the CLK 
 // pin 7 is connected to LOAD 
 // Only a single MAX72XX.
LedControl lc = LedControl(9,8,7,1);

// Wait a bit between updates of the display 
unsigned long delaytime=100;

RBD::Button buttonDice(6); // input_pullup by default
RBD::Button buttonCountDown(5); // input_pullup by default
RBD::Timer timer;
RBD::Timer setupTimer;

int countDownStart = 60;
int diceCount = 2;

// notes in the melody:
int melody[] = {
  NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4
};

// note durations: 4 = quarter note, 8 = eighth note, etc.:
int noteDurations[] = {
  4, 8, 8, 4, 4, 4, 4, 4
};

int setupMode = 0;

void setup() {
  // only one digit per address
  lc.setScanLimit(0, 1);
  lc.setScanLimit(1, 1);

  // The MAX72XX is in power-saving mode on startup, do a wakeup call
  lc.shutdown(0, false);
  
  // Set the brightness to a medium values 
  lc.setIntensity(0, 15);  

  initScreen();
  timer.stop();

  randomSeed(analogRead(0));
  pinMode(3, OUTPUT);

  // Current draw to keep power bank on
  digitalWrite(3, HIGH);
  delay(500);
  digitalWrite(3, LOW);
  
}

void initScreen() {
  lc.clearDisplay(0);
  lc.setChar(0, 1, ' ', true);
}

void printNumber(int v) {  
  int ones;  
  int tens;  
  
  if(v > 99)  
      return;  
      
  ones = v % 10;  
  v    = v / 10;  
  tens = v % 10;  
  
  //Now print the number digit by digit 
  if (tens == 0) {
    lc.setChar(0, 1, ' ', false);
  } else {
    lc.setDigit(0, 1, (byte)tens, false); 
  }
  
  lc.setDigit(0, 0, (byte)ones, false); 
} 

bool countDownReady = false;
bool diceReady = false;

void loop() { 
  inSetupMode();
  
  if (setupMode == 0) {
    handleCountDown();
    handleDice();
  } else if (setupMode == 3) {
    enterSetupType();
  } else if (setupMode == 4) {
    setupDice();
  } else if (setupMode == 5) {
    setupCounter();
  }
}

void enterSetupType() {
  if (buttonDice.onPressed()) {
    lc.setChar(0, 0, '2', false);
    lc.setChar(0, 1, '1', true);
    setupMode = 4;
  } else if (buttonCountDown.onPressed()) {
    printNumber(countDownStart);
    setupMode = 5;
  }    
}

int inSetupMode() {
  if(buttonDice.isPressed()
    && buttonCountDown.isPressed()) {
  
    diceReady      = false;
    countDownReady = false;
    timer.stop();

    if (setupMode == 0) {
      setupMode = 1;
      printNumber(88);
      setupTimer.setTimeout(5000);
      setupTimer.restart();
    } else if(setupMode == 1 && setupTimer.onRestart()) {
      lc.setChar(0, 0, '_', false);
      lc.setChar(0, 1, '_', false);
      setupMode = 2;
    }
  } else if (!buttonDice.isPressed()
    && !buttonCountDown.isPressed()) {  
      
    if (setupMode == 1) {
      setupTimer.stop();
      setupMode = 0;
      initScreen();
    } else if (setupMode == 2) {
      setupMode = 3;
    }
  }

  return setupMode;
}

void handleDice() {
    if(buttonDice.onPressed()) {
    ready();
    diceReady = true;
    timer.stop();
  }

  if(diceReady && buttonDice.onReleased()) {
    rollDice();
    countDownReady = false;
  }       
}

void rollDice() {
  for (int j = 0; j < 4; j++) {
    spin();
  }

  int die1 = random(1, 7);
  int die2 = random(1, 7);

  lc.clearDisplay(0);
  
  //printNumber(die1 + die2);
  lc.setDigit(0, 0, (byte)die1, true); 

  if (diceCount > 1)
    lc.setDigit(0, 1, (byte)die2, true); 
}

void handleCountDown() {
  if (buttonCountDown.onPressed()) {
    ready();
    countDownReady = true;
    timer.stop();
  }

  if (countDownReady && buttonCountDown.onReleased()) {
    startCountDown(countDownStart, 1000);
    countDownReady = false;
  }       

  if (!timer.isStopped()) {
    checkCountDown();
  }
}

void spin() {
  lc.clearDisplay(0);
  
  for (int i = 1; i < 7; i++) {
    lc.setLed(0, 0, i, true);
    lc.setLed(0, 1, i, true);
    delay(30);
    lc.setLed(0, 0, i, false);
    lc.setLed(0, 1, i, false);
    delay(10);      
  }
}

void ready() {
  lc.setChar(0, 0, '-', false);
  lc.setChar(0, 1, '-', false);
}

int counter;

void startCountDown(int from, int waitTime) {
  printNumber(from);
  playTheStart();
  counter = from - 1;

  timer.setTimeout(waitTime);
  timer.restart();
}

void checkCountDown() {
    if(timer.onRestart()) {
      printNumber(counter);

      counter--;

      if (counter >= 0) {
        timer.restart();
      } else {
        timer.stop();
        playTheEnd();
      }
  }    
}


void playTheEnd() {
  // iterate over the notes of the melody:
  for (int thisNote = 0; thisNote < 8; thisNote++) {

    // to calculate the note duration, take one second
    // divided by the note type.
    //e.g. quarter note = 1000 / 4, eighth note = 1000/8, etc.
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(4, melody[thisNote], noteDuration);

    // to distinguish the notes, set a minimum time between them.
    // the note's duration + 30% seems to work well:
    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(4);
  }
}

void playTheStart(){
    tone(4, 4978, 1000);
    delay(200);
    tone(4, 659, 200);
}

void setupDice() {
  if (buttonDice.onPressed()) {
    blink();
    diceCount = 1;
    setupMode = 0;
  }

  if (buttonCountDown.onPressed()) {
    blink();
    diceCount = 2;
    setupMode = 0;
  }  
}

void setupSetupCounter() {
    
}

void blink() {
  for (int k = 0; k < 2; k++) {
    lc.setChar(0, 0, '0', false);
    lc.setChar(0, 1, '0', false);
    delay(200);
    lc.clearDisplay(0);  
    delay(200);
  }

  initScreen();
}


void setupCounter() {
  setupTimer.setTimeout(5000);
  
  if (buttonDice.onPressed()) {
    countDownStart--;
    setupTimer.restart();
  }

  if (buttonCountDown.onPressed()) {
    countDownStart++;
    setupTimer.restart();
  }  
 
  printNumber(countDownStart);

  if (setupTimer.onRestart()) {
    blink();
    setupMode = 0;
  }
}


