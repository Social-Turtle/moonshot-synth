#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h> // Library for controlling the NeoTrellis


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
int wheelPos = 0;
int loopCounter = 0;
int r = 0, g = 0, b = 0;
int cycleParity = 0;

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

  // Invert and restore display, pausing in-between
    initialLights();
    strip.show(); // Display the set colors on all LEDs


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
      if (colorWipe == 1) {
        cycleParity++;
        cycleParity %= 2;
        loopCounter += cycleParity;
        loopCounter %= 256;
        wheel(loopCounter, &r, &g, &b);
        Serial.println(strip.Color(r,g,b));     
        for (int i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(r, g, b));
        }
        strip.show(); // Display the colors on all LEDs
        //delay(25);    // Pause for a moment
      }

    // Check if there are characters available in the serial buffer
    if (Serial.available() > 0) {
      // Read the next character from the serial buffer
      char firstChar = Serial.read();
      switch(firstChar) {
        case '0' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("C");
                  break;
        case '1' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("C#");
                   break;
        case '2' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("D");
                   break;
        case '3' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("Eb");
                   break;
        case '4' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("E");
                   break;
        case '5' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("F");
                   break;
        case '6' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("F#");
                   break;
        case '7' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("G");
                   break;
        case '8' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("Ab");
                   break;
        case '9' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("A");
                   break;
        case '!' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("Bb");
                   break;
        case '@' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("B");
                   break;
        case '#' : setLeds(255, 0, 0); // Set Red
                   break;
        case '$' : setLeds(0, 255, 0); // Set Green
                   break;
        case '%' : setLeds(0, 0, 255); // Set Blue
                   break;
        case '^' : setLeds(255, 0, 255); // Set Purple
                   break;
        case '&' : setLeds(255, 165, 0); // Set Orange
                   break;
        case '*' : setLeds(255, 255, 255); // Set White
                   break;
        case '(' : setLeds(0, 255, 255); // Set Cyan
                   break;
        case ')' : setLeds(255, 255, 0); // Set Yellow
                   break;
        case '+' : colorWipe = 1;
                   break;
        case '-' : colorWipe = 0;
                   break;
        case 'a' : display.clearDisplay(); // Page 0, RED
                   display.setCursor(0,0);
                   display.write("Drum Machine");
                   break;
        case 'b' : display.clearDisplay(); // Page 1, YELLOW
                   display.setCursor(0,0);
                   display.write("Chord Bank");
                   break;
        case 'c' : display.clearDisplay(); // Page 2, GREEN
                   display.setCursor(0,0);
                   display.write("Settings Mode");
                   break;
        case 'd' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write(firstChar);
                   break;
        case 'e' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("A");
                   break;
        case 'f' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("A#");
                   break;
        case 'g' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("B");
                   break;
        case 'h' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write(firstChar);
                   break;
        case 'i' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write(firstChar);
                   break;
        case 'j' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write(firstChar);
                   break;
        case 'k' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write(firstChar);
                   break;
        case 'l' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write(firstChar);
                   break;
        case 'm' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write(firstChar);
                   break;
        case 'n' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write(firstChar);
                   break;
        case 'o' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write(firstChar);
                   break;
        case 'p' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write(firstChar);
                   break;
        case 'q' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("Drums Muted");
                   break;
        case 'r' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("Drums Playing");
                   break;
        case 's' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("Cannot alter mode");
                   break;
        case 't' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("Mode increased");
                   break;
        case 'u' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("Cannot alter mode");
                   break;
        case 'v' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("Mode decreased");
                   break;
        case 'w' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("+1 Octave");
                   break;
        case 'x' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("-1 Octave");
                   break;
        case 'y' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("+1 Semitone");
                   break;
        case 'z' : display.clearDisplay();
                   display.setCursor(0,0);
                   display.write("-1 Semitone");
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

void testdrawstyles(void) {
  display.clearDisplay();

  display.setTextSize(1);             // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);             // Start at top-left corner
  display.println(F("Hello, world!"));

  display.setTextColor(SSD1306_BLACK, SSD1306_WHITE); // Draw 'inverse' text
  display.println(3.141592);

  display.setTextSize(2);             // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.print(F("0x")); display.println(0xDEADBEEF, HEX);

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
  for(int i = 0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(50, 0, 50)); // Set color to red (RGB: 255, 0, 0)
  }
  strip.show(); // Display the set colors on all LEDs
}

void setLeds(int a, int b, int c) {
  //for(int i = 0; i < strip.numPixels(); i++) {
  //  strip.setPixelColor(i, color);
  //}
  //strip.show(); // Display the set colors on all LEDs
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
