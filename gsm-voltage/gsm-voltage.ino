#define VIN_VOLTAGE_PIN (A0)
#define BAT_VOLTAGE_PIN (A1)
#define SOLAR_VOLTAGE_PIN (A2)
#define VOLTAGE_REF_COEF (0.006472)
#define VOLTAGE_SOLAR_COEF (0.01030)

// #define GATEWAY "7785225231"
#define GATEWAY "2267814018"

#define SOLAR_POWER_ENABLED_PIN (6)

#define INFO_PERIOD (3600)

#include "gsm.h"

int infoSeconds = 0;

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
  Serial1.print(info);
  Serial1.print(",");
  Serial1.print(voltageVin);
  Serial1.print(",");
  Serial1.print(voltageBattery);
  Serial1.print(",");
  Serial1.print(voltageSolar);
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
  }
}

void setup()
{
  setupGsm();

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BAT_VOLTAGE_PIN, INPUT);
  pinMode(SOLAR_VOLTAGE_PIN, INPUT);
  pinMode(VIN_VOLTAGE_PIN, INPUT);

  pinMode(SOLAR_POWER_ENABLED_PIN, OUTPUT);
  digitalWrite(SOLAR_POWER_ENABLED_PIN, HIGH);

  Serial.begin(9600);
  Serial1.begin(9600);

  infoSeconds = INFO_PERIOD - 120;
}

void loop()
{
  procesInput();
  readSolarVoltage();

  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN, LOW);
  delay(800);

  if (++infoSeconds >= INFO_PERIOD) {
    infoSeconds = 0;

    turnOn();
    sendInfo();
    turnOff();

    resetSolarVoltage();
  }
}
