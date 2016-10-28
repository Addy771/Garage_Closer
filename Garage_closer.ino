/*
-Garage Door Closer-
Created by Adam Oakley in October 2016.

This program uses an HC-SR04 ultrasonic distance sensor to detect if a garage door is open.
When the door is open, a timer counts down and closes the door when the timer runs out.
Users can see the time remaining displayed on a neopixel-style RGB LED strip. Users can
press a pushbutton to increase the time remaining.

Capable of monitoring multiple doors
*/

#include "Bounce2.h"
#include "FastLED.h"

#define NUM_LEDS 10				// How many LEDs the strip has
#define TIME_INCREMENT 10		// How much time each LED segment is worth in minutes
#define SEGMENT_BRIGHTNESS 30	// How bright an LED should be when fully on, on a scale of 0-255

#define DEBOUNCE_TIME 20		// How many ms must pass before the button is considered stable
#define NUM_CONFIRMATIONS 10	// How many seconds must pass before door state is considered stable
#define OUTPUT_DURATION 300		// Length of the garage door opener output signal in milliseconds. May need to be tweaked if the opener doesn't respond

// If the distance is between the min and max, the door is present (centimeters)
#define MIN_DISTANCE 10
#define MAX_DISTANCE 30  

// Pin assignments
const int DOOR1_TRIG_PIN = 6;
const int DOOR1_ECHO_PIN = 7;
const int DOOR1_SIGNAL_PIN = 8;

const int DOOR2_TRIG_PIN = 15;
const int DOOR2_ECHO_PIN = 14;
const int DOOR2_SIGNAL_PIN = 16;

const int LED_DATA_PIN = 4;
const int BTN_PIN = 2;


/* END OF USER CONFIGURATION SECTION */

#define SECS_PER_SEG TIME_INCREMENT * 60ul
FASTLED_USING_NAMESPACE

Bounce button(BTN_PIN, DEBOUNCE_TIME);	// Create an instance of the bounce2 library
CRGB leds[NUM_LEDS];					// Array for LED strip values
unsigned long int remaining_time = 0;
unsigned long int ref_time;

// Used in door state machine
enum door_states { DOOR_CLOSED, DOOR_CLOSING, DOOR_OPEN, DOOR_OPENING };

// The door structure handles the i/o for a single door. 
// Multiple can be used for multiple doors
struct door
{
	int trig_pin;
	int echo_pin;
	int output_pin;
	int door_state = DOOR_CLOSED;
	int confirm_count = 0;
	int last_value = 0;
};

door door1;
door door2;

// Function prototypes
void init_sensor(door &sensor, int trig_pin, int echo_pin, int output_pin);
unsigned long measure_distance(door &sensor);
void draw_led_bar(unsigned long int time);
int update_door(door &sensor);

void setup() {

	// Initialize the door1 struct. It needs to know which pins to use
	init_sensor(door1, DOOR1_TRIG_PIN, DOOR1_ECHO_PIN, DOOR1_SIGNAL_PIN);
	
	// Initialize the door2 struct.
	init_sensor(door2, DOOR2_TRIG_PIN, DOOR2_ECHO_PIN, DOOR2_SIGNAL_PIN);

	// Enable the internal pullup resistor for the button pin
	pinMode(BTN_PIN, INPUT_PULLUP);

	FastLED.addLeds<NEOPIXEL, LED_DATA_PIN>(leds, NUM_LEDS);

	ref_time = millis();

}


void loop() {

	button.update();

	// Each time the button is pressed we add enough time to light up one LED segment
	if (button.fell())
		remaining_time += SECS_PER_SEG;

	// if 1 second has passed, update the remaining_time
	if (millis() - ref_time >= 1000)
	{
		if (remaining_time > 0)
			remaining_time--;
		ref_time += 1000;

		// Check the door positions and close them when the timer expires
		update_door(door1);
		update_door(door2);
	}

	// Don't show the time if the doors are both closed
	if (door1.door_state == DOOR_CLOSED && door2.door_state == DOOR_CLOSED)
		draw_led_bar(0);
	else
		draw_led_bar(remaining_time);

}

// handles state machine logic for a door struct
int update_door(door &sensor)
{
	unsigned char door_present;
	unsigned int dist = measure_distance(sensor);

	if (dist >= MIN_DISTANCE && dist <= MAX_DISTANCE)
		door_present = 1;
	else
		door_present = 0;

	// When the sensor value stays the same, increase confirm_count
	if (sensor.last_value == door_present && sensor.confirm_count < NUM_CONFIRMATIONS)
	{
		sensor.confirm_count++;
	}
	// Otherwise if the value is different, reset the confirm_count
	else if (sensor.last_value != door_present)
	{
		sensor.confirm_count = 0;
		sensor.last_value = door_present;
	}

	// state machine
	switch (sensor.door_state)
	{
	case DOOR_CLOSED:
		if (door_present)
		{
			sensor.door_state = DOOR_OPENING;
			sensor.confirm_count = 0;
		}
		break;

	case DOOR_OPENING:
		if (door_present && sensor.confirm_count == NUM_CONFIRMATIONS)
		{
			sensor.door_state = DOOR_OPEN;
			remaining_time = SECS_PER_SEG;	// If the door is open, start the countdown timer
		}

		// If the door is confirmed to be closed, go back to the closed state
		if (!door_present && sensor.confirm_count == NUM_CONFIRMATIONS)
			sensor.door_state = DOOR_CLOSED;
		break;

	case DOOR_OPEN:
		// If time runs out, close the door
		if (remaining_time == 0)
		{
			// Output high for 300ms to activate garage door opener
			digitalWrite(sensor.output_pin, HIGH);
			delay(300);
			digitalWrite(sensor.output_pin, LOW);

			sensor.door_state = DOOR_CLOSING;
			sensor.confirm_count = 0;
		}

		// If the user closed the door manually, go to the closing state
		if (!door_present)
		{
			sensor.door_state = DOOR_CLOSING;
			sensor.confirm_count = 0;
		}
		break;

	case DOOR_CLOSING:
		// If the door actually closed, go to the closed state
		if (!door_present && sensor.confirm_count == NUM_CONFIRMATIONS)
			sensor.door_state = DOOR_CLOSED;

		// If the door is still open, go back to the open state. 
		// This way, we can try to close the door again.
		if (door_present && sensor.confirm_count == NUM_CONFIRMATIONS)
			sensor.door_state = DOOR_OPEN;
		break;
	}
	return door_present;
}

// Initialize an door struct's values
void init_sensor(door &sensor, int trig_pin, int echo_pin, int output_pin)
{
	pinMode(trig_pin, OUTPUT);
	pinMode(echo_pin, INPUT);
	pinMode(output_pin, OUTPUT);

	digitalWrite(trig_pin, LOW);
	digitalWrite(output_pin, LOW);

	sensor.trig_pin = trig_pin;
	sensor.echo_pin = echo_pin;
	sensor.output_pin = output_pin;
}

// Perform one distance measurement using the specified sensor
unsigned long measure_distance(door &sensor)
{
	unsigned long t1;
	unsigned long pulse_width;
	unsigned long watchdog;

	// Hold the trigger pin high for 10us
	digitalWrite(sensor.trig_pin, HIGH);
	delayMicroseconds(10);
	digitalWrite(sensor.trig_pin, LOW);
	
	watchdog = millis();
	// Wait for start of echo pulse
	while (digitalRead(sensor.echo_pin) == LOW)
		if (millis() - watchdog >= 10)
			return 9000;	// if we didn't get an echo pulse, exit the function

	// Record the time since the pulse has started
	t1 = micros();
	watchdog = millis();

	// Wait for the pulse to end
	while (digitalRead(sensor.echo_pin) == HIGH)
		if (millis() - watchdog >= 30)
			return 9001;	// if the echo pulse didn't end, exit the function


	// The pulse has ended now, so calculate how much time passed
	pulse_width = micros() - t1;

	// Equation given by datasheet to convert to centimeters
	return pulse_width / 58;
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
		//leds[i] = CRGB((SEGMENT_BRIGHTNESS * brightness) / SECS_PER_SEG,0,0);      
		leds[i] = CRGB(SEGMENT_BRIGHTNESS * (brightness / (TIME_INCREMENT * 60.0)), 0, 0);
	}

	FastLED.show();
}
