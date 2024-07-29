// Wire Slave Sender
// by Nicholas Zambetti <http://www.zambetti.com>
// modified by Jamie Coombe

// Demonstrates use of the Wire library
// Sends data as an I2C/TWI slave device
// Refer to the "Wire Master Reader" example for use with this

// Created 29 March 2006

// This example code is in the public domain.


#include <Wire.h>

// Pulse counting
volatile uint16_t low_pulse_count_int0 = 8478;
volatile uint16_t low_pulse_count_int1 = 149;

#define BUFFER_SIZE 8
// Buffer for I2C communication
uint8_t i2c_buffer[BUFFER_SIZE] = {0};

void setup() {
  Serial.begin(115200);
  Wire.begin(0x50);              // join i2c bus with address #2
  Wire.onRequest(requestEvent);  // register event

  Serial.println("Setup n ready");
}

void loop() {
  delay(100);
}

// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent() {
  //Wire.write("hello "); // respond with message of 6 bytes
  // as expected by master
  i2c_buffer[0] = (uint8_t)(low_pulse_count_int0 & 0xFF);  // LSB
  i2c_buffer[1] = (uint8_t)((low_pulse_count_int0 >> 8) & 0xFF);
  i2c_buffer[2] = (uint8_t)((low_pulse_count_int0 >> 16) & 0xFF);
  i2c_buffer[3] = (uint8_t)((low_pulse_count_int0 >> 24) & 0xFF);  // MSB
  i2c_buffer[4] = (uint8_t)(low_pulse_count_int1 & 0xFF);          // LSB
  i2c_buffer[5] = (uint8_t)((low_pulse_count_int1 >> 8) & 0xFF);
  i2c_buffer[6] = (uint8_t)((low_pulse_count_int1 >> 16) & 0xFF);
  i2c_buffer[7] = (uint8_t)((low_pulse_count_int1 >> 24) & 0xFF);  // MSB
  Wire.write(i2c_buffer, BUFFER_SIZE);
  
  Serial.print("int0: ");
  Serial.print(low_pulse_count_int0);
  Serial.print(", int1: ");
  Serial.println(low_pulse_count_int1);

  Serial.print("Buffer: ");
  for(uint8_t i=0; i<BUFFER_SIZE; i++)
  {
    Serial.print(i2c_buffer[i]);
    Serial.print(" ");
  }
  Serial.println();
}
