#include <FastLED.h>

// Number of LEDs on the strip
#define NUM_LEDS 30
#define LED_DATA_PIN 3
// Three colors
#define NUM_BYTES (NUM_LEDS*3)

// Maximum brightness (0 - 100)
#define BRIGHTNESS 100
// FPS
#define UPDATES_PER_SECOND 20

#define TIMEOUT 3000

#define MODE_ANIMATION 0
#define MODE_AMBILIGHT 1
// Set the default mode to be an ANIMATION
uint8_t mode = MODE_ANIMATION;

byte MESSAGE_PREAMBLE[] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 };
uint8_t PREAMBLE_LENGTH = 10;
uint8_t current_preamble_position = 0;

unsigned long last_serial_available = -1L;

uint8_t led_counter = 0;
uint8_t byte_counter = 0;

CRGB leds[NUM_LEDS];
byte buffer[NUM_BYTES];

// If we do not have a signal for ambillight
// Play rainbow colors on the LED strip
CRGBPalette16 currentPalette  = RainbowColors_p;
TBlendType    currentBlending = LINEARBLEND;
uint8_t startIndex = 0;

void setup() {
  // Sets the data rate in bits per second (baud) for serial data transmission.
  // 115200 is the highest possible rate for communicating with the computer.
  Serial.begin(115200);
  FastLED.clear(true);
  // Specify the LED strip type, data pin and colors
  FastLED.addLeds<WS2812B, LED_DATA_PIN, GRB>(leds, NUM_LEDS);
  // Set the desired brightness (from the BRIGHTNESS preprocessor directive)
  FastLED.setBrightness(BRIGHTNESS);
}

void loop() {
  // Based on the mode global var (which by default is MODE_ANIMATION)
  // act in a certain way
  switch (mode) {
    case MODE_ANIMATION:
      // If we are in animation mode, just play the rainbow colors on the strip
      fillLEDsFromPaletteColors();
      break;
    case MODE_AMBILIGHT:
      // The ambilight functionality!
      processIncomingData();
      break;
  }
}

void processIncomingData() {
  // The main functionality for the ambilight feature
  // if we received a preamble signal (waitForPreamble)
  if (waitForPreamble(TIMEOUT)) {
    // Read *NUM_BYTES* bytes into buffer
    Serial.readBytes((char*) buffer, NUM_BYTES);
  
    // While we have bytes
    while (byte_counter < NUM_BYTES) {
      byte blue = buffer[byte_counter++];
      byte green = buffer[byte_counter++];
      byte red = buffer[byte_counter++];
      
      // Set each led with the specified RGB signal
      leds[led_counter++] = CRGB(red, green, blue);
    }
    
    FastLED.show();
    
    byte_counter = 0;
    led_counter = 0;
  } else {
    // If we do not receive any preamble signal within the TIMEOUT
    // switch to animation mode
    mode = MODE_ANIMATION;
  }
}

bool waitForPreamble(int timeout) {
  // Returns the number of milliseconds since the Arduino board began running the current program.
  last_serial_available = millis();
  
  while (current_preamble_position < PREAMBLE_LENGTH) {
    
    // Checks whether the number of bytes (characters) available
    // for reading from the serial port is a positive number
    if (Serial.available() > 0) {
      last_serial_available = millis();
      
      if (Serial.read() == MESSAGE_PREAMBLE[current_preamble_position]) {
        current_preamble_position++;
      } else {
        current_preamble_position = 0;
      }
    }
   
    if (millis() - last_serial_available > timeout) {
      return false;
    }
  }
  
  current_preamble_position = 0;
  
  return true;
}

void fillLEDsFromPaletteColors() {
  uint8_t colorIndex = ++startIndex;
  
  for (int i = 0; i < NUM_LEDS; i++, colorIndex += 3) {
    leds[i] = ColorFromPalette(
      currentPalette, colorIndex, BRIGHTNESS, currentBlending
    );
  }

  FastLED.delay(1000 / UPDATES_PER_SECOND);

  if (Serial.available() > 0) {
    // If we received a preamble signal, switch to AMBILIGHT mode
    mode = MODE_AMBILIGHT;
  }
}

