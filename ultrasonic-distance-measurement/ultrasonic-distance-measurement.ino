// Include necessary libraries
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Pin Definitions ---
// Ultrasonic Sensor Pins
#define TRIG_PIN 12
#define ECHO_PIN 14

// Buzzer Pin
#define BUZZER_PIN 4

// LED Pins (in order from farthest to closest)
const int ledPins[] = {33, 32, 27, 26, 25};
const int numLeds = 5;

// --- OLED Display Setup ---
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --- Logic Parameters ---
#define MAX_DISTANCE 40 // The maximum distance we care about (in cm)
#define MIN_DISTANCE 5  // The distance at which all LEDs are on and the buzzer sounds

void setup() {
  Serial.begin(115200); // Start serial communication for debugging

  // Initialize Ultrasonic Sensor pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  // Initialize Buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW); // Make sure buzzer is off initially

  // Initialize LED pins
  for (int i = 0; i < numLeds; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW); // Turn all LEDs off
  }

  // Initialize OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  
  // Show initial display message
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Ultrasonic Sensor");
  display.println("System Ready!");
  display.display();
  delay(2000);
}

void loop() {
  // Get the distance from the sensor
  long duration, distance;
  
  // Trigger the sensor
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  // Read the echo pulse
  duration = pulseIn(ECHO_PIN, HIGH);
  
  // Calculate the distance in cm
  // Speed of sound is ~343 m/s or 0.0343 cm/µs.
  // The sound wave travels to the object and back, so we divide by 2.
  distance = duration * 0.0343 / 2;

  // Print distance to Serial Monitor for debugging
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // --- Control LEDs, Buzzer, and Display based on distance ---
  updateLeds(distance);
  updateBuzzer(distance);
  updateDisplay(distance);
  
  delay(100); // Wait 100ms before next reading
}

void updateLeds(long distance) {
  // Turn off all LEDs first
  for (int i = 0; i < numLeds; i++) {
    digitalWrite(ledPins[i], LOW);
  }

  // Calculate how many LEDs to turn on
  if (distance < MAX_DISTANCE) {
    // Map the distance to the number of LEDs
    // The closer the object, the more LEDs light up
    int ledsToLight = map(distance, MAX_DISTANCE, MIN_DISTANCE, 0, numLeds);
    ledsToLight = constrain(ledsToLight, 0, numLeds); // Ensure value is within bounds

    // Turn on the calculated number of LEDs
    for (int i = 0; i < ledsToLight; i++) {
      digitalWrite(ledPins[i], HIGH);
    }
  }
}

void updateBuzzer(long distance) {
  // If the object is closer than the minimum distance, beep the buzzer
  if (distance < MIN_DISTANCE && distance > 0) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }
}

void updateDisplay(long distance) {
  display.clearDisplay();
  display.setCursor(0, 0);
  
  if (distance < MAX_DISTANCE && distance > 0) {
    display.setTextSize(2);
    display.println("Object!");
    display.setTextSize(1);
    display.print("Dist: ");
    display.print(distance);
    display.print(" cm");
  } else {
    display.setTextSize(2);
    display.println("Clear");
  }
  
  display.display();
}