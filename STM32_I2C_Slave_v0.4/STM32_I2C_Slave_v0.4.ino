#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HardwareTimer.h>

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// OLED I2C address
#define OLED_ADDR 0x3C

// Create an instance of the Adafruit_SSD1306 class
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

volatile uint32_t rpm = 0;
volatile uint32_t pulse_rpm = 0;
HardwareTimer timer(TIM2); // Use hardware timer TIM2

// Gauge parameters
const int centerX = SCREEN_WIDTH / 2;
const int centerY = SCREEN_HEIGHT / 2;
const int radius = 15;
const int maxRPM = 8000;  // Max RPM for the gauge

void setup() {
  // Initialize the serial communication
  Serial.begin(115200);

  // Initialize the OLED display
  if(!display.begin(SSD1306_I2C_ADDRESS, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.display();
  delay(2000);
  display.clearDisplay();

  // Setup a pin for RPM signal (assuming you have a hall sensor or similar)
  pinMode(PA0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PA0), pulseISR_RPM, RISING);

  // Initialize hardware timer
  timer.setOverflow(1000000, HERTZ); // 1 Hz (1 second interval)
  timer.attachInterrupt(CalcTimer);
  timer.resume();

  // Draw the initial gauge
  drawGauge();
}

void loop() {
  // Update the gauge with the current RPM
  drawNeedle(rpm);

  delay(500);  // Update the display every 500 ms
}

// Interrupt service routine for counting pulses
void pulseISR_RPM() {
  pulse_rpm++;
}

// Timer interrupt service routine for calculating RPM
void CalcTimer() {
  rpm = (pulse_rpm * 60);  // Assuming 1 pulse per revolution
  pulse_rpm = 0;  // Reset pulse count for next measurement
}

// Function to draw the gauge
void drawGauge() {
  display.clearDisplay();
  display.drawCircle(centerX, centerY, radius, SSD1306_WHITE);

  for (int angle = -90; angle <= 90; angle += 10) {
    float rad = radians(angle);
    int xOuter = centerX + cos(rad) * (radius + 5);
    int yOuter = centerY + sin(rad) * (radius + 5);
    int xInner = centerX + cos(rad) * radius;
    int yInner = centerY + sin(rad) * radius;
    display.drawLine(xOuter, yOuter, xInner, yInner, SSD1306_WHITE);
  }

  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.print("RPM Gauge");

  display.display();
}

// Function to draw the needle on the gauge
void drawNeedle(uint32_t rpm) {
  static int oldX = 0, oldY = 0;
  float angle = map(rpm, 0, maxRPM, -90, 90);
  float rad = radians(angle);
  int x = centerX + cos(rad) * (radius - 5);
  int y = centerY + sin(rad) * (radius - 5);

  // Erase the old needle
  if (oldX != 0 && oldY != 0) {
    display.drawLine(centerX, centerY, oldX, oldY, SSD1306_BLACK);
  }

  // Draw the new needle
  display.drawLine(centerX, centerY, x, y, SSD1306_WHITE);

  // Update the old needle position
  oldX = x;
  oldY = y;

  // Display the RPM value
  display.fillRect(0, SCREEN_HEIGHT - 8, SCREEN_WIDTH, 8, SSD1306_BLACK);  // Clear the old value
  display.setCursor(0, SCREEN_HEIGHT - 8);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.print("RPM: ");
  display.print(rpm);

  display.display();
}
