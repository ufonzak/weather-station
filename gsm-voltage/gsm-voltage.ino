#define NO_PORTC_PINCHANGES
#define NO_PORTD_PINCHANGES

#include <PinChangeInt.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define VIN_VOLTAGE_PIN (A0)
#define BAT_VOLTAGE_PIN (A1)
#define SOLAR_VOLTAGE_PIN (A2)
#define VOLTAGE_REF_COEF (0.006472)
#define VOLTAGE_SOLAR_COEF (0.01030)

// #define GATEWAY "7785225231"
#define GATEWAY "2267814018"

#define SOLAR_POWER_ENABLED_PIN (6)

#define BREATHE_LED (200)

#include "gsm.h"

unsigned long cycleStart;
unsigned long cycleStartSecond;

#include "weather.h"

unsigned long solarVoltageSum;
unsigned long solarVoltageCount;

float getSolarVoltageAvg() {
  if (solarVoltageCount == 0) {
    return 0;
  }
  return (solarVoltageSum / solarVoltageCount) * VOLTAGE_SOLAR_COEF;
}

void readSolarVoltage() {
  int voltage = analogRead(SOLAR_VOLTAGE_PIN);
  unsigned long oldSum = solarVoltageSum;
  solarVoltageSum += (unsigned long)voltage;
  if (oldSum > solarVoltageSum) {
    Serial.println("Solar voltage sum overflow");
    solarVoltageSum = voltage;
    solarVoltageCount = 0;
  }
  solarVoltageCount++;
}

void resetSolarVoltage() {
  solarVoltageSum = 0;
  solarVoltageCount = 0;
}

bool sendInfo() {
  if (!waitForRegistration(30)) {
    return false;
  }

  String info;
  if (!queryCmd("AT+CBC", info)) {
    return false;
  }

  float voltageVin = analogRead(VIN_VOLTAGE_PIN) * VOLTAGE_REF_COEF;
  float voltageBattery = analogRead(BAT_VOLTAGE_PIN) * VOLTAGE_REF_COEF;
  float voltageSolar = getSolarVoltageAvg();

  if (!sendSms1(GATEWAY)) {
    return false;
  }
  Serial1.print(cycleStartSecond / 3600);
  Serial1.print(",");
  Serial1.print(info);
  Serial1.print(",");
  Serial1.print(voltageVin);
  Serial1.print(",");
  Serial1.print(voltageBattery);
  Serial1.print(",");
  Serial1.print(voltageSolar);

  printAnemo(false, ANEMO_SAMPLES_COUNT);
  printAnemo(false, 10);
  printDirection(false, DIRECTION_DISTRIBUTION_SAMPLES);
  printDirection(false, 2);
  printPrecipitation(false, true);
  printBmeData(false);
  
  if (!sendSms2()) {
    return false;
  }

  return true;
}

void procesInput() {
  if (!Serial.available()) {
    return;
  }

  String cmd = Serial.readStringUntil('\n');
  if (cmd.startsWith("AT")) {
    Serial1.println(cmd);

    while(true) {
      cmd = Serial1.readStringUntil('\n');
      if (cmd.length() == 0){
        break;
      }
      Serial.println(cmd);
    }
  } else if (cmd.compareTo("test") == 0) {
    turnOn();
    if (!sendInfo()) {
      Serial.println("sendInfo failed");
    }
    turnOff();
  } else if (cmd.compareTo("on") == 0) {
    turnOn();
  } else if (cmd.compareTo("off") == 0) {
    turnOff();
  } else if (cmd.compareTo("analog") == 0) {
    float voltageVin = analogRead(VIN_VOLTAGE_PIN) * VOLTAGE_REF_COEF;
    float voltageBattery = analogRead(BAT_VOLTAGE_PIN) * VOLTAGE_REF_COEF;
    float voltageSolar = analogRead(SOLAR_VOLTAGE_PIN) * VOLTAGE_SOLAR_COEF;
    Serial.print(voltageVin);
    Serial.print(",");
    Serial.print(voltageBattery);
    Serial.print(",");
    Serial.print(voltageSolar);
    Serial.println();
  } else if (cmd.compareTo("direct") == 0) {
    while(true) {
      if (Serial1.available()) {
        cmd = Serial1.readString();
        Serial.print(cmd);
      }
      if (Serial.available()) {
        cmd = Serial.readString();
        Serial1.print(cmd);
      }
    }
  } else if (cmd.compareTo("anemo") == 0) {
    printAnemo(true, ANEMO_SAMPLES_COUNT);
    printAnemo(true, 10);
  } else if (cmd.compareTo("direction") == 0) {
    printDirection(true, DIRECTION_DISTRIBUTION_SAMPLES);
    printDirection(true, 2);
  } else if (cmd.compareTo("rain") == 0) {
    printPrecipitation(true, false);
  } else if (cmd.compareTo("bme") == 0) {
    printBmeData(true);
  }
}

void setup()
{
  setupGsm();
  setupWeather();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BAT_VOLTAGE_PIN, INPUT);
  pinMode(SOLAR_VOLTAGE_PIN, INPUT);
  pinMode(VIN_VOLTAGE_PIN, INPUT);

  pinMode(SOLAR_POWER_ENABLED_PIN, OUTPUT);
  digitalWrite(SOLAR_POWER_ENABLED_PIN, HIGH);

  Serial.begin(9600);
  Serial1.begin(9600);

  delay(2000);
}

void loop()
{
  cycleStart = millis();
  cycleStartSecond = cycleStart / 1000;
  unsigned int secondOfHour = cycleStartSecond % 3600;
  
  procesInput();
  readSolarVoltage();
  measureDirection();
  measureAnemo();

  if (secondOfHour == 3599 || cycleStartSecond == 120) {
    turnOn();
    sendInfo();
    turnOff();

    resetSolarVoltage();
  }

  /*if (cycleStartSecond % 600 == 599) {    // TODO: remove
    turnOn();
    sendInfo();
    turnOff();
    
    printAnemo(true, ANEMO_SAMPLES_COUNT);
    printAnemo(true, 10);
    
    printDirection(true, DIRECTION_DISTRIBUTION_SAMPLES);
    printDirection(true, 2);
  }*/

  int leftOfSecond = 1000 - (int)(millis() - cycleStart);

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
