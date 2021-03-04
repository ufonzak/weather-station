#define NO_PORTC_PINCHANGES
#define NO_PORTD_PINCHANGES

#include <PinChangeInt.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LowPower.h>

#define DATA_FORMAT_VERSION (1)

#define VIN_VOLTAGE_PIN (A0)
#define BAT_VOLTAGE_PIN (A1)
#define SOLAR_VOLTAGE_PIN (A2)
#define BATTERY_TEMP_PIN (A4)
#define VOLTAGE_REF_COEF (0.006472)
#define VOLTAGE_REF_COEF_INT (6472ul)
#define VOLTAGE_SOLAR_COEF (0.01030)
#define VOLTAGE_SOLAR_COEF_INT (10300ul)

#define CHARGING_DISABLED_PIN (10)
#define IS_CHARGED_PIN (11)

// #define GATEWAY "7786812711"
#define GATEWAY "2267814018"

#define BREATHE_LED (200)

// TODO: watchdog

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
#ifdef DEBUG
    Serial.println("Solar voltage sum overflow");
#endif
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

bool sendInfo() {
  if (!waitForRegistration(60)) {
    return false;
  }

  String info;
  if (!queryCmd("AT+CBC", info)) {
    return false;
  }
  info = info.substring(2);

  float voltageVin = analogRead(VIN_VOLTAGE_PIN) * VOLTAGE_REF_COEF;
  float voltageBattery = analogRead(BAT_VOLTAGE_PIN) * VOLTAGE_REF_COEF;
  float voltageSolar = getSolarVoltageAvg();

  if (!sendSms1(GATEWAY)) {
    return false;
  }

  Serial1.print(DATA_FORMAT_VERSION);
  Serial1.print(",");
  Serial1.print(cycleStartSecond / 3600);
  Serial1.print(",");
  Serial1.print(info);
  Serial1.print(",");
  Serial1.print(maxBatteryTemperature);
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
    Serial.print(",");
    Serial.print(getBatteryVoltageInt());
    Serial.print(",");
    Serial.print(getSolarVoltageAvgInt());
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
  } else if (cmd.compareTo("resetbme") == 0) {
    setupBme();
  } else if (cmd.compareTo("battery_t") == 0) {
    Serial.println(analogRead(BATTERY_TEMP_PIN));
    Serial.println(readBatteryTemperature());
  }
}

bool chargingDisabled = false;

void batteryManagement() {
  unsigned int batteryVoltage = getBatteryVoltageInt();
  int thermistor = analogRead(BATTERY_TEMP_PIN);

  // values precalculated
  bool tempOk = 240 <= thermistor && thermistor <= 669; // <0.5,40>
  bool tempOkHyster = 245 <= thermistor && thermistor <= 620; //  <1,35>

  if (!chargingDisabled) {
    bool batteryCharged = digitalRead(IS_CHARGED_PIN) == HIGH && batteryVoltage >= 4050;
    if (batteryCharged || !tempOk) {
      chargingDisabled = true;
      digitalWrite(CHARGING_DISABLED_PIN, HIGH);
    }
  } else if (chargingDisabled) {
    // reset on low battery or during night
    bool chargingReset = batteryVoltage < 3800 || (batteryVoltage < 4000 && getSolarVoltageAvgInt() < 2000);
    if (chargingReset && tempOkHyster) {
      chargingDisabled = false;
      digitalWrite(CHARGING_DISABLED_PIN, LOW);
    }
  }
}

void setup()
{
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

  delay(2000);
  Serial.println("Start");
}

#define MIN_GSM_VOLTAGE (3650) // 30%
#define SLEEP_VOLTAGE (3570)   // 20%

void loop()
{
  // TODO: sms received ignore (breaks GSM at this moments)

  readSolarVoltage();
  batteryManagement();

  unsigned int batteryVoltage = getBatteryVoltageInt();
  if (batteryVoltage <= SLEEP_VOLTAGE) {
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    return;
  }

  cycleStart = millis();
  cycleStartSecond = cycleStart / 1000;
  unsigned int secondOfHour = cycleStartSecond % 3600;

  procesInput();
  measureDirection();
  measureAnemo();
  measureBatteryTemperature();

  bool sendBoost = secondOfHour % 900 == 899 && getSolarVoltageAvgInt() >= 6000;
  bool shouldSend = secondOfHour == 3599 || sendBoost || cycleStartSecond == 120;
  if (shouldSend) {
    if (batteryVoltage > MIN_GSM_VOLTAGE) {
      if (turnOn()) {
        sendInfo();
      }
      turnOff();
    }

    resetSolarVoltage();
    resetBatteryTemperature();
  }

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
