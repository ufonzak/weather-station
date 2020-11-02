#define GSM_STATUS_PIN (12)
#define GSM_POWER_PIN (5)
#define VIN_VOLTAGE_PIN (A0)
#define BAT_VOLTAGE_PIN (A1)
#define SOLAR_VOLTAGE_PIN (A2)
#define VOLTAGE_REF_COEF (0.006472)
#define VOLTAGE_SOLAR_COEF (0.01030)
// #define GATEWAY "7785225231" 
#define GATEWAY "2267814018"

#define PIN_DIRECT_SUPPLY (9)
#define PIN_REGULATED_SUUPLY (6)

#define INFO_PERIOD (3600)
int infoSeconds = 0;

bool turnOn() {
  Serial.println("Powering On...");
  
  bool isOn = digitalRead(GSM_STATUS_PIN) == HIGH;
  if (isOn) {
    return true;
  }

  digitalWrite(GSM_POWER_PIN, LOW);
  delay(2000);
  digitalWrite(GSM_POWER_PIN, HIGH);

  for (int att = 5; att > 0 && !isOn; att--) {
    delay(1000);
    isOn = digitalRead(GSM_STATUS_PIN) == HIGH;
  }
  if (!isOn) {
    Serial.println("Cannot turn on");
    return false;
  }

  int seconds = 10;
  do {
    Serial.print(Serial1.readString());
  } while (Serial1.available() || --seconds > 0);

  Serial.println("ON");
  return true;
}

bool turnOff() {
  Serial.println("Powering Off...");
  
  bool isOn = digitalRead(GSM_STATUS_PIN) == HIGH;
  if (!isOn) {
    return true;
  }
  
  digitalWrite(GSM_POWER_PIN, LOW);
  delay(2000);
  digitalWrite(GSM_POWER_PIN, HIGH);
  
  for (int att = 5; att > 0 && isOn; att--) {
    delay(1000);
    isOn = digitalRead(GSM_STATUS_PIN) == HIGH;
  }
  if (isOn) {
    Serial.println("Cannot turn off");
    return false;
  }

  do {
    Serial.print(Serial1.readString());
  } while (Serial1.available());
  
  Serial.println("OFF");
  return true;
}

bool readReply(String& reply) {
  reply = Serial1.readStringUntil('\n');
  if (reply.compareTo("\r") != 0) {
    Serial.print("Unexpected return (expected \\r): ");
    Serial.println(reply);
    return false;
  }
  
  reply = Serial1.readStringUntil('\n');
  if (reply.length() == 0) {
    Serial.print("No reply");
    return false;
  }
  
  reply.trim();
  return true;
}


bool readOk() {
  String reply;
  if (!readReply(reply)) {
    return false;
  }
  return reply.compareTo("OK") == 0;
}


bool queryCmd(const char* toSend, String& reply) {
  Serial1.println(toSend);
  String line; 
  if (!readReply(line)) {
    return false;
  }
  int dataBegin = line.indexOf(':');
  if (dataBegin < 0) {
    Serial.print("Error (query resp): ");
    Serial.println(reply);
    return false;
  }
  
  reply = line.substring(dataBegin + 1);
  reply.trim();
  
  line = line.substring(0, dataBegin);
  String cmdPart(toSend + 2);
  if (!cmdPart.startsWith(line)) {
    Serial.print("Not a valid reply: ");
    Serial.println(line);
    return false;
  }

  return readOk();
}

bool setCmd(const char* toSend) {
  Serial1.println(toSend);
  return readOk();
}

bool sendSms1(const char* number) {  
  Serial1.print("AT+CMGS=\"");
  Serial1.print(number);
  Serial1.println("\"");

  String line; 
  if (!readReply(line)) {
    return false;
  }
  if (line.compareTo(">") != 0) {
    Serial.print("No prompt: ");
    Serial.println(line);
  }

  return true;
}

bool waitForData(int secondsToWait) {
  while(!Serial1.available() && --secondsToWait > 0) {
    delay(1000);
  }
  return secondsToWait > 0;
}

bool sendSms2() {
  Serial1.write(26);
  Serial1.flush();
  
  if (!waitForData(15)) {
    return false;
  }
  
  String reply; 
  if (!readReply(reply)) {
    return false;
  }
  if (!reply.startsWith("+CMGS")) {
    Serial.print("Unexpected reply (sms): ");
    Serial.println(reply);
    return false;
  }  
  
  return readOk();
}

bool waitForRegistration(int secondsToWait) {
  String reply;
  for (int att = secondsToWait; att > 0; att--) {
    if (!queryCmd("AT+CGREG?", reply)) {
      return false;
    }
    
    if (reply.compareTo("0,1") == 0) {
      Serial.println("On Network");
      return true;
    }
    
    Serial.print("Waiting for network: ");
    Serial.println(reply);
    delay(1000);
  }
  return false;
}

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

bool isPowerSupplyDirect;

#define POWER_SUPPLY_DIRECT (true)
#define POWER_SUPPLY_REGULATED (false)

void setPowerSupply(bool directOrRegulated) {
  digitalWrite(directOrRegulated ? PIN_DIRECT_SUPPLY : PIN_REGULATED_SUUPLY, HIGH);
  delay(500);      
  digitalWrite(directOrRegulated ? PIN_DIRECT_SUPPLY : PIN_REGULATED_SUUPLY, LOW); 
  
  isPowerSupplyDirect = directOrRegulated; 
}

void checkPowerSupply() {
  float voltageSolar = analogRead(SOLAR_VOLTAGE_PIN) * VOLTAGE_SOLAR_COEF;

  bool shouldRegulate = voltageSolar > 6.5;
  bool shouldDirect = voltageSolar < 6.0;
  if (isPowerSupplyDirect && shouldRegulate) {
    setPowerSupply(POWER_SUPPLY_REGULATED);
  } else if (!isPowerSupplyDirect && shouldDirect) {
    setPowerSupply(POWER_SUPPLY_DIRECT);
  }
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(GSM_STATUS_PIN, INPUT);
  pinMode(GSM_POWER_PIN, OUTPUT);
  pinMode(BAT_VOLTAGE_PIN, INPUT);
  pinMode(SOLAR_VOLTAGE_PIN, INPUT);
  pinMode(VIN_VOLTAGE_PIN, INPUT);
  
  digitalWrite(GSM_POWER_PIN, HIGH);

  pinMode(PIN_DIRECT_SUPPLY, OUTPUT);
  pinMode(PIN_REGULATED_SUUPLY, OUTPUT);
  setPowerSupply(POWER_SUPPLY_DIRECT);
  
  Serial.begin(9600);
  Serial1.begin(9600);

  infoSeconds = INFO_PERIOD - 120;
}

void loop()
{
  procesInput();
  readSolarVoltage();
  checkPowerSupply();

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
