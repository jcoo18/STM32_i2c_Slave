#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "STM32TimerInterrupt.h"

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// OLED I2C address
#define OLED_ADDR 0x3C

TwoWire Wire2(PB11,PB10);

// Create an instance of the Adafruit_SSD1306 class
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire2, -1);

volatile uint32_t rpm = 0;
volatile uint32_t pulse_rpm = 0;
volatile bool drawNew = false;


// Gauge parameters
const int radius = 15;
const int centerX = SCREEN_WIDTH - radius - 5;
const int centerY = SCREEN_HEIGHT / 2;
const int maxRPM = 8500;  // Max RPM for the gauge

//HardwareTimer timer(TIM2); // Use hardware timer TIM2
// Init STM32 timer TIM2
STM32Timer ITimer1(TIM2);
const int timInterval = 250;  // Timer interval (ms)
#define timConst 60*(1000/timInterval)

void setup() {
  // Initialize the serial communication
  Serial.begin(115200);

  // Initialize the OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  pinMode(PC13, OUTPUT);
  // Setup a pin for RPM signal (assuming you have a hall sensor or similar)
  pinMode(PA0, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PA0), pulseISR_RPM, FALLING);

  
  // Initialize  timer
  ITimer1.attachInterruptInterval(timInterval * 1000, CalcTimer);
  

  // display.clearDisplay();
  // display.setCursor(0, 0);
  // display.setTextColor(SSD1306_WHITE);
  // display.setTextSize(1);
  // display.print("Pre");
  // display.display();
  // delay(1000);
  // Draw the initial gauge

  //drawGauge();

  // delay(1000);
  // display.clearDisplay();
  // display.setCursor(0, 0);
  // display.setTextColor(SSD1306_WHITE);
  // display.setTextSize(1);
  // display.print("Post");
  // display.display();
  // delay(1000);
  
}

void loop() {
  if (drawNew)
  {
    // Update the gauge with the current RPM
    drawNeedle(rpm);
    drawNew = false;
  }

  //delay(500);  // Update the display every 500 ms
}

// Interrupt service routine for counting pulses
void pulseISR_RPM() {
  pulse_rpm++;
  digitalWrite(PC13, !digitalRead(PC13));
}

// Timer interrupt service routine for calculating RPM
void CalcTimer() {
  rpm = (pulse_rpm * rpmConst);  // Assuming 1 pulse per revolution
  pulse_rpm = 0;  // Reset pulse count for next measurement
  drawNew = true;
}

// Function to draw the gauge
void drawGauge() {
  display.clearDisplay();
  display.drawCircle(centerX, centerY, radius, SSD1306_WHITE);
  
  // for (int angle = -90; angle <= 90; angle += 10) {
  //   float rad = radians(angle);
  //   int xOuter = centerX + cos(rad) * (radius + 5);
  //   int yOuter = centerY + sin(rad) * (radius + 5);
  //   int xInner = centerX + cos(rad) * radius;
  //   int yInner = centerY + sin(rad) * radius;
  //   display.drawLine(xOuter, yOuter, xInner, yInner, SSD1306_WHITE);
  // }

  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.print("Speed");

  display.display();
}

// Function to draw the needle on the gauge
void drawNeedle(uint32_t rpm) {
  if(rpm > maxRPM) rpm = maxRPM;
  // static int oldX = 0, oldY = 0;
  // float angle = map(rpm, 0, maxRPM, -180, 90);
  // float rad = radians(angle);
  // int x = centerX + cos(rad) * (radius - 5);
  // int y = centerY + sin(rad) * (radius - 5);

  // // Erase the old needle
  // if (oldX != 0 && oldY != 0) {
  //   display.drawLine(centerX, centerY, oldX, oldY, SSD1306_BLACK);
  // }

  // // Draw the new needle
  // display.drawLine(centerX, centerY, x, y, SSD1306_WHITE);

  // // Update the old needle position
  // oldX = x;
  // oldY = y;

  // Display the RPM value
  display.fillRect(0, SCREEN_HEIGHT - 14, SCREEN_WIDTH, 14, SSD1306_BLACK);  // Clear the old value
  display.setCursor(0, SCREEN_HEIGHT - 14);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.print("RPM: ");
  display.print(rpm);

  display.display();
}
