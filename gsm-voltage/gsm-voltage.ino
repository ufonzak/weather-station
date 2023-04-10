#define NO_PORTC_PINCHANGES
#define NO_PORTD_PINCHANGES

#include <PinChangeInt.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LowPower.h>
#include <avr/wdt.h>

#include "utils.h"

#define DATA_FORMAT_VERSION (1)
#define GENERIC_BUFFER (32)

#define VIN_VOLTAGE_PIN (A0)
#define BAT_VOLTAGE_PIN (A1)
#define SOLAR_VOLTAGE_PIN (A2)
#define BATTERY_TEMP_PIN (A4)
#define VOLTAGE_REF_COEF (0.006472)
#define VOLTAGE_REF_COEF_INT (6472ul)
#define VOLTAGE_SOLAR_COEF (0.01030)
#define VOLTAGE_SOLAR_COEF_INT (10300ul)

#define CHARGING_DISABLED_PIN (5)
#define IS_CHARGED_PIN (6)

#define GATEWAY "2267814018"

#define APN "internet.fido.ca"
#define APN_USER "fido"
#define APN_PWD "fido"
#define STATION "pemberton"

#define BREATHE_LED (200)

// TODO: sms received ignore (breaks GSM at this moments)

#include "gsm.h"

unsigned long cycleStart;
unsigned long cycleStartSecond;

#include "weather.h"

bool chargingDisabled = false;
bool cycleReady = true;

unsigned int sendFailCount = 0;

unsigned long solarVoltageSum;
unsigned long solarVoltageCount;
unsigned int batteryVoltage;

float getSolarVoltageAvg() {
  if (solarVoltageCount == 0) {
    return 0;
  }
  return (solarVoltageSum / solarVoltageCount) * VOLTAGE_SOLAR_COEF;
}
unsigned int getSolarVoltageAvgInt() {
  if (solarVoltageCount == 0) {
    return 0;
  }
  return (unsigned int)((solarVoltageSum / solarVoltageCount) * VOLTAGE_SOLAR_COEF_INT / 1000ul);
}

void readSolarVoltage() {
  int voltage = analogRead(SOLAR_VOLTAGE_PIN);
  unsigned long oldSum = solarVoltageSum;
  solarVoltageSum += (unsigned long)voltage;
  if (oldSum > solarVoltageSum) {
    solarVoltageSum = voltage;
    solarVoltageCount = 0;
  }
  solarVoltageCount++;
}

void resetSolarVoltage() {
  solarVoltageSum = 0;
  solarVoltageCount = 0;
}

#define THERMISTOR_RESISTOR2 (10000)
#define THERMISTOR_R (10000)
#define THERMISTOR_NOMINAL_TEMP (25.f)
#define THERMISTOR_BCOEFFICIENT (3950.f)
#define ABSOLUTE_ZERO_TEMP (-273.15f)

float maxBatteryTemperature = ABSOLUTE_ZERO_TEMP;

float readBatteryTemperature() {
  int reading = analogRead(BATTERY_TEMP_PIN);

  float resistance = 1023.f / reading - 1.f;
  resistance = THERMISTOR_RESISTOR2 * resistance;

  float steinhart;
  steinhart = resistance / THERMISTOR_R;     // (R/Ro)
  steinhart = log(steinhart);                // ln(R/Ro)
  steinhart /= THERMISTOR_BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (THERMISTOR_NOMINAL_TEMP - ABSOLUTE_ZERO_TEMP);  // + (1/To)
  steinhart = 1.0 / steinhart;
  steinhart += ABSOLUTE_ZERO_TEMP;             // convert absolute temp to C

  return steinhart;
}

void measureBatteryTemperature() {
  if (cycleStartSecond % 60 != 0) {
    return;
  }
  float temp = readBatteryTemperature();
  if (temp > maxBatteryTemperature) {
    maxBatteryTemperature = temp;
  }
}

void resetBatteryTemperature() {
  maxBatteryTemperature = ABSOLUTE_ZERO_TEMP;
}

unsigned int getBatteryVoltageInt() {
  return (unsigned int)(analogRead(BAT_VOLTAGE_PIN) * VOLTAGE_REF_COEF_INT / 1000ul);
}

unsigned int getInputVoltageInt() {
  return (unsigned int)(analogRead(VIN_VOLTAGE_PIN) * VOLTAGE_REF_COEF_INT / 1000ul);
}

siz_t prepareData(char* dataBuffer, char* buffer, siz_t bufferSize) {
  char* bufferPtr = dataBuffer;

  bufferPtr += sprintf(bufferPtr, "%d,%d", DATA_FORMAT_VERSION, cycleStartSecond / 3600);

  if (queryCmd("AT+CBC", buffer, bufferSize)) {
    strcpy(bufferPtr, buffer + 1);
    bufferPtr += strlen(bufferPtr);
  } else {
    bufferPtr += sprintf(bufferPtr, ",0,0");
  }

  {
    float voltageVin = analogRead(VIN_VOLTAGE_PIN) * VOLTAGE_REF_COEF;
    float voltageBattery = analogRead(BAT_VOLTAGE_PIN) * VOLTAGE_REF_COEF;
    float voltageSolar = getSolarVoltageAvg();

    appendComma(&bufferPtr);
    appendFloat(&bufferPtr, maxBatteryTemperature);
    appendComma(&bufferPtr);
    appendFloat(&bufferPtr, voltageVin);
    appendComma(&bufferPtr);
    appendFloat(&bufferPtr, voltageBattery);
    appendComma(&bufferPtr);
    appendFloat(&bufferPtr, voltageSolar);
  }

  printAnemo(&bufferPtr, ANEMO_SAMPLES_COUNT);
  printAnemo(&bufferPtr, 10);

  printDirection(&bufferPtr, DIRECTION_DISTRIBUTION_SAMPLES);
  printDirection(&bufferPtr, 2);

  printPrecipitation(&bufferPtr, true);
  printBmeData(&bufferPtr);

  int totalLength = bufferPtr - dataBuffer;
  return totalLength;
}

bool makeHttp(char* data, siz_t dataLength, char* buffer, siz_t bufferSize) {
  if (!setCmd("AT+HTTPINIT")) {
    return false;
  }
  if (!setCmd("AT+HTTPPARA=\"CID\",1")) {
    return false;
  }
  if (!setCmd("AT+HTTPPARA=\"URL\",\"http://weather.martinzak.me/api/data/"STATION"\"")) {
    return false;
  }

  Serial1.print("AT+HTTPDATA=");
  Serial1.print(dataLength);
  Serial1.println(",20000");
  if (!readExpected("DOWNLOAD", buffer, bufferSize, 5)) {
    return false;
  }

  Serial1.println(data);
  if (!readOk()) {
    return false;
  }

  if (!setCmd("AT+HTTPACTION=1")) {
    return false;
  }
  if (!readReply(buffer, bufferSize, 30)) {
    return false;
  }
  if (strcmp(buffer, "+HTTPACTION: 1,204,0") != 0) {
    Serial.print("HTTP Error:");
    Serial.println(buffer);
  }

  if (!setCmd("AT+HTTPTERM")) {
    return false;
  }
  return true;
}

bool sendData() {
  char buffer[GENERIC_BUFFER];

  char dataBuffer[192];
  siz_t totalLength = prepareData(dataBuffer, buffer, sizeof(buffer));

  if (!waitForRegistration(30, buffer, sizeof(buffer))) {
    return false;
  }

  if (!setCmd("AT+SAPBR=3,1,\"Contype\",\"GPRS\"")) {
    return false;
  }
  if (!setCmd("AT+SAPBR=3,1,\"APN\",\""APN"\"")) {
    return false;
  }
  if (!setCmd("AT+SAPBR=3,1,\"USER\",\""APN_USER"\"")) {
    return false;
  }
  if (!setCmd("AT+SAPBR=3,1,\"PWD\",\""APN_PWD"\"")) {
    return false;
  }

  if (!setCmd("AT+SAPBR=1,1", 10)) {
    return false;
  }
  if (!waitForGprs(30, buffer, sizeof(buffer))) {
    return false;
  }

  if(!makeHttp(dataBuffer, totalLength, buffer, sizeof(buffer))) {
    return false;
  }

  if (!setCmd("AT+SAPBR=0,1", 5)) {
    return false;
  }

  return true;
}

void procesInput() {
  if (!Serial.available()) {
    return;
  }

  char cmd[GENERIC_BUFFER];
  siz_t read = serialReadLine(cmd, sizeof(cmd));
  if (!read) {
    return;
  }

  if (startsWith(cmd, "AT")) {
    Serial.println(cmd);
    Serial1.println(cmd);

    while(true) {
      read = serial1ReadLine(cmd, sizeof(cmd), 1);
      if (!read){
        break;
      }
      Serial.println(cmd);
    }
  } else if (strcmp(cmd, "test") == 0) {
    turnOn();
    if (!sendData()) {
      Serial.println("sendData failed");
    } else {
      Serial.println("TEST OK");
    }
    // turnOff();
  } else if (strcmp(cmd, "on") == 0) {
    turnOn();
  } else if (strcmp(cmd, "off") == 0) {
    turnOff();
  } else if (strcmp(cmd, "analog") == 0) {
    float voltageVin = analogRead(VIN_VOLTAGE_PIN) * VOLTAGE_REF_COEF;
    float voltageBattery = analogRead(BAT_VOLTAGE_PIN) * VOLTAGE_REF_COEF;
    float voltageSolar = analogRead(SOLAR_VOLTAGE_PIN) * VOLTAGE_SOLAR_COEF;
    Serial.print(voltageVin);
    Serial.print(",");
    Serial.print(voltageBattery);
    Serial.print(",");
    Serial.print(voltageSolar);
    Serial.print(",");
    Serial.print(getInputVoltageInt());
    Serial.print(",");
    Serial.print(getBatteryVoltageInt());
    Serial.print(",");
    Serial.print(getSolarVoltageAvgInt());
    Serial.println();
  } else if (strcmp(cmd, "direct") == 0) {
    while(true) {
      wdt_reset();
      if (Serial1.available()) {
        read = Serial1.readBytes(cmd, sizeof(cmd));
        Serial.write(cmd, read);
      }
      if (Serial.available()) {
        read = Serial.readBytes(cmd, sizeof(cmd));
        if (strncmp(cmd, "quit", 4) == 0) {
          break;
        }
        Serial1.write(cmd, read);
      }
    }
  } else if (strcmp(cmd, "anemo") == 0) {
    printAnemo(NULL, ANEMO_SAMPLES_COUNT);
    printAnemo(NULL, 10);
  } else if (strcmp(cmd, "direction") == 0) {
    printDirection(NULL, DIRECTION_DISTRIBUTION_SAMPLES);
    printDirection(NULL, 2);
  } else if (strcmp(cmd, "rain") == 0) {
    printPrecipitation(NULL, false);
  } else if (strcmp(cmd, "bme") == 0) {
    printBmeData(NULL);
  } else if (strcmp(cmd, "battery_t") == 0) {
    Serial.println(analogRead(BATTERY_TEMP_PIN));
    Serial.println(readBatteryTemperature());
  }
}

void batteryManagement() {
  batteryVoltage = getBatteryVoltageInt();
  int thermistor = analogRead(BATTERY_TEMP_PIN);

  // values precalculated
  bool tempOk = 240 <= thermistor && thermistor <= 669; // <0.5,40>
  bool tempOkHyster = 245 <= thermistor && thermistor <= 620; //  <1,35>

  if (!chargingDisabled) {
    bool batteryCharged = digitalRead(IS_CHARGED_PIN) == HIGH && batteryVoltage >= 4050;
    if (batteryCharged) {
      cycleReady = false;
    }
    if (batteryCharged || !tempOk) {
      chargingDisabled = true;
      digitalWrite(CHARGING_DISABLED_PIN, HIGH);
    }
  } else if (chargingDisabled) {
    if (!cycleReady) {
      // reset on low battery or during night
      bool chargingReset = batteryVoltage < 3800 || (batteryVoltage < 4000 && getSolarVoltageAvgInt() < 2000);
      if (chargingReset) {
        cycleReady = true;
      }
    }
    if (cycleReady && tempOkHyster) {
      chargingDisabled = false;
      digitalWrite(CHARGING_DISABLED_PIN, LOW);
    }
  }
}

void setup()
{
  wdt_enable(WDTO_8S);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BAT_VOLTAGE_PIN, INPUT);
  pinMode(SOLAR_VOLTAGE_PIN, INPUT);
  pinMode(VIN_VOLTAGE_PIN, INPUT);
  pinMode(BATTERY_TEMP_PIN, INPUT);

  pinMode(IS_CHARGED_PIN, INPUT);
  pinMode(CHARGING_DISABLED_PIN, OUTPUT);
  digitalWrite(CHARGING_DISABLED_PIN, LOW);

  Serial.setTimeout(1000);
  Serial.begin(9600);

  setupGsm();
  setupWeather();

  digitalWrite(LED_BUILTIN, HIGH);
  safeDelay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.println("Start");
}

#define MIN_GSM_VOLTAGE (3650) // 30%
#define SLEEP_VOLTAGE (3570)   // 20%
#define SLEEP_ERR_DELAY (10)

void loop()
{
  wdt_reset();

  readSolarVoltage();
  batteryManagement();

  if (batteryVoltage <= SLEEP_VOLTAGE) {
    bool isUsb = getInputVoltageInt() > 4800;
    if (!isUsb) {
      LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_ON);
      return;
    }
  }

  cycleStart = millis();
  cycleStartSecond = cycleStart / 1000;
  unsigned int secondOfHour = cycleStartSecond % 3600;

  procesInput();
  measureDirection();
  measureAnemo();
  measureBatteryTemperature();

  bool sendBoost = secondOfHour % 600 == 599 && getSolarVoltageAvgInt() >= 6000;
  bool shouldSend = secondOfHour == 3599 || sendBoost || cycleStartSecond == 120;
  if (shouldSend) {
    if (batteryVoltage > MIN_GSM_VOLTAGE) {
      if (turnOn()) {
        if (sendData()) {
          sendFailCount = 0;
        } else {
          sendFailCount++;
        }
      }
      turnOff();

      if (sendFailCount >= 3) {
        delay(10000); // expire watchdog
      }
    }

    resetSolarVoltage();
    resetBatteryTemperature();
  }

  int leftOfSecond = 1000 - (int)(millis() % 1000) + SLEEP_ERR_DELAY;

  if (cycleStartSecond % 10 == 0 && leftOfSecond > BREATHE_LED) {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(BREATHE_LED);
    digitalWrite(LED_BUILTIN, LOW);
    leftOfSecond -= BREATHE_LED;
  }

  if (leftOfSecond > 0) {
    delay(leftOfSecond);
  }
}
