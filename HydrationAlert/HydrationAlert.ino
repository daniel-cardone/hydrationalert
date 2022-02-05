/**
 * 
 * Hydration Alert
 * Designed for Arduino Nano (ATmega328P Old Bootloader)
 * 
 */

#define SMAARTWIRE_CALIBRATION_SEQ        0x55

// TMP107 register map
#define TMP107_REG_TEMPERATURE            0x00
#define TMP107_REG_CONFIGURATION          0x01
#define TMP107_REG_HIGH_LIMIT_1           0x02
#define TMP107_REG_LOW_LIMIT_1            0x03
#define TMP107_REG_HIGH_LIMIT_2           0x04
#define TMP107_REG_LOW_LIMIT_2            0x05
#define TMP107_REG_EEPROM_1               0x06
#define TMP107_REG_EEPROM_2               0x07
#define TMP107_REG_EEPROM_3               0x08
#define TMP107_REG_EEPROM_4               0x09
#define TMP107_REG_EEPROM_5               0x0A
#define TMP107_REG_EEPROM_6               0x0B
#define TMP107_REG_EEPROM_7               0x0C
#define TMP107_REG_EEPROM_8               0x0D
#define TMP107_REG_DIE_ID                 0x0F

//TMP107_REG_CONFIGURATION
#define TMP107_CONV_RATE_15_MS            0b00000000
#define TMP107_CONV_RATE_50_MS            0b00100000
#define TMP107_CONV_RATE_100_MS           0b01000000
#define TMP107_CONV_RATE_250_MS           0b01100000
#define TMP107_CONV_RATE_500_MS           0b10000000
#define TMP107_CONV_RATE_1_S              0b10100000
#define TMP107_CONV_RATE_4_S              0b11000000
#define TMP107_CONV_RATE_16_S             0b11100000

// TMP107 commands
#define TMP107_CMD_ADDRESS_INIT           0x95
#define TMP107_CMD_ADDRESS_ASSIGN         0x0D
#define TMP107_CMD_LAST_DEVICE_POLL       0x57
#define TMP107_CMD_GLOBAL_SOFT_RST        0x5D
#define TMP107_CMD_GLOBAL_ALERT_CLR_1     0xB5
#define TMP107_CMD_GLOBAL_ALERT_CLR_2     0x75
#define TMP107_CMD_GLOBAL_READ            0b00000011
#define TMP107_CMD_GLOBAL_WRITE           0b00000001
#define TMP107_CMD_INDIVIDUAL_READ        0b00000010
#define TMP107_CMD_INDIVIDUAL_WRITE       0b00000000
#define TMP107_CMD_PTR_PADDING            0b10100000

// number of sensors in daisy chain
#define NUMBER_OF_SENSORS                 1

// include the library
#include <SmaartWire.h>

// the first TMP107 is connected to Arduino pin A0
// IMPORTANT: pull-up resistor MUST be used on the line!
//            the recommended value is 4.1k
//            (4.7k should be fine)
int outputPin = 4;
int inputPin = 7;
SmaartWire tmp107(A0);
bool canAlert = false;
bool shouldGetTemp = true;

// array to hold sensor addresses
byte addr[NUMBER_OF_SENSORS];

unsigned long previousMillis;
int delays[] = {400, 200, 400, 200, 400, 200, 200, 100, 150, 100};
unsigned long timeout = 5000;
int currentDelayIndex = 0;
int outputState = LOW;

// function to iniaitalize the sensor chain
// IMPORTANT: this function MUST be run before any measurements!
void addrInit(byte* addr) {
  // calibration phase
  tmp107.write(SMAARTWIRE_CALIBRATION_SEQ);

  // command phase
  tmp107.write(TMP107_CMD_ADDRESS_INIT);
  
  // address assign byte
  tmp107.write(TMP107_CMD_ADDRESS_ASSIGN);
  
  // data phase
  for(int i = 0; i < NUMBER_OF_SENSORS; i++) {
    addr[i] = (tmp107.read() & 0b11111000) >> 3;
  }

  // wait until address initialization is complete
  delay(1250);
}

// function the read temperature from sensor at a given address
float getTemp(byte addr) {
  // calibration phase
  tmp107.write(SMAARTWIRE_CALIBRATION_SEQ);

  // command and address phase
  tmp107.write(TMP107_CMD_INDIVIDUAL_READ | (addr << 3));

  // register pointer phase
  tmp107.write(TMP107_REG_TEMPERATURE | TMP107_CMD_PTR_PADDING);

  // data phase
  byte tmpLSB = tmp107.read();
  byte tmpMSB = tmp107.read();

  // convert raw 14-bit number to temperature
  int raw;
  float temp;
  if(tmpMSB & 0b10000000) {
    // temperature is negative, convert it and output
    raw = ~((tmpMSB << 6) | (tmpLSB >> 2)) + 1;
    raw &= 0x3FFF;
    temp = (float)raw * -0.015625;
  } else {
    // temperature is positive, just output
    raw = (tmpMSB << 6) | (tmpLSB >> 2);
    temp = (float)raw * 0.015625;
  }
  
  return(temp);
}

// function to set measurement interval
void setInterval(byte lastAddr, byte convRate) {
  // calibration phase
  tmp107.write(SMAARTWIRE_CALIBRATION_SEQ);

  // command and address phase
  tmp107.write(TMP107_CMD_GLOBAL_WRITE | (lastAddr << 3));

  // register pointer phase
  tmp107.write(TMP107_REG_CONFIGURATION | TMP107_CMD_PTR_PADDING);

  // data phase
  tmp107.write(0x00);
  tmp107.write(convRate);
}

void adjustDelay() {
  // get current temperature and convert to farenheit
  float tempC = getTemp(addr[0]);
  float tempF = tempC * 1.8 + 32;

  // apply the function to get the time in minutes
  // -0.0015x^2 + 30
  float res = -0.0015 * pow(tempF, 2) + 30;

  // apply a minimum delay of 8 minutes and maximum of 30
  float mins = constrain(res, 8, 30);

  // convert delay time in minute to milliseconds and adjust timeout
  unsigned long delayMs = mins * 60000;
  timeout = delayMs;
  shouldGetTemp = false;

  // serial monitoring
  Serial.println(tempF);
  Serial.println(mins);
}

void setup() {
  pinMode(outputPin, OUTPUT);
  pinMode(inputPin, INPUT);
  
  Serial.begin(9600);
  Serial.println();

  // initialize SmaartWire interface at 9600 baud
  tmp107.begin(9600);

  // initialize sensors
  Serial.println("Initializing sensors");
  addrInit(addr);

  // print the sensor addresses
  for(int i = 0; i < NUMBER_OF_SENSORS; i++) {
    char str[23];
    sprintf(str, "Sensor %d address: 0x%02x", i, addr[i]);
    Serial.println(str);
  }

  // set measurement intervals of all sensors to 100 ms
  setInterval(addr[NUMBER_OF_SENSORS - 1], TMP107_CONV_RATE_100_MS);

  // start by getting the current temperature right away
  adjustDelay();
}

void loop() {
  // get current system time
  unsigned long currentMillis = millis();

  // if the button is pressed
  if (digitalRead(inputPin) == HIGH) {
    if (shouldGetTemp) { // only the first time the input changes (1 instead of 0)
      adjustDelay(); // adjust the delay based on temperature
    }

    // stop alerting
    canAlert = false;
    digitalWrite(outputPin, LOW);
    outputState = LOW;
    previousMillis = currentMillis;
  } else {
    // if the button is not pressed, set a flag to prepare for it to change
    shouldGetTemp = true;
  }

  // if enough time has passed, cycle through vibration sequence
  if (canAlert && currentMillis - previousMillis >= delays[currentDelayIndex]) {
    previousMillis = currentMillis;

    if (outputState == LOW) {
      digitalWrite(outputPin, HIGH);
    } else {
      digitalWrite(outputPin, LOW);
    }

    outputState = !outputState;

    // go to next step in delay sequence
    currentDelayIndex++;
    if (currentDelayIndex >= sizeof(delays) / sizeof(delays[0])) {
      currentDelayIndex = 0;
    }
  } else if (!canAlert) {
    if (currentMillis - previousMillis >= timeout) {
      previousMillis = currentMillis;
      canAlert = true;
      currentDelayIndex = 0;
    }
  }
}
