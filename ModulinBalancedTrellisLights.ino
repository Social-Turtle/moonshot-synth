/*
TODO:
 - Develop function to make a gradient slide effect from notes + bends - start with computeNote - - Give this to a MoonShotter
 - Sometimes launches at G9 (MIDI 127) why? - signal noise issue with potentiometer. Let's solve that.
 - Chord pack development
 - general smoothness/bug hunting
 - accellerometer interfacing
 - led strip debugging
 */
// Primary Synth operates on Channel 0
// Drum Kit operates on Channel 1
// Chord Library/Arpeggiator operates on Channel 2

// LIBRARIES USED
#include <Adafruit_NeoPixel.h>
#include <MIDIUSB.h> //Midi Communication protocol
#include <Wire.h> // i2c protocol
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Adafruit_NeoTrellis.h" // Neotrellis library
#define Y_DIM 4 //number of rows of key
#define X_DIM 8 //number of columns of keys
#define LED_PIN     2    // Define the data pin connected to the LED strip
#define NUM_LEDS    29   // Define the number of LEDs in your strip
#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
// accellerometer sits at 0x62
// display @ 0x3C

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

//create a matrix of trellis panels
Adafruit_NeoTrellis t_array[Y_DIM/4][X_DIM/4] = { { Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F) }};
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, Y_DIM/4, X_DIM/4); //define Neotrellis object

// PIN ASSIGNMENTS
const int velocityPin = A1; // analog input for velocity potentiometer
const int pitchPin = A0; // the number of the ribbon potentiometer pin
const int bendPin = A2;

const unsigned long debounceDelay = 5; //length in ms
const unsigned long debounceNote = 50;

int startNote = 55; // the lowest note playable on the modulin's primary string
const int fretNumber = 15; // the number of notes the potentiometer can reach
int modeCode = 0; // scale mode we're in
int noteVelocity = 80; // note velocity output, later controlled by a fader

// PITCH COMPUTATION
byte keyRoots[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
byte major[7] = {0, 2, 4, 5, 7, 9, 11}; 
byte minor[7] = {0, 2, 3, 5, 7, 8, 10};
byte blues[6] = {0, 3, 5, 6, 7, 9};
byte pentatonic[5] = {0, 2, 4, 7, 9};
int numModes = 4; 
byte modeLengths[4] = {7, 7, 6, 5};
byte* modes[] = {major, minor, blues, pentatonic}; 
int bendVal;

// INITIALIZE AT GLOBAL
unsigned long lastButtonDebounceTime = 0;
unsigned long lastRibbonTriggerTime = 0;
int previousNote; 
int currentTime = millis();
int lastTime = 0;
byte isOn = 0;
int lastNote;
int currentPage = 0;
int lastFret;
int lastVelocity;
int toggleThreshold = 20; // threshold for velocitypot to trigger a note

const int numReadings = 10;
int readings[numReadings];
int readIndex = 0;
int total = 0;
int noteAverage = 0;

int bendAverage;
int bendTotal;
const int bendPoints = 32;
int bendReadings[bendPoints];
int bendIndex = 0;

boolean averagingPaused = false;
unsigned long lastDebounceTime = 0;

unsigned long tempo; // store tempo value
unsigned long lastTempoPressTime = 0; // store more recent tempo press
unsigned long nextBeat = 0;
int beatIndex = 0;
bool dontbother = false;
///////////////////////////////////////////////////////////////////

// TRELLIS DATA
byte modeMemory[4][32] = { // Button position memory bank
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // store drum machine data
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// - // - // - // - // - // - // - // - // - // Functions below here // - // - // - // - // - // - // - // - // - // - // - // - // - //

// inverts value pointed to in array
void toggleArray(int buttonValue, int currentPage) {
  if (modeMemory[currentPage][buttonValue] == 0) {
    modeMemory[currentPage][buttonValue] = 1;
  } else {
    modeMemory[currentPage][buttonValue] = 0;        
  }
}

TrellisCallback blink(keyEvent evt){ // Operational Trellis FSM
  if (evt.bit.EDGE == SEESAW_KEYPAD_EDGE_RISING) { // WHEN A BUTTON HAS BEEN PRESSED
    trellis.setPixelColor(evt.bit.NUM, 0x555555);   // set the pressed button to white
    if (evt.bit.NUM >= 0 && evt.bit.NUM <= 2 && evt.bit.NUM != currentPage) { // If we press a novel page button, update Trellis Page
        currentPage = evt.bit.NUM;
        trellisFlicker(currentPage);
    } else if (currentPage == 2 && evt.bit.NUM >= 4 && evt.bit.NUM <= 31) { // CONTROL SETTINGS FROM HERE:
        if (evt.bit.NUM == 8) { // Row 1, button 0 = - 1 semitone
          startNote--;
          trellis.setPixelColor(evt.bit.NUM, 0x550000); // RED
        } else if (evt.bit.NUM == 9) { // Row 1, button 1 = + 1 semitone
          startNote++;
          trellis.setPixelColor(evt.bit.NUM, 0x005500); // GREEN   
        } else if (evt.bit.NUM == 10) { // Row 1, button 2 = - 1 octave
          startNote -= 12;
          trellis.setPixelColor(evt.bit.NUM, 0x000055); // BLUE 
        } else if (evt.bit.NUM == 11) { // Row 1, button 3 = + 1 octave
          startNote += 12;
          trellis.setPixelColor(evt.bit.NUM, 0x550055); // PURPLE  
        } else if (evt.bit.NUM == 12) { // Row 1, button 4 = MODECODE - 1 IF AVAILABLE
          if (modeCode > 0) {
            modeCode--;
          }
          Serial.println(modeCode);
          trellis.setPixelColor(evt.bit.NUM, 0x005555); // CYAN
        } else if (evt.bit.NUM == 13) { // Row 1, button 5 = MODECODE + 1 IF AVAILABLE
          if (modeCode < 4) {
            modeCode++;
          }
          Serial.println(modeCode);
          trellis.setPixelColor(evt.bit.NUM, 0x552200); // ORANGE      
        }
    } else if (evt.bit.NUM > 3 && evt.bit.NUM <= 31) { // if a standard button and chords or beatbox
        toggleArray(evt.bit.NUM, currentPage);
    } else if (evt.bit.NUM == 3) { // Tempo button!
      dontbother = true;
      tempo = millis() - lastTempoPressTime;
      Serial.println(tempo);
      lastTempoPressTime = millis();
      nextBeat = lastTempoPressTime;
      beatIndex = 0;
    }
    trellis.show();
  }
    // WHEN A BUTTON HAS BEEN RELEASED
  else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    if (evt.bit.NUM >= 0 && evt.bit.NUM <= 3) { // if in row 1 reset to original color
      if (evt.bit.NUM == 0) { // set tab colors to page 0 - red
          trellis.setPixelColor(evt.bit.NUM, 0xFF0000);
          trellis.setPixelColor(1, 0xFFFF4B);
          trellis.setPixelColor(2, 0x4BFF4B);    
      } else if (evt.bit.NUM == 1) { // set tab colors to page 1 - yellow
          trellis.setPixelColor(evt.bit.NUM, 0xFFFF00);
          trellis.setPixelColor(0, 0xFF4B4B);
          trellis.setPixelColor(2, 0x4BFF4B);
      } else if (evt.bit.NUM == 2) { // set tab colors to page 2 - green
          trellis.setPixelColor(evt.bit.NUM, 0x00FF00);
          trellis.setPixelColor(0, 0xFF4B4B);
          trellis.setPixelColor(1, 0xFFFF4B);
      } else if (evt.bit.NUM == 3) { // if it's the tempo button, just keep it blue
          trellis.setPixelColor(evt.bit.NUM, 0x0000FF);
      }
    } else { // if a standard button
      if (modeMemory[currentPage][evt.bit.NUM] == 0) {
        trellis.setPixelColor(evt.bit.NUM, 0x333333);  
      } else {
        if (currentPage == 0) {
          trellis.setPixelColor(evt.bit.NUM, 0x640000);
        } else if (currentPage == 1) {
          trellis.setPixelColor(evt.bit.NUM, 0x484800);
        } else if (currentPage == 2) {
          trellis.setPixelColor(evt.bit.NUM, 0x006400);
        }
      }
    }
    trellis.show(); //update display
  }
}

// updates colored buttons based on array values
void trellisFlicker(int currentPage) {
  for (int i = 4; i < 32; i++) {
    if (modeMemory[currentPage][i] == 0) { // if value is zero
      trellis.setPixelColor(i, 0x333333); // base color when set to a new page
    } else { // if value is one
      if (currentPage == 0) {
        trellis.setPixelColor(i, 0x640000);
      } else if (currentPage == 1) {
        trellis.setPixelColor(i, 0x484800);  
      } else if (currentPage == 2) {
        trellis.setPixelColor(i, 0x006400);
      }
    }
      trellis.show();
    }
}

void toDisplay(String text) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(text);
  display.display();
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

// compute current mode state
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

//Allows us to interact with our DAW if we deem it worthwhile
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
}

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

void updateNote(int pitch) { // add a check for 127 spikes
  if (pitch >= startNote) { // weed pitch errors
    if (pitch != lastNote) { // if note is different
      noteOff(0, lastNote, 0); //Kill the note
      //Serial.println("note is different");
      noteOn(0, pitch, noteVelocity); // trigger note
      isOn = 1;
      lastRibbonTriggerTime = millis();
      MidiUSB.flush();
      lastNote = pitch; // record note
      toDisplay("Diff Note!"); // display on Metro
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Setup Starts Here

void setup() {
  Serial.begin(9600);
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Wire.setClock(50000); // match Metro Mini i2c clock of 50kHz
  
  Serial.println("Setup Begin");
  //strip.begin();  // Initialize NeoPixel strip
  //strip.show();   // Initialize all pixels to 'off'

  for (int thisReading = 0; thisReading < numReadings; thisReading++) { // initialize averaging function
    readings[thisReading] = 0;
  }

  if(!trellis.begin()){ // start trellis
    Serial.println("could not start trellis");
    while(1) delay(1);
  }
  else{
    Serial.println("trellis started");
  }

  //activate all keys and set callbacks
  for(int i=0; i<32; i++){
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_RISING);
    trellis.activateKey(i, SEESAW_KEYPAD_EDGE_FALLING);
    trellis.registerCallback(i, blink);
  }

  //do a little animation to show we're on
  for(uint16_t i=0; i<32; i++) {
    trellis.setPixelColor(i, 0x320096); // colors for initial flash
    trellis.show();
    delay(50);
  }
  for(uint16_t i=0; i<32; i++) {
    trellis.setPixelColor(i,0x333333); // initial white spread
    trellis.show();
    delay(50);
  }
  trellis.setPixelColor(0, 0xFF0000); // base light red
  trellis.setPixelColor(1, 0xFFFF32); // base light yellow
  trellis.setPixelColor(2, 0x32FF32); // base light green
  trellis.setPixelColor(3, 0x3232FF); // base light blue
  trellis.show();

  // initialize the pushbutton pins as an inputs:
  pinMode(pitchPin, INPUT);
  pinMode(velocityPin, INPUT);
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Loop Starts Here


void loop() {
  //toDisplay("Yo Mama");
  if (millis() - nextBeat >= tempo/8 && dontbother) { // if we've reached one eighth
    nextBeat = millis();
    Serial.println(beatIndex);
    if (modeMemory[0][8 + beatIndex] == 1) {
      Serial.println("Hat");
      noteOn(1, 42, 60); // closed high hat
    }
    if (modeMemory[0][16 + beatIndex] == 1) {
      Serial.println("Snare");
      noteOn(1, 38, 60);
    }
    if (modeMemory[0][24 + beatIndex] == 1) {
      Serial.println("Kick");
      noteOn(1, 36, 60);
    }
    MidiUSB.flush();
    beatIndex++;     // do stuff on the eighth
    if (beatIndex > 7) { // update the index of the 1/8th we're on
      beatIndex = 0; // trip back to zero if we step over 7
    }
  }

  noteVelocity = map(analogRead(velocityPin), 330, 690, 0, 128);   // Convert velocitypin into a velocity value
  trellis.read(); // Check the trellis

  if ((millis() - lastButtonDebounceTime) > debounceDelay) { // if it's been 5 ms since the last update
    if (noteVelocity != lastVelocity) { // if the velocity has changed, record when
      lastButtonDebounceTime = millis();
      lastVelocity = noteVelocity;
    } 
      if (noteVelocity < toggleThreshold && isOn == 1) { // if the string is released but a note is playing: 
        noteOff(0, lastNote, 0); //Kill the note
        MidiUSB.flush();
        toDisplay(""); // turn off display if no note is present
        isOn = 0;
    } else if (noteVelocity > toggleThreshold && isOn == 0) { // the string is pressed and no note is playing
        noteOn(0, computeNote(modeCode, pitchPin, fretNumber), noteVelocity); //play the note
        lastRibbonTriggerTime = millis();
        MidiUSB.flush();
        toDisplay("Playing");
        isOn = 1;
    }
  }

  if (noteVelocity > toggleThreshold) { // what case is this??? - - - - - - - - - - - -  - - -  - **
    if (millis() - lastRibbonTriggerTime > debounceNote) {
      updateNote(computeNote(modeCode, pitchPin, fretNumber));
    }
  }
    lastVelocity = noteVelocity;
  if (noteVelocity < toggleThreshold) { // note is triggered
    if (!averagingPaused) {
      averagingPaused = true;
      //Serial.println("Averaging Paused");
    }
  } else { // string released
    if (averagingPaused) {
      clearData();
      averagingPaused = false;
      //Serial.println("Averaging Resumed");
    }
  }

  if (!averagingPaused) {
    total = total - readings[readIndex];
    readings[readIndex] = analogRead(pitchPin);
    total = total + readings[readIndex];
    readIndex++;
    if (readIndex >= numReadings) {
      readIndex = 0;
    }
    noteAverage = total / numReadings; // **skews data low at start. How to fix this?
  }

    // Running average for note bend data
    bendTotal = bendTotal - bendReadings[bendIndex];
    bendReadings[bendIndex] = analogRead(bendPin);
    bendTotal = bendTotal + bendReadings[bendIndex];
    bendIndex++;
    if (bendIndex >= bendPoints) {
      bendIndex = 0;
    }

    bendAverage = bendTotal / bendPoints;
    bendVal = map(bendAverage, 2, 1020, 0, 16383);
    if (bendVal > 8192 + 1500 || bendVal < 8192 - 1500) { // If pot is distant enough to trigger a bend . - - - - - - - - - - - - This methodology spams the pitch wheel, and could use to be updated to only send bend values when either 1: the note played changes or 2: the bend value changes outside of standard noise.
      pitchBend(0, bendVal);
    }

  int sensorValue = analogRead(pitchPin);  // Read analog input value
  int numLEDsToLight = map(sensorValue, 330, 650, 0, NUM_LEDS);  // Map input value to the number of LEDs
  //Serial.println(numLEDsToLight);
  // Turn off all LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);  // Set color to 'off' (Black)
  }
  strip.show();  // Update the strip to turn off all LEDs
  
  // Turn on LEDs based on the mapped value
  for (int i = 0; i < numLEDsToLight; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 80));  // Set color to Red (adjust as needed)
  }
  strip.show();  // Update the strip to show the illuminated LEDs
}
