#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h> // Library for controlling the NeoTrellis

int ledMax = 84;
int ledA = 78;
int ledB = 35;
int ledC = 67;  

void startupLights(int l, int m, int n) {
  for (int i = ledA; i < 78 + 84; i++) {
    int j = i % 84;
      strip.setPixelColor(j, strip.Color(l,m,n));
      strip.show();
      delay(15);
  }
}

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define LED_VCC     9    //
#define LED_PIN     8    // Define the data pin connected to the LED strip
#define LED_GND     7    //
#define NUM_LEDS    84   // Define the number of LEDs in your strip
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int colorWipe = 0;
int loopCounter = 0;
int r = 0, g = 0, b = 0;
int cycleParity = 0;
unsigned long dispTime = millis();

#define LOGO_HEIGHT   32
#define LOGO_WIDTH    32
static const unsigned char PROGMEM logo_bmp[] =
{ 0b00000000, 0b00000000, 0b00000000, 0b00000000,
  0b00000000, 0b00000000, 0b00000000, 0b00000000,
  0b00000000, 0b00000000, 0b00000000, 0b00000000,
  0b00000000, 0b00000000, 0b00000000, 0b00111000,
  0b00000000, 0b00000000, 0b00000000, 0b01111000,
  0b00000000, 0b00000000, 0b00000011, 0b10000000,
  0b00000000, 0b00000000, 0b00001111, 0b00000000,
  0b00000000, 0b00001111, 0b11011100, 0b00000000,
  0b00000000, 0b00111111, 0b11110000, 0b00000000,
  0b00000000, 0b11111111, 0b11100110, 0b00000000,
  0b00000001, 0b11111111, 0b11001111, 0b00000000,
  0b00000001, 0b11111111, 0b10111111, 0b00000000,
  0b00000011, 0b11011111, 0b00111111, 0b10000000,
  0b00000111, 0b11011111, 0b01111111, 0b10000000,
  0b00000111, 0b10000110, 0b11111111, 0b11000000,
  0b00001111, 0b10011110, 0b11111111, 0b11000000,
  0b00001111, 0b01111101, 0b11111111, 0b11100000,
  0b00001110, 0b01111101, 0b11111111, 0b11100000,
  0b00001100, 0b11111011, 0b11100011, 0b11100000,
  0b00001100, 0b11111011, 0b11100011, 0b11100000,
  0b00001001, 0b11111111, 0b11101111, 0b11000000,
  0b00001001, 0b11111111, 0b11011111, 0b11000000,
  0b00001011, 0b11111111, 0b10111111, 0b10000000,
  0b00000011, 0b11111111, 0b01111111, 0b10000000,
  0b00000001, 0b11111111, 0b01111111, 0b00000000,
  0b00000000, 0b11111110, 0b01111110, 0b00000000,
  0b00000000, 0b11111110, 0b11111100, 0b00000000,
  0b00000000, 0b00111110, 0b11111000, 0b00000000,
  0b00000000, 0b00001110, 0b11000000, 0b00000000,
  0b00000000, 0b00000000, 0b00000000, 0b00000000,
  0b00000000, 0b00000000, 0b00000000, 0b00000000,
  0b00000000, 0b00000000, 0b00000000, 0b00000000,
   };

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(9600);
  strip.begin();
  
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
  display.setRotation(2); 
  display.display();

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);

  testdrawbitmap();

  initialLights();
  strip.show(); // Display the set colors on all LEDs

  startupLights(50, 0, 50);

  display.invertDisplay(true);
  delay(1000);
  display.invertDisplay(false);
  delay(1000);
  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
        
      // Print the incoming character on the OLED display
  display.setCursor(0, 0);
  display.write("Press Play...");

  initialLights();
}

void loop() {
    if (millis() - dispTime > 1000 && dispTime != 0) {
      display.clearDisplay();
      display.display();
      dispTime = 0;
    }
    if (colorWipe == 1) {
      cycleParity++;
      cycleParity %= 2;
      loopCounter += cycleParity;
      loopCounter %= 256;
      wheel(loopCounter, &r, &g, &b); 
      for (int i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(r, g, b));
      }
      strip.show(); // Display the colors on all LEDs
    }

    // Check if there are characters available in the serial buffer
    if (Serial.available() > 0) {
      // Read the next character from the serial buffer
      char firstChar = Serial.read();
      display.clearDisplay();
      display.setCursor(0,0);
      switch(firstChar) {
        case '0' :
                   display.write("C");
                   dispTime = millis();
                  break;
        case '1' : 
                   display.write("C#");
                   dispTime = millis();
                   break;
        case '2' : 
                   display.write("D");
                   dispTime = millis();
                   break;
        case '3' : 
                   display.write("Eb");
                   dispTime = millis();
                   break;
        case '4' :  
                   display.write("E");
                   dispTime = millis();
                   break;
        case '5' :  
                   display.write("F");\
                   dispTime = millis();
                   break;
        case '6' : 
                   display.write("F#");
                   dispTime = millis();
                   break;
        case '7' :
                   display.write("G");
                   dispTime = millis();
                   break;
        case '8' : 
                   display.write("Ab");
                   dispTime = millis();
                   break;
        case '9' : 
                   display.write("A");
                   dispTime = millis();
                   break;
        case '!' : 
                   display.write("Bb");
                   dispTime = millis();
                   break;
        case '@' : 
                   display.write("B");
                   dispTime = millis();
                   break;
//        case '#' : setLeds(255, 0, 0); // Set Red
//                   break;
//        case '$' : setLeds(0, 255, 0); // Set Green
//                   break;
//        case '%' : setLeds(0, 0, 255); // Set Blue
//                   break;
//        case '^' : setLeds(255, 0, 255); // Set Purple
//                   break;
//        case '&' : setLeds(255, 165, 0); // Set Orange
//                   break;
//        case '*' : setLeds(255, 255, 255); // Set White
//                   break;
//        case '(' : setLeds(0, 255, 255); // Set Cyan
//                   break;
//        case ')' : setLeds(255, 255, 0); // Set Yellow
//                   break;
        case '#' : setLeds(50, 0, 0); // Set Red
                   break;
        case '$' : setLeds(0, 50, 0); // Set Green
                   break;
        case '%' : setLeds(0, 0, 50); // Set Blue
                   break;
        case '^' : setLeds(50, 0, 50); // Set Purple
                   break;
        case '&' : setLeds(50, 25, 0); // Set Orange
                   break;
        case '*' : setLeds(50, 50, 50); // Set White
                   break;
        case '(' : setLeds(0, 50, 50); // Set Cyan
                   break;
        case ')' : setLeds(50, 50, 0); // Set Yellow
                   break;
        case '+' : colorWipe = 1;
                   break;
        case '-' : colorWipe = 0;
                   break;
        case 'a' : dispTime = millis();
                   display.write("Drums");
                   break;
        case 'b' : dispTime = millis();
                   display.write("Chords");
                   break;
        case 'c' : dispTime = millis();
                   display.write("Settings");
                   break;
        case 'd' : dispTime = millis();
                   display.write(firstChar);
                   break;
        case 'e' : dispTime = millis();
                   display.write("A");
                   break;
        case 'f' : dispTime = millis();
                   display.write("A#");
                   break;
        case 'g' : dispTime = millis();
                   display.write("B");
                   break;
        case 'h' : dispTime = millis();
                   display.write(firstChar);
                   break;
        case 'i' : dispTime = millis();
                   display.write(firstChar);
                   break;
        case 'j' : dispTime = millis();
                   display.write(firstChar);
                   break;
        case 'k' : dispTime = millis();
                   display.write(firstChar);
                   break;
        case 'l' : dispTime = millis();
                   display.write(firstChar);
                   break;
        case 'm' : dispTime = millis();
                   display.write(firstChar);
                   break;
        case 'n' : dispTime = millis();
                   display.write(firstChar);
                   break;
        case 'o' : dispTime = millis();
                   display.write(firstChar);
                   break;
        case 'p' : dispTime = millis();
                   display.write(firstChar);
                   break;
        case 'q' : dispTime = millis();
                   display.write("Drums Off");
                   break;
        case 'r' : dispTime = millis();
                   display.write("Drums On");
                   break;
        case 's' : dispTime = millis();
                   display.setTextSize(1);
                   display.write("Cannot alter mode");
                   display.setTextSize(2);
                   break;
        case 't' : dispTime = millis();
                   display.setTextSize(1);
                   display.write("Mode increased");
                   display.setTextSize(2);
                   break;
        case 'u' : display.setTextSize(1);
                   display.write("Cannot alter mode");
                   display.setTextSize(2);
                   break;
        case 'v' : dispTime = millis();
                   display.setTextSize(1);
                   display.write("Mode decreased");
                   display.setTextSize(2);
                   break;
        case 'w' : dispTime = millis();
                   display.write("+1 Octave");
                   break;
        case 'x' : dispTime = millis();
                   display.write("-1 Octave");
                   break;
        case 'y' : dispTime = millis();
                   display.write("+1 Semi");
                   break;
        case 'z' : dispTime = millis();
                   display.write("-1 Semi");
                   break;
        default :  display.clearDisplay(); break;
      }
      display.display();
    }
}

void testdrawchar(void) {
  display.clearDisplay();

  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(0, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  for(int16_t i=0; i<256; i++) {
    if(i == '\n') display.write(' ');
    else          display.write(i);
  }

  display.display();
  delay(2000);
}

void testdrawbitmap(void) {
  display.clearDisplay();

  display.drawBitmap(
    (display.width()  - LOGO_WIDTH ) / 2,
    (display.height() - LOGO_HEIGHT) / 2,
    logo_bmp, LOGO_WIDTH, LOGO_HEIGHT, 1);
  display.display();
  delay(1000);
}

void initialLights(void) {
  for (int i = 0;)
  for(int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(50, 0, 50)); // Set color to red (RGB: 255, 0, 0)
  }
  strip.show(); // Display the set colors on all LEDs
}

void setLeds(int a, int b, int c) {
  for(int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(a, b, c)); // Set color to red (RGB: 255, 0, 0)
  }
  strip.show(); // Display the set colors on all LEDs
}

void testscrolltext(void) {
  display.clearDisplay();

  display.setTextSize(2); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(F("scroll"));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}

void wheel(byte WheelPos, int *r, int *g, int *b) {
  if (WheelPos < 85) {
    *r = WheelPos * 3;
    *g = 255 - WheelPos * 3;
    *b = 0;
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    *r = 255 - WheelPos * 3;
    *g = 0;
    *b = WheelPos * 3;
  } else {
    WheelPos -= 170;
    *r = 0;
    *g = WheelPos * 3;
    *b = 255 - WheelPos * 3;
  }
}