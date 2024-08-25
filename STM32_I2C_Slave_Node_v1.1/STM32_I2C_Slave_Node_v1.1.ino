/**
 * Base code made by jcoo18 for datalogging to a SparkFun Openlog_Artimus
 * https://github.com/jcoo18/STM32_i2c_Slave_Sensor
 * 
 * 
 * Speed calculations adapted from code by InterlinkKnight
 * https://www.youtube.com/watch?v=u2uJMJWsfsg
 */


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

// I2C as slave on the default wire port (PB6=SCL, PB7=SDA)
// I2C address and buffer size
#define SLAVE_ADDRESS 0x50
#define BUFFER_SIZE 8
// Buffer for I2C communication
uint8_t i2c_buffer[BUFFER_SIZE] = {0};

/*
uint32_t rpm = 0;
uint32_t speed = 0;
volatile uint32_t pulse_rpm = 0, pulse_speed = 0, missedPulse_speed = 0;
volatile bool drawNew = false;
long timeLast = 0;
*/

///////////////
// Calibration:
///////////////

const byte PulsesPerRevolution = 1;  // Set how many pulses there are on each revolution on sensor 1. Default: 2.
const byte PulsesPerRevolutionSpeed = 2;  // Set how many pulses there are on each revolution on sensor 2. Default: 2.


// If the period between pulses is too high, or even if the pulses stopped, then we would get stuck showing the
// last value instead of a 0. Because of this we are going to set a limit for the maximum period allowed.
// If the period is above this value, the RPM will show as 0.
// The higher the set value, the longer lag/delay will have to sense that pulses stopped, but it will allow readings
// at very low RPM.
// Setting a low value is going to allow the detection of stop situations faster, but it will prevent having low RPM readings.
// The unit is in microseconds.
const unsigned long ZeroTimeout = 100000;  // For high response time, a good value would be 100000.
                                           // For reading very low RPM, a good value would be 300000.
const unsigned long ZeroTimeoutSpeed = 100000;  // Sensor 2

// Calibration for smoothing RPM:
const byte numReadings = 2;  // Number of samples for smoothing. The higher, the more smoothing, but it's going to
                             // react slower to changes. 1 = no smoothing. Default: 2.
const byte numReadingsSpeed = 2;  // Sensor 2

/////////////
// Variables:
/////////////

//////////// Sensor 1:

volatile unsigned long LastTimeWeMeasured;  // Stores the last time we measured a pulse so we can calculate the period.
volatile unsigned long PeriodBetweenPulses = ZeroTimeout+1000;  // Stores the period between pulses in microseconds.
                       // It has a big number so it doesn't start with 0 which would be interpreted as a high frequency.
volatile unsigned long PeriodAverage = ZeroTimeout+1000;  // Stores the period between pulses in microseconds in total, if we are taking multiple pulses.
                       // It has a big number so it doesn't start with 0 which would be interpreted as a high frequency.
unsigned long FrequencyRaw;  // Calculated frequency, based on the period. This has a lot of extra decimals without the decimal point.
unsigned long FrequencyReal;  // Frequency without decimals.
unsigned long RPM;  // Raw RPM without any processing.
unsigned int PulseCounter = 1;  // Counts the amount of pulse readings we took so we can average multiple pulses before calculating the period.

unsigned long PeriodSum; // Stores the summation of all the periods to do the average.

unsigned long LastTimeCycleMeasure = LastTimeWeMeasured;  // Stores the last time we measure a pulse in that cycle.
                                    // We need a variable with a value that is not going to be affected by the interrupt
                                    // because we are going to do math and functions that are going to mess up if the values
                                    // changes in the middle of the cycle.
unsigned long CurrentMicros = micros();  // Stores the micros in that cycle.
                                         // We need a variable with a value that is not going to be affected by the interrupt
                                         // because we are going to do math and functions that are going to mess up if the values
                                         // changes in the middle of the cycle.

// We get the RPM by measuring the time between 2 or more pulses so the following will set how many pulses to
// take before calculating the RPM. 1 would be the minimum giving a result every pulse, which would feel very responsive
// even at very low speeds but also is going to be less accurate at higher speeds.
// With a value around 10 you will get a very accurate result at high speeds, but readings at lower speeds are going to be
// farther from eachother making it less "real time" at those speeds.
// There's a function that will set the value depending on the speed so this is done automatically.
unsigned int AmountOfReadings = 1;

unsigned int ZeroDebouncingExtra;  // Stores the extra value added to the ZeroTimeout to debounce it.
                                   // The ZeroTimeout needs debouncing so when the value is close to the threshold it
                                   // doesn't jump from 0 to the value. This extra value changes the threshold a little
                                   // when we show a 0.

// Variables for smoothing tachometer 1:
unsigned long readings[numReadings];  // The input.
unsigned long readIndex;  // The index of the current reading.
unsigned long total;  // The running total.
unsigned long averageRPM;  // The RPM value after applying the smoothing.




//////////// Speed Sensor:

volatile unsigned long LastSpeedTimeMeasured;  // Stores the last time we measured a pulse so we can calculate the period.
volatile unsigned long PeriodBetweenSpeedPulses = ZeroTimeout+1000;  // Stores the period between pulses in microseconds.
                       // It has a big number so it doesn't start with 0 which would be interpreted as a high frequency.
volatile unsigned long PeriodAverageSpeed = ZeroTimeout+1000;  // Stores the period between pulses in microseconds in total, if we are taking multiple pulses.
                       // It has a big number so it doesn't start with 0 which would be interpreted as a high frequency.
unsigned long FrequencyRawSpeed;  // Calculated frequency, based on the period. This has a lot of extra decimals without the decimal point.
unsigned long FrequencyRealSpeed;  // Frequency without decimals.
unsigned long Speed;  // Raw RPM without any processing.
unsigned int PulseCounterSpeed = 1;  // Counts the amount of pulse readings we took so we can average multiple pulses before calculating the period.

unsigned long PeriodSumSpeed; // Stores the summation of all the periods to do the average.

unsigned long LastTimeCycleMeasureSpeed = LastSpeedTimeMeasured;  // Stores the last time we measure a pulse in that cycle.
                                    // We need a variable with a value that is not going to be affected by the interrupt
                                    // because we are going to do math and functions that are going to mess up if the values
                                    // changes in the middle of the cycle.
unsigned long CurrentMicrosSpeed = micros();  // Stores the micros in that cycle.
                                         // We need a variable with a value that is not going to be affected by the interrupt
                                         // because we are going to do math and functions that are going to mess up if the values
                                         // changes in the middle of the cycle.

// We get the RPM by measuring the time between 2 or more pulses so the following will set how many pulses to
// take before calculating the RPM. 1 would be the minimum giving a result every pulse, which would feel very responsive
// even at very low speeds but also is going to be less accurate at higher speeds.
// With a value around 10 you will get a very accurate result at high speeds, but readings at lower speeds are going to be
// farther from eachother making it less "real time" at those speeds.
// There's a function that will set the value depending on the speed so this is done automatically.
unsigned int AmountOfReadingsSpeed = 1;

unsigned int ZeroDebouncingExtraSpeed;  // Stores the extra value added to the ZeroTimeout to debounce it.
                                   // The ZeroTimeout needs debouncing so when the value is close to the threshold it
                                   // doesn't jump from 0 to the value. This extra value changes the threshold a little
                                   // when we show a 0.


// Variables for smoothing tachometer 2:
unsigned long readingsSpeed[numReadings];  // The input.
unsigned long readIndexSpeed;  // The index of the current reading.
unsigned long totalSpeed;  // The running total.
unsigned long averageSpeed;  // The RPM value after applying the smoothing.

// System parameters
const int maxRPM = 9000; // Max RPM for the engine
// Speed sensor parameters
#define tyreDiameter 18                                                          // Tyre size in inches
#define tyreWidth 3                                                              // Tyre thickness in inches per side accounting for aspect ratio
#define tyreCircumferance ((tyreDiameter + (2 * tyreWidth)) * 25.4 * 3.14159265) // Tyre circumference (tyre inches + aspect ratio) * mm/in * Pi
#define speedPulsesRev 2                                                         // Number of pulses per revolution

// Calculation constants
#define timInterval 250
#define rpmConst 60 * (1000 / timInterval) // Pulses per min for rpm
//                    distance traveled per pulse       # calcs /sec       min hours meters km
//#define speedConst ((tyreCircumferance / speedPulsesRev) * (1000/timInterval)  * 60 * 60 / (1000 * 1000)) // # Pulse distance per hour
#define speedConst (tyreCircumferance   * 60 * 60 / (1000 * 1000)) // # Frequency to Km distance per hour conversion factor

#define speedZeroTime ((3 * 1000) / timInterval)                                                           // # seconds * ms / timer interval

// Pin defines
#define pinRpmPulse PA11
#define pinSpeedPulse PA12
#define ledPin PC13

//#define serialDebug

void setup()
{
  // Initialize the serial communication
  #ifdef serialDebug
  Serial.begin(115200);
  #endif

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

  // Configure Timer 3 for generating an interrupt every millisecond
  //TIM_Config(TIM3, 72, 1000, TIMER_INTERRUPT_PIN);
}

void loop()
{
  
  ////// Sensor 1:
  
  // The following is going to store the two values that might change in the middle of the cycle.
  // We are going to do math and functions with those values and they can create glitches if they change in the
  // middle of the cycle.
  LastTimeCycleMeasure = LastTimeWeMeasured;  // Store the LastTimeWeMeasured in a variable.
  CurrentMicros = micros();  // Store the micros() in a variable.

  // CurrentMicros should always be higher than LastTimeWeMeasured, but in rare occasions that's not true.
  // I'm not sure why this happens, but my solution is to compare both and if CurrentMicros is lower than
  // LastTimeCycleMeasure I set it as the CurrentMicros.
  // The need of fixing this is that we later use this information to see if pulses stopped.
  if(CurrentMicros < LastTimeCycleMeasure)
  {
    LastTimeCycleMeasure = CurrentMicros;
  }

  // Calculate the frequency:
  FrequencyRaw = 10000000000 / PeriodAverage;  // Calculate the frequency using the period between pulses.

  // Detect if pulses stopped or frequency is too low, so we can show 0 Frequency:
  if(PeriodBetweenPulses > ZeroTimeout - ZeroDebouncingExtra || CurrentMicros - LastTimeCycleMeasure > ZeroTimeout - ZeroDebouncingExtra)
  {  // If the pulses are too far apart that we reached the timeout for zero:
    FrequencyRaw = 0;  // Set frequency as 0.
    ZeroDebouncingExtra = 2000;  // Change the threshold a little so it doesn't bounce.
  }
  else
  {
    ZeroDebouncingExtra = 0;  // Reset the threshold to the normal value so it doesn't bounce.
  }


  FrequencyReal = FrequencyRaw / 10000;  // Get frequency without decimals.
                                          // This is not used to calculate RPM but we remove the decimals just in case
                                          // you want to print it.

  // Calculate the RPM:
  RPM = FrequencyRaw / PulsesPerRevolution * 60;  // Frequency divided by amount of pulses per revolution multiply by
                                                  // 60 seconds to get minutes.
  RPM = RPM / 10000;  // Remove the decimals.

  // Smoothing RPM:
  total = total - readings[readIndex];  // Advance to the next position in the array.
  readings[readIndex] = RPM;  // Takes the value that we are going to smooth.
  total = total + readings[readIndex];  // Add the reading to the total.
  readIndex = readIndex + 1;  // Advance to the next position in the array.

  if (readIndex >= numReadings)  // If we're at the end of the array:
  {
    readIndex = 0;  // Reset array index.
  }
  
  // Calculate the average:
  averageRPM = total / numReadings;  // The average value it's the smoothed result.
  if (averageRPM > maxRPM) averageRPM = maxRPM;

  /********************** Speed: */ 

  // The following is going to store the two values that might change in the middle of the cycle.
  // We are going to do math and functions with those values and they can create glitches if they change in the
  // middle of the cycle.
  LastTimeCycleMeasureSpeed = LastSpeedTimeMeasured;  // Store the LastTimeWeMeasured in a variable.
  CurrentMicrosSpeed = micros();  // Store the micros() in a variable.

  // CurrentMicros should always be higher than LastTimeWeMeasured, but in rare occasions that's not true.
  // I'm not sure why this happens, but my solution is to compare both and if CurrentMicros is lower than
  // LastTimeCycleMeasure I set it as the CurrentMicros.
  // The need of fixing this is that we later use this information to see if pulses stopped.
  if(CurrentMicrosSpeed < LastTimeCycleMeasureSpeed)
  {
    LastTimeCycleMeasureSpeed = CurrentMicrosSpeed;
  }

  // Calculate the frequency:
  FrequencyRawSpeed = 10000000000 / PeriodAverageSpeed;  // Calculate the frequency using the period between pulses.

  // Detect if pulses stopped or frequency is too low, so we can show 0 Frequency:
  if(PeriodBetweenSpeedPulses > ZeroTimeoutSpeed - ZeroDebouncingExtraSpeed || CurrentMicrosSpeed - LastTimeCycleMeasureSpeed > ZeroTimeoutSpeed - ZeroDebouncingExtraSpeed)
  {  // If the pulses are too far apart that we reached the timeout for zero:
    FrequencyRawSpeed = 0;  // Set frequency as 0.
    ZeroDebouncingExtraSpeed = 2000;  // Change the threshold a little so it doesn't bounce.
  }
  else
  {
    ZeroDebouncingExtraSpeed = 0;  // Reset the threshold to the normal value so it doesn't bounce.
  }

  FrequencyRealSpeed = FrequencyRawSpeed / 10000;  // Get frequency without decimals.
                                          // This is not used to calculate RPM but we remove the decimals just in case
                                          // you want to print it.

  // Calculate the speed:
  // Frequency divided by amount of pulses per revolution multiply by speed constant (wheel circumferance * hours) to get Km/h.
  Speed = FrequencyRawSpeed / PulsesPerRevolutionSpeed * speedConst;  
  Speed = Speed / 10000;  // Remove the decimals.

  // Smoothing RPM:
  totalSpeed = totalSpeed - readingsSpeed[readIndexSpeed];  // Advance to the next position in the array.
  readingsSpeed[readIndexSpeed] = Speed;  // Takes the value that we are going to smooth.
  totalSpeed = totalSpeed + readingsSpeed[readIndexSpeed];  // Add the reading to the total.
  readIndexSpeed = readIndexSpeed + 1;  // Advance to the next position in the array.

  if (readIndexSpeed >= numReadingsSpeed)  // If we're at the end of the array:
  {
    readIndexSpeed = 0;  // Reset array index.
  }
  
  // Calculate the average:
  averageSpeed = totalSpeed / numReadingsSpeed;  // The average value it's the smoothed result.
    if (averageSpeed > 180) averageSpeed = 180;

  // Print information on the serial monitor:
  // Comment this section if you have a display and you don't need to monitor the values on the serial monitor.
  // This is because disabling this section would make the loop run faster.
  
   #ifdef serialDebug
  // Sensor 1:
  /*
  Serial.print("Period: ");
  Serial.print(PeriodBetweenPulses);
  Serial.print("\t");  // TAB space
  Serial.print("Readings: ");
  Serial.print(AmountOfReadings);
  Serial.print("\t");  // TAB space
  Serial.print("Frequency: ");
  Serial.print(FrequencyReal);
  Serial.print("\t");  // TAB space
  Serial.print("RPM: ");
  Serial.print(RPM);
  Serial.print("\t");  // TAB space
  */

  Serial.print("Tachometer: ");
  Serial.print(averageRPM);
  
  
  // Sensor 2:
  /*
  Serial.print("\t");  // TAB space
  Serial.print("Period2: ");
  Serial.print(PeriodBetweenPulses2);
  Serial.print("\t");  // TAB space
  Serial.print("Readings2: ");
  Serial.print(AmountOfReadings2);
  Serial.print("\t");  // TAB space
  Serial.print("Frequency2: ");
  Serial.print(FrequencyReal2);
  Serial.print("\t");  // TAB space
  Serial.print("RPM2: ");
  Serial.print(RPM2);
  Serial.print("\t");  // TAB space
  */
  Serial.print("Tachometer2: ");
  Serial.println(averageSpeed);
  #endif

  if (1)
  {
    // Update the gauge with the current RPM
    drawRPM(averageRPM);
    drawSpeed(averageSpeed);
    //drawNew = false;
  }


  /*
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
  */
}

// Interrupt service routine for counting RPM pulses
void pulseISR_RPM()
{
  // pulse_rpm++;
  // digitalWrite(ledPin, !digitalRead(ledPin));
    PeriodBetweenPulses = micros() - LastTimeWeMeasured;  // Current "micros" minus the old "micros" when the last pulse happens.
                                                        // This will result with the period (microseconds) between both pulses.
                                                        // The way is made, the overflow of the "micros" is not going to cause any issue.

  LastTimeWeMeasured = micros();  // Stores the current micros so the next time we have a pulse we would have something to compare with.

  if(PulseCounter >= AmountOfReadings)  // If counter for amount of readings reach the set limit:
  {
    PeriodAverage = PeriodSum / AmountOfReadings;  // Calculate the final period dividing the sum of all readings by the
                                                   // amount of readings to get the average.
    PulseCounter = 1;  // Reset the counter to start over. The reset value is 1 because its the minimum setting allowed (1 reading).
    PeriodSum = PeriodBetweenPulses;  // Reset PeriodSum to start a new averaging operation.


    // Change the amount of readings depending on the period between pulses.
    // To be very responsive, ideally we should read every pulse. The problem is that at higher speeds the period gets
    // too low decreasing the accuracy. To get more accurate readings at higher speeds we should get multiple pulses and
    // average the period, but if we do that at lower speeds then we would have readings too far apart (laggy or sluggish).
    // To have both advantages at different speeds, we will change the amount of readings depending on the period between pulses.
    // Remap period to the amount of readings:
    int RemapedAmountOfReadings = map(PeriodBetweenPulses, 40000, 5000, 1, 10);  // Remap the period range to the reading range.
    // 1st value is what are we going to remap. In this case is the PeriodBetweenPulses.
    // 2nd value is the period value when we are going to have only 1 reading. The higher it is, the lower RPM has to be to reach 1 reading.
    // 3rd value is the period value when we are going to have 10 readings. The higher it is, the lower RPM has to be to reach 10 readings.
    // 4th and 5th values are the amount of readings range.
    RemapedAmountOfReadings = constrain(RemapedAmountOfReadings, 1, 10);  // Constrain the value so it doesn't go below or above the limits.
    AmountOfReadings = RemapedAmountOfReadings;  // Set amount of readings as the remaped value.
  }
  else
  {
    PulseCounter++;  // Increase the counter for amount of readings by 1.
    PeriodSum = PeriodSum + PeriodBetweenPulses;  // Add the periods so later we can average.
  }
}

// Interrupt service routine for counting Speed pulses
void pulseISR_Speed()
{
  //pulse_speed++;
  // digitalWrite(ledPin, !digitalRead(ledPin));

  PeriodBetweenSpeedPulses = micros() - LastSpeedTimeMeasured;  // Current "micros" minus the old "micros" when the last pulse happens.
                                                        // This will result with the period (microseconds) between both pulses.
                                                        // The way is made, the overflow of the "micros" is not going to cause any issue.

  LastSpeedTimeMeasured = micros();  // Stores the current micros so the next time we have a pulse we would have something to compare with.

  if(PulseCounterSpeed >= AmountOfReadingsSpeed)  // If counter for amount of readings reach the set limit:
  {
    PeriodAverageSpeed = PeriodSumSpeed / AmountOfReadingsSpeed;  // Calculate the final period dividing the sum of all readings by the
                                                   // amount of readings to get the average.
    PulseCounterSpeed = 1;  // Reset the counter to start over. The reset value is 1 because its the minimum setting allowed (1 reading).
    PeriodSumSpeed = PeriodBetweenSpeedPulses;  // Reset PeriodSum to start a new averaging operation.

    // Change the amount of readings depending on the period between pulses.
    // To be very responsive, ideally we should read every pulse. The problem is that at higher speeds the period gets
    // too low decreasing the accuracy. To get more accurate readings at higher speeds we should get multiple pulses and
    // average the period, but if we do that at lower speeds then we would have readings too far apart (laggy or sluggish).
    // To have both advantages at different speeds, we will change the amount of readings depending on the period between pulses.
    // Remap period to the amount of readings:
    int RemapedAmountOfReadings2 = map(PeriodBetweenSpeedPulses, 40000, 5000, 1, 10);  // Remap the period range to the reading range.
    // 1st value is what are we going to remap. In this case is the PeriodBetweenPulses.
    // 2nd value is the period value when we are going to have only 1 reading. The higher it is, the lower RPM has to be to reach 1 reading.
    // 3rd value is the period value when we are going to have 10 readings. The higher it is, the lower RPM has to be to reach 10 readings.
    // 4th and 5th values are the amount of readings range.
    RemapedAmountOfReadings2 = constrain(RemapedAmountOfReadings2, 1, 10);  // Constrain the value so it doesn't go below or above the limits.
    AmountOfReadingsSpeed = RemapedAmountOfReadings2;  // Set amount of readings as the remaped value.
  }
  else
  {
    PulseCounterSpeed++;  // Increase the counter for amount of readings by 1.
    PeriodSumSpeed = PeriodSumSpeed + PeriodBetweenSpeedPulses;  // Add the periods so later we can average.
  }

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
// Reply event
void requestEvent()
{
  // Prepare the response buffer with the rpm then speed data as 32bit numbers in 8 bit packets
  i2c_buffer[0] = (uint8_t)(averageRPM & 0xFF); // LSB
  i2c_buffer[1] = (uint8_t)((averageRPM >> 8) & 0xFF);
  i2c_buffer[2] = (uint8_t)((averageRPM >> 16) & 0xFF);
  i2c_buffer[3] = (uint8_t)((averageRPM >> 24) & 0xFF); // MSB
  i2c_buffer[4] = (uint8_t)(averageSpeed & 0xFF);       // LSB
  i2c_buffer[5] = (uint8_t)((averageSpeed >> 8) & 0xFF);
  i2c_buffer[6] = (uint8_t)((averageSpeed >> 16) & 0xFF);
  i2c_buffer[7] = (uint8_t)((averageSpeed >> 24) & 0xFF); // MSB
  Wire.write(i2c_buffer, BUFFER_SIZE);

  // Clear registers
  // rpm = 0;
  // speed = 0;
}