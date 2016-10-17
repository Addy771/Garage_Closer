/*
-Garage Door Closer-
This program uses an HC-SR04 ultrasonic distance sensor  to detect if a garage door is open.
If the door has been open for too long, a signal is sent to the garage door opener so it closes the door
*/

#include <FastLED.h>
#define NUM_LEDS 10
#define TIME_INCREMENT 30  // How much time each LED segment is worth in minutes
#define SEGMENT_BRIGHTNESS 50  // How bright an LED should be when fully on, on a scale of 0-255

#define SECS_PER_SEG TIME_INCREMENT * 60ul
FASTLED_USING_NAMESPACE


// Pins
const int SENS1_TRIG_PIN = 7;
const int SENS1_ECHO_PIN = 8;
const int LED_DATA_PIN = 2;
const int DOOR1_SIGNAL_PIN = 3;

// Anything over 400 cm (23200 us pulse) is "out of range"
const unsigned int MAX_DIST = 23200;

CRGB leds[NUM_LEDS];

void draw_led_bar(unsigned long int time);

void setup() {

  // Configure the pin for Sensor 1
  pinMode(SENS1_TRIG_PIN, OUTPUT);
  digitalWrite(SENS1_TRIG_PIN, LOW);
  
  FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, NUM_LEDS);
  
  draw_led_bar(9050);
  
  /************************************/
  // Init pushbutton/debounce library

}

void loop() {
  
  // poll pushbutton
  
  // Test door1 open status
  
  //door1 state machine CLOSED, OPEN, CLOSING, OPENING
  
  // Process button input, add to timer if needed
  
  // Convert time remaining to LED indication. Segments will be lit up, last segment fades down as time runs out
  


}

// Convert remaining time in seconds to display on bar LEDs
void draw_led_bar(unsigned long int time)
{
  
  // Loop through each LED and decide what it should do
  for (char i = 0; i < NUM_LEDS; i++)
  {
    int brightness;
    
    // Calculate LED brightness for each segment. If time remaining is between whole segments, the last segment is partially lit
    brightness = time - i * SECS_PER_SEG;
    if (brightness < 0)
      brightness = 0;
    if (brightness > SECS_PER_SEG)
      brightness = SECS_PER_SEG;
    
    // Apply the brightness to the red channel of the current LED  
    leds[i] = CRGB(SEGMENT_BRIGHTNESS * (brightness / (TIME_INCREMENT * 60.0)),0,0);        
  }
  
  FastLED.show();
  
}
