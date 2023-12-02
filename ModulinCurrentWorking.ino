/* 
TODO:
 - Develop function to make a gradient slide effect from notes + bends
 - Develop a single-press input function on trellis for settings panel
 - Attain multi-channel midi output via drum machine
 */

// LIBRARIES USED
#include <MIDIUSB.h> //Midi Communication protocol
#include <Wire.h> // i2c protocol
#include "Adafruit_NeoTrellis.h" // Neotrellis library

Adafruit_NeoTrellis trellis; //define neotrellis object
#define INT_PIN 10 //define neotrellis interrupt pin

// PIN ASSIGNMENTS
const int buttonPin = 5;  // the number of the pushbutton pin
const int velocityPin = A3; // analog input for velocity potentiometer
const int pitchPin = A5; // the number of the ribbon potentiometer pin

const unsigned long debounceDelay = 5; //length of debounces in ms
const unsigned long debounceNote = 10;


const int startNote = 79; // the lowest note playable on the modulin - start on middle c
const int fretNumber = 15; // the number of notes the potentiometer can toggle
int modeCode = 0;
int noteVelocity = 80;

// PITCH COMPUTATION
byte keyRoots[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; // *delete*?
byte major[7] = {0, 2, 4, 5, 7, 9, 11}; 
byte minor[7] = {0, 2, 3, 5, 7, 8, 10};
byte blues[6] = {0, 3, 5, 6, 7, 9};
byte pentatonic[5] = {0, 2, 4, 7, 9};
int numModes = 4; 
byte modeLengths[4] = {7, 7, 6, 5};
byte* modes[] = {major, minor, blues, pentatonic}; 
int bufferValue = 25; // fret buffer distance

// INITIALIZE AT GLOBAL
int buttonState = 0;  
int lastButtonState = HIGH; 
unsigned long lastButtonDebounceTime = 0;
unsigned long lastRibbonTriggerTime = 0;
int previousNote = -1; 
int currentTime = millis();
int lastTime = 0;
byte isOn = 0;
int lastNote = -1;
int currentPage = 0;
int lastFret = 0;

const int numReadings = 10;
int readings[numReadings];
int readIndex = 0;
int total = 0;
int noteAverage = 0;

boolean averagingPaused = false;
unsigned long lastDebounceTime = 0;
///////////////////////////////////////////////////////////////////

// TRELLIS DATA
byte modeMemory[4][16] = { // Button position memory bank
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

void setup() {
  Serial.begin(9600);
  Wire.begin();
  Wire.setClock(50000); // match Metro Mini i2c clock of 50kHz
  pinMode(INT_PIN, INPUT); // Setup interrupt pin

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }

  if(!trellis.begin()){
    //Serial.println("could not start trellis");
    while(1) delay(1);
  }
  else{
    Serial.println("trellis started");
  }

  //activate all keys and set callbacks
  for(int i=0; i<NEO_TRELLIS_NUM_KEYS; i++){
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, blink);
  }

  //do a little animation to show we're on
  for(uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, 50, 0, 150); // colors for initial flash
    trellis.pixels.show();
    delay(50);
  }
  for(uint16_t i=0; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i,100, 100, 100);
    trellis.pixels.show();
    delay(50);
  }
  trellis.pixels.setPixelColor(0, 255, 0, 0);
  trellis.pixels.setPixelColor(1, 255, 255, 50);
  trellis.pixels.setPixelColor(2, 50, 255, 50);
  trellis.pixels.setPixelColor(3, 50, 50, 255);
  trellis.pixels.show();

  // initialize the pushbutton pins as an inputs:
  pinMode(buttonPin, INPUT);
  pinMode(pitchPin, INPUT);
}

void loop() {
  
  if(!digitalRead(INT_PIN)){ // if a button is pressed, run trellis functions
    trellis.read(false);
  } 

  if ((millis() - lastButtonDebounceTime) > debounceDelay) { // if it's been 5 ms since the last update
    int reading = digitalRead(buttonPin); // read the state of the pushbutton value:
    if (reading != lastButtonState) {
      lastButtonDebounceTime = millis(); // record when the button value changes
      //Serial.println("DifferenceIdentified");
    }
    if (reading != buttonState) { // update current button state
      buttonState = reading;
      } 
      if (buttonState == LOW && isOn == 1) { // If the button is released but a note is playing: 
        Serial.println("button up, kill note");
        noteOff(0, lastNote, 0); //Kill the note
        MidiUSB.flush();
        toDisplay(999); // turn off display if no note is present
        isOn = 0;
      } else if (buttonState == HIGH && isOn == 0) {
        Serial.println("button down, play once");
        noteOn(0, computeNote(modeCode, pitchPin, fretNumber), noteVelocity); //play the note
        lastRibbonTriggerTime = millis();
        MidiUSB.flush();
        toDisplay(computeNote(modeCode, pitchPin, fretNumber)); // turn off display if no note is present
        isOn = 1;
      }
    }

  if (buttonState == HIGH) {
    if (millis() - lastRibbonTriggerTime > debounceNote) {
      updateNote(computeNote(modeCode, pitchPin, fretNumber));
    }
  }

    lastButtonState = buttonState;
    //Serial.println(analogRead(pitchPin));
    
    if (buttonState == LOW) { // Button is pressed
      if (!averagingPaused) {
        averagingPaused = true;
        Serial.println("Averaging Paused");
      }
    } else { // Button is released
      if (averagingPaused) {
        clearData();
        averagingPaused = false;
        Serial.println("Averaging Resumed");
      }
    }

    if (!averagingPaused) {
      total = total - readings[readIndex];
      readings[readIndex] = analogRead(pitchPin);
      total = total + readings[readIndex];
      readIndex = readIndex + 1;

      if (readIndex >= numReadings) {
        readIndex = 0;
      }

      noteAverage = total / numReadings;
      Serial.println(noteAverage);
    }
}

// a callback for key presses
TrellisCallback blink(keyEvent evt){
  // WHEN A BUTTON HAS BEEN PRESSED
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) {
    // set the pressed button to white
    trellis.pixels.setPixelColor(evt.bit.NUM, 255, 255, 255);
    trellis.pixels.show();
    if (evt.bit.NUM >= 0 && evt.bit.NUM <= 2 && evt.bit.NUM != currentPage) { // IF ROW 1, and page is changing update Trellis Page
        currentPage = evt.bit.NUM;
        trellisFlicker(currentPage);
    } else if (evt.bit.NUM >= 4 && evt.bit.NUM <= 15) { // if a standard button
        toggleArray(evt.bit.NUM, currentPage);
    }      
  }
    // WHEN A BUTTON HAS BEEN RELEASED
  else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    if (evt.bit.NUM >= 0 && evt.bit.NUM <= 3) { // if in row 1 reset to original color
      if (evt.bit.NUM == 0) {
          trellis.pixels.setPixelColor(evt.bit.NUM, 255, 0, 0);
          trellis.pixels.setPixelColor(1, 255, 255, 75);
          trellis.pixels.setPixelColor(2, 75, 255, 75);
          
      } else if (evt.bit.NUM == 1) {
          trellis.pixels.setPixelColor(evt.bit.NUM, 255, 255, 0);
          trellis.pixels.setPixelColor(0, 255, 75, 75);
          trellis.pixels.setPixelColor(2, 75, 255, 75);
          
      } else if (evt.bit.NUM == 2) {
        trellis.pixels.setPixelColor(evt.bit.NUM, 0, 255, 0);
        trellis.pixels.setPixelColor(0, 255, 75, 75);
        trellis.pixels.setPixelColor(1, 255, 255, 75);
          
      } else if (evt.bit.NUM == 3) {
        trellis.pixels.setPixelColor(evt.bit.NUM, 75, 75, 255);
      }
    } else { // if a standard button
      if (modeMemory[currentPage][evt.bit.NUM] == 0) {
        trellis.pixels.setPixelColor(evt.bit.NUM, 100, 100, 100);  
      } else {
        if (currentPage == 0) {
          trellis.pixels.setPixelColor(evt.bit.NUM, 255, 0, 0);
        } else if (currentPage == 1) {
          trellis.pixels.setPixelColor(evt.bit.NUM, 255, 255, 0);
        } else if (currentPage == 2) {
          trellis.pixels.setPixelColor(evt.bit.NUM, 0, 255, 0);
        }
      }
    }
    trellis.pixels.show(); //update display
  }
}
// updates colored buttons based on array values
void trellisFlicker(int currentPage) {
  for (int i = 4; i < trellis.pixels.numPixels(); i++) {
    if (modeMemory[currentPage][i] == 0) { // if value is zero
      trellis.pixels.setPixelColor(i, 100, 100, 100);
    } else if (modeMemory[currentPage][i]) { // if value is one
      if (currentPage == 0) {
        trellis.pixels.setPixelColor(i, 255, 0, 0);
      } else if (currentPage == 1) {
        trellis.pixels.setPixelColor(i, 255, 255, 0);  
      } else if (currentPage == 2) {
        trellis.pixels.setPixelColor(i, 0, 255, 0);
      }
    }
      trellis.pixels.show();
    }
}
// inverts value pointed to in array
void toggleArray(int buttonValue, int currentPage) { //not yet working
  if (modeMemory[currentPage][buttonValue] == 0) {
    modeMemory[currentPage][buttonValue] = 1;
  } else {
    modeMemory[currentPage][buttonValue] = 0;        
  }
}
// unused code to trigger all trellis buttons to the same color
void setTrellisButtons(int red, int green, int blue) {
  for(uint16_t i=4; i<trellis.pixels.numPixels(); i++) {
    trellis.pixels.setPixelColor(i, red, green, blue); // colors for initial flash
    trellis.pixels.show();
  }
}
// sent a three digit integer display code to Metro display
void toDisplay(int note) {
  // Send 3-digit integer display code to Metro
  Wire.beginTransmission(10); // metro address
  Wire.write((uint8_t*)&note, sizeof(note));
  Wire.endTransmission();
}
// activate a MIDI note
void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = { 0x09, 0x90 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOn);
}
// turn off MIDI note
void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = { 0x08, 0x80 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOff);
}
// compute current code mode
byte* getCurrentMode(int modeIndex) {
  if (modeIndex >= 0 && modeIndex < numModes){
    return modes[modeIndex];
  }
}
// compute size of an array pointed to by arr
int getArraySize(byte* arr) {
  int size = 0;
  while (arr[size] != '\0') {
    size++;
  }
  return size;
}
//unknown
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
}
// Yet unused theoretical pitch bending function
void pitchBend(byte channel, int value) {
  byte lowValue = value & 0x7F;
  byte highValue = value >> 7;
  midiEventPacket_t pitchBend = { 0x0E, 0xE0 | channel, lowValue, highValue };
  MidiUSB.sendMIDI(pitchBend);
}
// solve for note from mode and position
int computeNote(int modeCode,int pitchPin, int fretNumber) {
  // compute proper pitch from mode data and ribbon data
  int numOfNotes = modeLengths[modeCode];
  int fretIndex = map(noteAverage, 330, 690, 0, fretNumber); // calculate fret value
  lastFret = fretIndex;
  if (fretIndex >= 0 && fretIndex <= fretNumber) {
    int pitch = startNote + (12 * (fretIndex / numOfNotes)) + getCurrentMode(modeCode)[(fretIndex % numOfNotes)];
  return pitch; 
  }
}

void clearData() {
  for (int i = 0; i < numReadings; i++) {
    readings[i] = 0;
  }
  total = 0;
  noteAverage = 0;
}

void updateNote(int pitch) {
  if (pitch >= startNote) { // weed pitch errors
    if (pitch != lastNote) { // if note is different
      noteOff(0, lastNote, 0); //Kill the note
      Serial.println("note is different");
      noteOn(0, pitch, noteVelocity); // trigger note
      isOn = 1;
      lastRibbonTriggerTime = millis();
      MidiUSB.flush();
      lastNote = pitch; // record note
      toDisplay(pitch); // display on Metro
    }
  }
}
