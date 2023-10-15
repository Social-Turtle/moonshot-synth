/* Modulin Note Display Code
 * 
 */
 
#include <Wire.h>
// Arrays of pin numbers for anode (+) wires and cathode (-) wires.
const byte ANODE_PINS[8] = {13, 12, 11, 10, 9, 8, 7, 6};
const byte CATHODE_PINS[8] = {A3, A2, A1, A0, 5, 4, 3, 2};
//initializing graphic grids
byte naturalgrid[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
  };
byte flatgrid[8][8] = {
    {0, 0, 0, 0, 0, 0, 1, 0},
    {0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
  };
byte sharpgrid[8][8] = {
    {0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
  };
byte blankgrid[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
  };
byte agrid[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {1, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0}
  };
byte bgrid[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0}
  };
byte cgrid[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0}
  };
byte dgrid[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0}
  };
byte egrid[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 0, 0, 0}
  };
byte fgrid[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0}
  };
byte ggrid[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 0, 0, 0, 0, 0},
    {1, 0, 1, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {1, 0, 0, 0, 1, 0, 0, 0},
    {0, 1, 1, 1, 0, 0, 0, 0}
  };

// default 8x8 grid
//    {0, 0, 0, 0, 0, 0, 0, 0},
//    {0, 0, 0, 0, 0, 0, 0, 0},
//    {0, 0, 0, 0, 0, 0, 0, 0},
//    {0, 0, 0, 0, 0, 0, 0, 0},
//    {0, 0, 0, 0, 0, 0, 0, 0},
//    {0, 0, 0, 0, 0, 0, 0, 0},
//    {0, 0, 0, 0, 0, 0, 0, 0},
//    {0, 0, 0, 0, 0, 0, 0, 0}
    
byte (*noteGrids[12])[8][8] = {&cgrid, &dgrid, &dgrid, &egrid, &egrid, &fgrid, &ggrid, &ggrid, &agrid, &agrid, &bgrid, &bgrid}; //Each note 
byte (*accidentalGrids[3])[8][8] = {&flatgrid, &naturalgrid, &sharpgrid}; //Sharp, Natural, Flat
int note = 0;
byte (*maingrid)[8][8] = &naturalgrid;
byte (*accidentalgrid)[8][8] = &naturalgrid;
int receivedNote = 0;
int receivedValue = 0;

void setup() {

  Wire.begin(10); //begin i2c communication protocol
  Wire.setClock(50000);
  Wire.onReceive(receiveEvent);
  //setup physical LED control systems
  for (byte i = 0; i < 8; i++) {
    pinMode(ANODE_PINS[i], OUTPUT);
    pinMode(CATHODE_PINS[i], OUTPUT);
    digitalWrite(CATHODE_PINS[i], HIGH);
    digitalWrite(ANODE_PINS[i], HIGH);
  } 
  Serial.begin(9600);
}


void loop() {
    
  // use 'static' so that it retains its value between successive calls of loop()
  static byte ledOn[8][8];
  if (receivedNote == 999) { // offCase
    maingrid = &naturalgrid;
    accidentalgrid = &naturalgrid;

  } else if (receivedNote != 0) {
    Serial.println(receivedNote);
    int index = receivedNote % 12;         // Get the remainder of division by 12
    int noteIndex = (index + 12) % 12;     // Ensure positive index values
    maingrid = noteGrids[noteIndex];
    if (noteIndex == 1 ||noteIndex == 3 || noteIndex == 6 || noteIndex == 8) { //if note is flat
      accidentalgrid = &flatgrid;
      } else {
      accidentalgrid = &naturalgrid;
    }
  }
  
  mergeGlyphs(ledOn, *maingrid, *accidentalgrid);
  display(ledOn);
}

void display(byte pattern[8][8]) {
  //iterate through [8][8] arrays, set any 1s to "ON"
  //otherwise, leave off (uses multiplexing)
  for(int r = 0; r < 8; r++) {
    digitalWrite(ANODE_PINS[r],LOW);
    for( int c = 0; c < 8; c++ ) {
      if( pattern[r][c] == 1 ) {
        digitalWrite(CATHODE_PINS[c],LOW);
      } else {
        digitalWrite(CATHODE_PINS[c],HIGH);
      }
    }
    delay(1); //Adjust the delay to control brightness/flickering
    digitalWrite(ANODE_PINS[r],HIGH);
  }
}

void mergeGlyphs(byte pattern[8][8], byte image1[8][8], byte image2[8][8]){
  //merge two [8][8] arrays, only leave off if both LEDs are off
  for (int i = 0; i < 8; i++){
    for (int j = 0; j < 8; j++ ){
      byte val1 = image1[i][j];
      byte val2 = image2[i][j];
      if (val1 == 0 && val2 == 0){
        pattern[i][j] = 0;
      } else {
        pattern[i][j] = 1;
      }
    }
  }
}

void receiveEvent(int numBytes) {
  if (numBytes >= sizeof(int)) {
    Wire.readBytes((char*)&receivedNote, sizeof(int)); // Read the integer bytes
  }
}
