// Primary Synth operates on Channel 0
// Drum Kit operates on Channel 1
// Chord Library/Arpeggiator operates on Channel 2

// LIBRARIES USED
#include <Adafruit_NeoPixel.h> // Library for controlling the NeoTrellis
#include <MIDIUSB.h> //Midi Communication protocol
#include <Wire.h> // i2c protocol
#include "Adafruit_NeoTrellis.h" // Neotrellis library
#include <SoftwareSerial.h>


#define Y_DIM 4 //number of rows of key
#define X_DIM 8 //number of columns of keys
#define OLED_RESET 4

SoftwareSerial mySerial(0,1);

//create a matrix of trellis panels
Adafruit_NeoTrellis t_array[Y_DIM/4][X_DIM/4] = { { Adafruit_NeoTrellis(0x2E), Adafruit_NeoTrellis(0x2F) }};
Adafruit_MultiTrellis trellis((Adafruit_NeoTrellis *)t_array, Y_DIM/4, X_DIM/4); //define Neotrellis object

// PIN ASSIGNMENTS
const int velocityPin = A1; // analog input for velocity potentiometer
const int pitchPin = A0; // the number of the ribbon potentiometer pin
const int bendPin = A2;

const unsigned long debounceDelay = 5; //length in ms
const unsigned long debounceNote = 50;


// PITCH COMPUTATION
int startNote = 55; // the lowest note playable on the modulin's primary string
const int fretNumber = 16; // the number of notes the potentiometer can reach
int modeCode = 0; // scale mode we're in
int noteVelocity = 80; // note velocity output, later controlled by a fader
byte major[7] = {0, 2, 4, 5, 7, 9, 11}; // Keep track of our scales! (7 Note Scales only)
byte minor[7] = {0, 2, 3, 5, 7, 8, 10};
byte harmonicMinor[7] = {0, 2, 3, 5, 7, 8, 11};
int numModes = 3; // How many modes we have
int modeLengths = 7; // all modes are of length 7
byte* modes[] = {major, minor, harmonicMinor}; 
int bendVal;

// Debouncing
unsigned long lastButtonDebounceTime = 0; // Initialize time of button presses
unsigned long lastRibbonTriggerTime = 0;  // Initialize time of ribbon presses
int currentTime = millis();
int lastTime = 0;
byte isOn = 0;                            // Tracks if a note is actively playing
int lastNote;                             // Last note played
int currentPage = 0;
int lastFret;                             // Last fret position (to check if a note has changed)
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
        switch (currentPage) {
          case 0 : mySerial.print('a'); break;
          case 1 : mySerial.print('b'); break;
          case 2 : mySerial.print('c'); break;
        }
        trellisFlicker(currentPage);
    } else if (evt.bit.NUM == 7){ //Drum mute button
        toggleArray(evt.bit.NUM, 0);
        if (modeMemory[0][7] == 0){
          mySerial.print('q');
          trellis.setPixelColor(evt.bit.NUM, 0xFFA500);
        }
        else {
          mySerial.print('r');
          trellis.setPixelColor(evt.bit.NUM, 0xFF0000);
        }
    } else if (currentPage == 2 && evt.bit.NUM >= 4 && evt.bit.NUM <= 31) { // CONTROL SETTINGS FROM HERE:
        if (evt.bit.NUM == 8) { // Row 1, button 0 = - 1 semitone
          startNote--;
          mySerial.print('z');
          //trellis.setPixelColor(evt.bit.NUM, 0x550000); // RED
        } else if (evt.bit.NUM == 9) { // Row 1, button 1 = + 1 semitone
          startNote++;
          mySerial.print('y');
          //trellis.setPixelColor(evt.bit.NUM, 0x005500); // GREEN   
        } else if (evt.bit.NUM == 10) { // Row 1, button 2 = - 1 octave
          startNote -= 12;
          mySerial.print('x');
          //trellis.setPixelColor(evt.bit.NUM, 0x000055); // BLUE 
        } else if (evt.bit.NUM == 11) { // Row 1, button 3 = + 1 octave
          startNote += 12;
          mySerial.print('w');
          //trellis.setPixelColor(evt.bit.NUM, 0x550055); // PURPLE  
        } else if (evt.bit.NUM == 12) { // Row 1, button 4 = MODECODE - 1 IF AVAILABLE
          if (modeCode > 0) {
            mySerial.print('v'); // Decreased
            modeCode--;
          } else {
            mySerial.print('u'); // Can't decrease
          }
          //trellis.setPixelColor(evt.bit.NUM, 0x005555); // CYAN
        } else if (evt.bit.NUM == 13) { // Row 1, button 5 = MODECODE + 1 IF AVAILABLE
          if (modeCode < 4) {
            modeCode++;
            mySerial.print('t'); // Increased
          } else {
            mySerial.print('s'); // Can't increase
          }
          //trellis.setPixelColor(evt.bit.NUM, 0x552200); // ORANGE      
        } else if (evt.bit.NUM == 16) {
            mySerial.print('#'); // Red
        } else if (evt.bit.NUM == 17) {
            mySerial.print('&'); // Orange
        } else if (evt.bit.NUM == 18) {
            mySerial.print(')'); // Yellow
        } else if (evt.bit.NUM == 19) {
            mySerial.print('$'); // Green
        } else if (evt.bit.NUM == 20) {
            mySerial.print('('); // Cyan
        } else if (evt.bit.NUM == 21) {
            mySerial.print('%'); // Blue
        } else if (evt.bit.NUM == 22) {
            mySerial.print('^'); // Purple
        } else if (evt.bit.NUM == 23) {
            mySerial.print('*'); // White
        } else if (evt.bit.NUM == 24) {
            mySerial.print('+'); // White
        } else if (evt.bit.NUM == 25) {
            mySerial.print('+'); // White
        } else if (evt.bit.NUM == 26) {
            mySerial.print('+'); // White
        } else if (evt.bit.NUM == 27) {
            mySerial.print('+'); // White
        } else if (evt.bit.NUM == 28) {
            mySerial.print('-'); // White
        } else if (evt.bit.NUM == 29) {
            mySerial.print('-'); // White
        } else if (evt.bit.NUM == 30) {
            mySerial.print('-'); // White
        } else if (evt.bit.NUM == 31) {
            mySerial.print('-'); // White
        }
    } else if (currentPage == 1 && evt.bit.NUM > 7 && evt.bit.NUM <= 31) { // CHORD BOX SETTINGS
        trellis.setPixelColor(evt.bit.NUM, 0xFFFF00); //YELLOW
        int buttonRow = evt.bit.NUM / 8;
        int buttonColumn = evt.bit.NUM % 8;
        //arrangeChords(buttonRow, buttonColumn, true);
                
    } else if (evt.bit.NUM > 3 && evt.bit.NUM <= 31) { // if a standard button and chords or beatbox
        toggleArray(evt.bit.NUM, currentPage);
    } else if (evt.bit.NUM == 3) { // Tempo button!
      dontbother = true;
      tempo = millis() - lastTempoPressTime;
      lastTempoPressTime = millis();
      nextBeat = lastTempoPressTime;
      beatIndex = 0;
    }
    trellis.show();
  }
    // WHEN A BUTTON HAS BEEN RELEASED
  else if(evt.bit.EDGE == SEESAW_KEYPAD_EDGE_FALLING) {
    if (evt.bit.NUM >= 0 && evt.bit.NUM <= 7) { // if in row 1 reset to original color
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
    } else if (currentPage == 1 && evt.bit.NUM > 7 && evt.bit.NUM <= 31) { // If a standard chord button
        /*
        There are four rows indexed from 0, 8 columns indexed from 0. We can find out which column we're in by
        modding the button number by 8. We can find out which row we're in by dividing the button number by 8 (using integer division).
        */
        int buttonRow = evt.bit.NUM / 8;
        int buttonColumn = evt.bit.NUM % 8;
        //arrangeChords(buttonRow, buttonColumn, false);
        trellis.setPixelColor(evt.bit.NUM, 0x333333);
        
    } else { // if a standard button - after pressed
      if (evt.bit.NUM > 7 && modeMemory[currentPage][evt.bit.NUM] == 0 && currentPage == 0) {
        trellis.setPixelColor(evt.bit.NUM, 0x333333);  
      } else {
        if (currentPage == 0) {
          trellis.setPixelColor(evt.bit.NUM, 0x640000);
        } else if (currentPage == 2) { // Settings button colors go here
            if (evt.bit.NUM == 8) { // Row 1, button 0 = - 1 semitone
            trellis.setPixelColor(evt.bit.NUM, 0x550000); // RED
          } else if (evt.bit.NUM == 9) { // Row 1, button 1 = + 1 semitone
            trellis.setPixelColor(evt.bit.NUM, 0x005500); // GREEN   
          } else if (evt.bit.NUM == 10) { // Row 1, button 2 = - 1 octave
            trellis.setPixelColor(evt.bit.NUM, 0x000055); // BLUE 
          } else if (evt.bit.NUM == 11) { // Row 1, button 3 = + 1 octave
            trellis.setPixelColor(evt.bit.NUM, 0x550055); // PURPLE  
          } else if (evt.bit.NUM == 12) { // Row 1, button 4 = MODECODE - 1 IF AVAILABLE
            trellis.setPixelColor(evt.bit.NUM, 0x005555); // CYAN
          } else if (evt.bit.NUM == 13) { // Row 1, button 5 = MODECODE + 1 IF AVAILABLE
            trellis.setPixelColor(evt.bit.NUM, 0x552200); // ORANGE      
          } else if (evt.bit.NUM == 16) {
              trellis.setPixelColor(evt.bit.NUM, 0xAA0000); // Red
          } else if (evt.bit.NUM == 17) {
              trellis.setPixelColor(evt.bit.NUM, 0xAA5500); // Orange
          } else if (evt.bit.NUM == 18) {
              trellis.setPixelColor(evt.bit.NUM, 0xAAAA00); // Yellow
          } else if (evt.bit.NUM == 19) {
              trellis.setPixelColor(evt.bit.NUM, 0x00AA00); // Green
          } else if (evt.bit.NUM == 20) {
              trellis.setPixelColor(evt.bit.NUM, 0x00AAAA); // Cyan
          } else if (evt.bit.NUM == 21) {
              trellis.setPixelColor(evt.bit.NUM, 0x0000AA); // Blue
          } else if (evt.bit.NUM == 22) {
              trellis.setPixelColor(evt.bit.NUM, 0xAA00AA); // Purple
          } else if (evt.bit.NUM == 23) {
              trellis.setPixelColor(evt.bit.NUM, 0x0AAAAAA); // White
          } else if (evt.bit.NUM == 24) {
              trellis.setPixelColor(evt.bit.NUM, 0x033AA33); // Go
          } else if (evt.bit.NUM == 25) {
              trellis.setPixelColor(evt.bit.NUM, 0x033AA33); // Go
          } else if (evt.bit.NUM == 26) {
              trellis.setPixelColor(evt.bit.NUM, 0x033AA33); // Go
          } else if (evt.bit.NUM == 27) {
              trellis.setPixelColor(evt.bit.NUM, 0x033AA33); // Go
          } else if (evt.bit.NUM == 28) {
              trellis.setPixelColor(evt.bit.NUM, 0x0AA3333); // Stop
          } else if (evt.bit.NUM == 29) {
              trellis.setPixelColor(evt.bit.NUM, 0x0AA3333); // Stop
          } else if (evt.bit.NUM == 30) {
              trellis.setPixelColor(evt.bit.NUM, 0x0AA3333); // Stop
          } else if (evt.bit.NUM == 31) {
              trellis.setPixelColor(evt.bit.NUM, 0x0AA3333); // Stop
          }
        }
      }
    }
    trellis.show(); //update display
  }
}

// updates colored buttons based on array values
void trellisFlicker(int currentPage) {
  for (int i = 8; i < 32; i++) {
    if (currentPage == 0) {
      if (modeMemory[currentPage][i] == 0) {
        trellis.setPixelColor(i, 0x333333); // base color when set to a new page
      } else {
        trellis.setPixelColor(i, 0x640000); // drum active here
      }
    } else if (currentPage == 1) {
      trellis.setPixelColor(i, 0x333333); // base color when set to a new page
    } else if (currentPage == 2) {
         if (i == 8) { // Row 1, button 0 = - 1 semitone
            trellis.setPixelColor(i, 0x771111); // RED
          } else if (i == 9) { // Row 1, button 1 = + 1 semitone
            trellis.setPixelColor(i, 0x117711); // GREEN   
          } else if (i == 10) { // Row 1, button 2 = - 1 octave
            trellis.setPixelColor(i, 0x111177); // BLUE 
          } else if (i == 11) { // Row 1, button 3 = + 1 octave
            trellis.setPixelColor(i, 0x771177); // PURPLE  
          } else if (i == 12) { // Row 1, button 4 = MODECODE - 1 IF AVAILABLE
            trellis.setPixelColor(i, 0x117777); // CYAN
          } else if (i == 13) { // Row 1, button 5 = MODECODE + 1 IF AVAILABLE
            trellis.setPixelColor(i, 0x773311); // ORANGE      
          } else if (i == 14) { // Row 1, button 5 = MODECODE + 1 IF AVAILABLE
            trellis.setPixelColor(i, 0x000000); // unused      
          } else if (i == 15) { // Row 1, button 5 = MODECODE + 1 IF AVAILABLE
            trellis.setPixelColor(i, 0x000000); // unused      
          } else if (i == 16) {
              trellis.setPixelColor(i, 0xFF0000); // Red
          } else if (i == 17) {
              trellis.setPixelColor(i, 0xFF8800); // Orange
          } else if (i == 18) {
              trellis.setPixelColor(i, 0xFFFF00); // Yellow
          } else if (i == 19) {
              trellis.setPixelColor(i, 0x00FF00); // Green
          } else if (i == 20) {
              trellis.setPixelColor(i, 0x00FFFF); // Cyan
          } else if (i == 21) {
              trellis.setPixelColor(i, 0x0000FF); // Blue
          } else if (i == 22) {
              trellis.setPixelColor(i, 0xFF00FF); // Purple
          } else if (i == 23) {
              trellis.setPixelColor(i, 0x0FFFFFF); // White
          } else if (i == 24) {
              trellis.setPixelColor(i, 0x033AA33); // Go
          } else if (i == 25) {
              trellis.setPixelColor(i, 0x033AA33); // Go
          } else if (i == 26) {
              trellis.setPixelColor(i, 0x033AA33); // Go
          } else if (i == 27) {
              trellis.setPixelColor(i, 0x033AA33); // Go
          } else if (i == 28) {
              trellis.setPixelColor(i, 0x0AA3333); // Stop
          } else if (i == 29) {
              trellis.setPixelColor(i, 0x0AA3333); // Stop
          } else if (i == 30) {
              trellis.setPixelColor(i, 0x0AA3333); // Stop
          } else if (i == 31) {
              trellis.setPixelColor(i, 0x0AA3333); // Stop
          }
    } else {
      
    }
//    if (modeMemory[currentPage][i] == 0) { // if value is zero
//      trellis.setPixelColor(i, 0x333333); // base color when set to a new page
//    } else { // if value is one
//      if (currentPage == 0) {
//        trellis.setPixelColor(i, 0x640000);
//      }
//    }
    trellis.show();
  }
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

char TwelvetoChar(int num) {
  num = num % 12;
  switch (num) {
    case 0 : return '0';
    case 1 : return '1';
    case 2 : return '2';
    case 3 : return '3';
    case 4 : return '4';
    case 5 : return '5';
    case 6 : return '6';
    case 7 : return '7';
    case 8 : return '8';
    case 9 : return '9';
    case 10 : return '!';
    case 11 : return '@';
  }
}

// solve for note from mode and position
int computeNote(int modeCode,int pitchPin, int fretNumber) {
  // compute proper pitch from mode data and ribbon data
  int numOfNotes = modeLengths;
  int fretIndex = map(noteAverage, 270, 710, 0, fretNumber); // calculate fret value
  lastFret = fretIndex;
  if (fretIndex >= 0 && fretIndex <= fretNumber) {
    int pitch = startNote + (12 * (fretIndex / numOfNotes)) + getCurrentMode(modeCode)[(fretIndex % numOfNotes)];
    mySerial.write(TwelvetoChar(pitch));
  return pitch; 
  }
}

void arrangeChords(int Y, int X, bool buttonPress) { // button x position, button y position, 
    if(buttonPress) {
      if (Y == 1) {
        noteOn(2, startNote - 12 + getCurrentMode(modeCode)[X % 7], 60);
        noteOn(2, startNote - 12 + getCurrentMode(modeCode)[(X+2) % 7], 60);
        noteOn(2, startNote - 12 + getCurrentMode(modeCode)[(X+4) % 7], 60);
        MidiUSB.flush();
       
      } else if (Y == 2) {
        noteOn(2, startNote - 12 + getCurrentMode(modeCode)[X % 7], 60);
        noteOn(2, startNote - 12 + getCurrentMode(modeCode)[(X+2) % 7], 60);
        noteOn(2, startNote - 12 + getCurrentMode(modeCode)[(X+4) % 7], 60);
        noteOn(2, startNote - 12 + getCurrentMode(modeCode)[(X+6) % 7], 60);
        MidiUSB.flush();
          
      } else if (Y == 3) {
        noteOn(2, startNote - 12 + getCurrentMode(modeCode)[X] + 7, 60);
        noteOn(2, startNote - 12 + getCurrentMode(modeCode)[X] + 11, 60);
        noteOn(2, startNote - 12 + getCurrentMode(modeCode)[X] + 14, 60);
        noteOn(2, startNote - 12 + getCurrentMode(modeCode)[X] + 17, 60);
        MidiUSB.flush();
        // fancy wildness (X+7 Semitones Major b7)
      }
    } else if (!buttonPress) {
      if (Y == 1) {
        noteOff(2, startNote - 12 + getCurrentMode(modeCode)[X % 7], 0);
        noteOff(2, startNote - 12 + getCurrentMode(modeCode)[(X+2) % 7], 0);
        noteOff(2, startNote - 12 + getCurrentMode(modeCode)[(X+4) % 7], 0);
        MidiUSB.flush();
      } else if (Y == 2) {
        noteOff(2, startNote - 12 + getCurrentMode(modeCode)[X % 7], 0);
        noteOff(2, startNote - 12 + getCurrentMode(modeCode)[(X+2) % 7], 0);
        noteOff(2, startNote - 12 + getCurrentMode(modeCode)[(X+4) % 7], 0);
        noteOff(2, startNote - 12 + getCurrentMode(modeCode)[(X+6) % 7], 0);
        MidiUSB.flush();
      } else if (Y == 3) {
        noteOff(2, startNote - 12 + getCurrentMode(modeCode)[X] + 7 + 0, 0);
        noteOff(2, startNote - 12 + getCurrentMode(modeCode)[X] + 11, 0);
        noteOff(2, startNote - 12 + getCurrentMode(modeCode)[X] + 14, 0);
        noteOff(2, startNote - 12 + getCurrentMode(modeCode)[X] + 17, 0);
        MidiUSB.flush();
        // fancy wildness (X+7 Semitones Major b7)
      }
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
      noteOn(0, pitch, noteVelocity); // trigger note
      isOn = 1;
      lastRibbonTriggerTime = millis();
      MidiUSB.flush();
      lastNote = pitch; // record note
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Setup Starts Here

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);
  Wire.begin();
  //display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  Wire.setClock(50000); // match Metro Mini i2c clock of 50kHz
  
  Serial.println("Setup Begin");
  
  // initialize the Sensor pins as an inputs:
  pinMode(pitchPin, INPUT);
  pinMode(velocityPin, INPUT);
  
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
  trellis.show();
  delay(50);
  trellis.setPixelColor(1, 0xFFFF32); // base light yellow
  trellis.show();
  delay(50);
  trellis.setPixelColor(2, 0x32FF32); // base light green
  trellis.show();
  delay(50);
  trellis.setPixelColor(3, 0x0000FF); // base light blue
  trellis.show();
  delay(50);
  trellis.setPixelColor(4, 0x000000);
  trellis.show();
  delay(50);
  trellis.setPixelColor(5, 0x000000);
  trellis.show();
  delay(50);
  trellis.setPixelColor(6, 0x000000);
  trellis.show();
  delay(50);
  trellis.setPixelColor(7,0xFFA500);
  trellis.show();

}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - Loop Starts Here


void loop() {
  if (millis() - nextBeat >= tempo/8 && dontbother) { // if we've reached one eighth
    nextBeat = millis();
    if (modeMemory[0][7] == 1){
      if (modeMemory[0][8 + beatIndex] == 1) {
        noteOn(1, 42, 60); // closed high hat
      }
      if (modeMemory[0][16 + beatIndex] == 1) {
        noteOn(1, 38, 60);
      }
      if (modeMemory[0][24 + beatIndex] == 1) {
        noteOn(1, 36, 60);
      }
    }
    MidiUSB.flush();
    beatIndex++;     // do stuff on the eighth
    noteOff(1, 42, 60);
    noteOff(1, 38, 60);
    noteOff(1, 36, 60);
    if (beatIndex > 7) { // update the index of the 1/8th we're on
      beatIndex = 0; // trip back to zero if we step over 7
    }
  }

  noteVelocity = map(analogRead(velocityPin), 1000, 100, 0, 128);   // Convert velocitypin into a velocity value
  trellis.read(); // Check the trellis

  if ((millis() - lastButtonDebounceTime) > debounceDelay) { // if it's been 5 ms since the last update
    if (noteVelocity != lastVelocity) { // if the velocity has changed, record when
      lastButtonDebounceTime = millis();
      lastVelocity = noteVelocity;
    } 
      if (noteVelocity < toggleThreshold && isOn == 1) { // if the string is released but a note is playing: 
        noteOff(0, lastNote, 0); //Kill the note
        MidiUSB.flush();
        //toDisplay(""); // turn off display if no note is present
        isOn = 0;
    } else if (noteVelocity > toggleThreshold && isOn == 0) { // the string is pressed and no note is playing
        //noteOn(0, computeNote(modeCode, pitchPin, fretNumber), noteVelocity); //play the note
        lastRibbonTriggerTime = millis();
        MidiUSB.flush();
        //toDisplay("Playing");
        isOn = 1;
    }
  }

  if (noteVelocity > toggleThreshold) {
    if (millis() - lastRibbonTriggerTime > debounceNote) {
      updateNote(computeNote(modeCode, pitchPin, fretNumber));
    }
  }
    lastVelocity = noteVelocity;
  if (noteVelocity < toggleThreshold) { // note is triggered
    if (!averagingPaused) {
      averagingPaused = true;
    }
  } else { // string released
    if (averagingPaused) {
      clearData();
      averagingPaused = false;
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
    noteAverage = total / numReadings;
  }

//    // Running average for note bend data - - - - - - - - - - - - - - - Bend Functionality Not in Use
//    bendTotal = bendTotal - bendReadings[bendIndex];
//    bendReadings[bendIndex] = analogRead(bendPin);
//    bendTotal = bendTotal + bendReadings[bendIndex];
//    bendIndex++;
//    if (bendIndex >= bendPoints) {
//      bendIndex = 0;
//    }
//
//    bendAverage = bendTotal / bendPoints;
//    bendVal = map(bendAverage, 2, 1020, 0, 16383);
//    if (bendVal > 8192 + 1500 || bendVal < 8192 - 1500) { // If pot is distant enough to trigger a bend . - - - - - - - - - - - - This methodology spams the pitch wheel, and could use to be updated to only send bend values when either 1: the note played changes or 2: the bend value changes outside of standard noise.
//      //pitchBend(0, bendVal);
//    }

//  int sensorValue = analogRead(pitchPin); // Read analog input value - - - - - - - - - - LED Strip Functionality Not in Use
//  int numLEDsToLight = map(sensorValue, 330, 650, 0, NUM_LEDS);  // Map input value to the number of LEDs
//  //Serial.println(numLEDsToLight);
//  // Turn off all LEDs
//  for (int i = 0; i < NUM_LEDS; i++) {
//    strip.setPixelColor(i, 0);  // Set color to 'off' (Black)
//  }
//  strip.show();  // Update the strip to turn off all LEDs
//  
//  // Turn on LEDs based on the mapped value
//  for (int i = 0; i < numLEDsToLight; i++) {
//    strip.setPixelColor(i, strip.Color(0, 0, 80));  // Set color to Red (adjust as needed)
//  }
//  strip.show();  // Update the strip to show the illuminated LEDs
}
