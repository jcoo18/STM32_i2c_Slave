#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED display dimensions
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

// OLED I2C address
#define OLED_ADDR 0x3C

// Display port
TwoWire Wire2(PB11, PB10);
// Create an instance of the Adafruit_SSD1306 class
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire2, -1);

// I2C address and buffer size
#define SLAVE_ADDRESS 0x50
#define BUFFER_SIZE 8
// Buffer for I2C communication
uint8_t i2c_buffer[BUFFER_SIZE] = {0};

uint32_t rpm = 0;
uint32_t speed = 0;
volatile uint32_t pulse_rpm = 0, pulse_speed = 0, missedPulse_speed = 0;
volatile bool drawNew = false;
long timeLast = 0;

// Gauge parameters
const int radius = 15;
const int centerX = SCREEN_WIDTH - radius - 5;
const int centerY = SCREEN_HEIGHT / 2;

// System parameters
const int maxRPM = 8500; // Max RPM for the engine
// Speed sensor parameters
#define tyreDiameter 18                                                          // Tyre size in inches
#define tyreWidth 3                                                              // Tyre thickness in inches per side accounting for aspect ratio
#define tyreCircumferance ((tyreDiameter + (2 * tyreWidth)) * 25.4 * 3.14159265) // Tyre circumference
#define speedPulsesRev 2                                                         // Number of pulses per revolution

// Calculation constants
#define timInterval 250
#define rpmConst 60 * (1000 / timInterval) // Pulses per min for rpm
//                    distance traveled per pulse       # calcs /sec       min hours meters km
#define speedConst ((tyreCircumferance / speedPulsesRev) * (1000/timInterval)  * 60 * 60 / (1000 * 1000)) // # Pulse distance per hour
#define speedZeroTime ((3 * 1000) / timInterval)                                                           // # seconds * ms / timer interval

// Pin defines
#define pinRpmPulse PA11
#define pinSpeedPulse PA12
#define ledPin PC13

void setup()
{
  // Initialize the serial communication
  Serial.begin(115200);

  // Initialize the OLED display on the objects wire port
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR))
  {
    // Serial.println(F("SSD1306 allocation failed"));
    // for (;;);
  }

  // Initialize I2C as slave on the default wire port (PB6=SCL, PB7=SDA)
  Wire.begin(SLAVE_ADDRESS);
  Wire.onRequest(requestEvent);

  // Pin mode defines
  pinMode(ledPin, OUTPUT);
  pinMode(pinRpmPulse, INPUT_PULLUP);
  pinMode(pinSpeedPulse, INPUT_PULLUP);
  // Setup a pin for RPM signal (assuming you have a hall sensor or similar)
  attachInterrupt(digitalPinToInterrupt(pinRpmPulse), pulseISR_RPM, FALLING);
  // Setup a pin for Speed signal (assuming you have a hall sensor or similar)
  attachInterrupt(digitalPinToInterrupt(pinSpeedPulse), pulseISR_Speed, FALLING);
}

void loop()
{
  long timeNow = millis();
  if (timeNow - timeLast >= timInterval)
  {
    timeLast = timeNow;
    // Calc RPM
    rpm = (pulse_rpm * rpmConst); // Assuming 1 pulse per revolution
    pulse_rpm = 0;                // Reset pulse count for next measurement
    // Calc Speed
    if (pulse_speed > 0)
    {
      // If going so slow at least one calculation was missed
      if (missedPulse_speed > 0)
      {
        speed = ((pulse_speed * speedConst) / missedPulse_speed);
        missedPulse_speed = 0;
      }
      // Else going faster than the minimum calculation speed
      else
      {
        speed = (pulse_speed * speedConst);
      }
      pulse_speed = 0; // Reset pulse count for next measurement
    }
    // if no speed pulse sened then log a missed calculation
    else
    {
      // Incriment the number of calc events missed
      missedPulse_speed++;

      // if too many pulses are missed will reset speed to 0
      if (missedPulse_speed > speedZeroTime)
      {
        missedPulse_speed = 0;
        speed = 0;
      }
    }
    drawNew = true; // Signal new calculation has been performed and ready to output on the screen
  }

  if (drawNew)
  {
    // Update the gauge with the current RPM
    drawRPM(rpm);
    drawSpeed(speed);
    drawNew = false;
  }

  // delay(500);  // Update the display every 500 ms
}

// Interrupt service routine for counting RPM pulses
void pulseISR_RPM()
{
  pulse_rpm++;
  digitalWrite(ledPin, !digitalRead(ledPin));
}

// Interrupt service routine for counting Speed pulses
void pulseISR_Speed()
{
  pulse_speed++;
  // digitalWrite(ledPin, !digitalRead(ledPin));
}


// Function to draw the Speed
void drawSpeed(uint32_t _speed)
{
  // Display the Speed value
  display.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT / 2, SSD1306_BLACK); // Clear the old value
  display.setCursor(0, 0);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.print("KM/H: ");
  display.print(_speed);

  display.display();
}

// Function to draw the RPM
void drawRPM(uint32_t _rpm)
{
  // if (rpm > maxRPM)    rpm = maxRPM;

  // Display the RPM value
  display.fillRect(0, SCREEN_HEIGHT / 2, SCREEN_WIDTH, 14, SSD1306_BLACK); // Clear the old value
  display.setCursor(0, SCREEN_HEIGHT / 2);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.print("RPM: ");
  display.print(_rpm);

  display.display();
}

// I2C wire events
void requestEvent()
{
  // Prepare the response buffer with the rpm then speed data as 32bit numbers in 8 bit packets
  i2c_buffer[0] = (uint8_t)(rpm & 0xFF); // LSB
  i2c_buffer[1] = (uint8_t)((rpm >> 8) & 0xFF);
  i2c_buffer[2] = (uint8_t)((rpm >> 16) & 0xFF);
  i2c_buffer[3] = (uint8_t)((rpm >> 24) & 0xFF); // MSB
  i2c_buffer[4] = (uint8_t)(speed & 0xFF);       // LSB
  i2c_buffer[5] = (uint8_t)((speed >> 8) & 0xFF);
  i2c_buffer[6] = (uint8_t)((speed >> 16) & 0xFF);
  i2c_buffer[7] = (uint8_t)((speed >> 24) & 0xFF); // MSB
  Wire.write(i2c_buffer, BUFFER_SIZE);

  // Clear registers
  // rpm = 0;
  // speed = 0;
}