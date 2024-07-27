#include <Wire.h>


// I2C address and buffer size
#define SLAVE_ADDRESS 0x50
#define BUFFER_SIZE 4

// Pulse counting
volatile uint16_t low_pulse_count_int0 = 0;
volatile uint16_t low_pulse_count_int1 = 0;

// Pin defines
#define pinPulse1 PA9
#define pinPulse2 PA10

// Buffer for I2C communication
uint8_t i2c_buffer[BUFFER_SIZE] = {0};


void setup() {
  // Initialize Serial Monitor for debugging
  //Serial.begin(9600);
  
  // Initialize I2C as slave
  Wire.begin(SLAVE_ADDRESS);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);
  
  // Initialize external interrupts for pulse counting
  pinMode(2, INPUT_PULLUP);  // INT0
  pinMode(3, INPUT_PULLUP);  // INT1
  attachInterrupt(digitalPinToInterrupt(pinRpmPulse), countPulseInt0, FALLING);
  attachInterrupt(digitalPinToInterrupt(pinSpeedPulse), countPulseInt1, FALLING);
}

void loop() {
  // Main loop can be used for other tasks
}

void countPulseInt0() {
  low_pulse_count_int0++;
}

void countPulseInt1() {
  low_pulse_count_int1++;
}

void receiveEvent(int howMany) {
  if (howMany == 1) {
    i2c_buffer[0] = Wire.read();
  }
}

void requestEvent() {
  if (i2c_buffer[0] == 0x01) {
    // Prepare the response buffer with the pulse count data
    i2c_buffer[1] = (uint8_t)(low_pulse_count_int0 & 0xFF); // LSB
    i2c_buffer[2] = (uint8_t)((low_pulse_count_int0 >> 8) & 0xFF); // MSB
    i2c_buffer[3] = (uint8_t)(low_pulse_count_int1 & 0xFF); // LSB
    i2c_buffer[4] = (uint8_t)((low_pulse_count_int1 >> 8) & 0xFF); // MSB
    Wire.write(i2c_buffer, BUFFER_SIZE);

    // Clear registers
    low_pulse_count_int0 = 0;
    low_pulse_count_int1 = 0;
  }
}
