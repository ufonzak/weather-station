  
String cmd;

#define GSM_STATUS_PIN (12)
#define GSM_POWER_PIN (5)

#define INFO_PERIOD (3600)
int infoSeconds = 0;

void turnOn() {
  Serial.println("Powering On");
  
  while (true) {
    bool isOn = digitalRead(GSM_STATUS_PIN) == HIGH;
    if (isOn) {
      break;
    }

    digitalWrite(GSM_POWER_PIN, LOW);
    delay(2000);
    digitalWrite(GSM_POWER_PIN, HIGH);
    delay(2000);
  }

  int seconds = 10;
  do {
    Serial.print(Serial1.readString());
  } while (Serial1.available() || --seconds > 0);

  Serial.println("ON");
}

void turnOff() {
  Serial.println("Powering Off");
  
  while (true) {
    bool isOn = digitalRead(GSM_STATUS_PIN) == HIGH;
    if (!isOn) {
      break;
    }

    digitalWrite(GSM_POWER_PIN, LOW);
    delay(2000);
    digitalWrite(GSM_POWER_PIN, HIGH);
    delay(2000);
  }

  do {
    Serial.print(Serial1.readString());
  } while (Serial1.available());
  
  Serial.println("OFF");
}

bool queryCmd(const char* toSend, String& reply) {
  Serial1.println(toSend);
  String line; 
  
  line = Serial1.readStringUntil('\n');
  line.trim();
  if (line.compareTo(toSend) != 0) {
    Serial.print("Unexpected echo: ");
    Serial.println(line);
    return false;
  }
  
  line = Serial1.readStringUntil('\n');
  line.trim();
  int dataBegin = line.indexOf(':');
  if (line.length() == 0 || line.compareTo("ERROR") == 0 || dataBegin < 0) {
    Serial.print("Error: ");
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
  
  line = Serial1.readStringUntil('\n');
  if (line.compareTo("\r") != 0) {
    Serial.print("Unexpected return: ");
    Serial.println(line);
    return false;
  }
  
  line = Serial1.readStringUntil('\n');
  line.trim();
  if (line.compareTo("OK") != 0) {
    Serial.print("NOK: ");
    Serial.println(line);
    return false;
  }
  return true;
}

bool setCmd(const char* toSend) {
  Serial1.println(toSend);
  String line; 
  
  line = Serial1.readStringUntil('\n');
  line.trim();
  if (line.compareTo(toSend) != 0) {
    Serial.print("Unexpected echo: ");
    Serial.println(line);
    return false;
  }
  
  line = Serial1.readStringUntil('\n');
  line.trim();
  if (line.compareTo("OK") != 0) {
    Serial.print("NOK: ");
    Serial.println(line);
    return false;
  }
  return true;
}

bool sendSms1(const char* number) {
  if (!setCmd("AT+CMGF=1")) {
    return false;
  }
  
  Serial1.print("AT+CMGS=\"");
  Serial1.print(number);
  Serial1.println("\"");
  
  String line; 
  line = Serial1.readStringUntil('\n');
  line.trim();
  
  if (!line.startsWith("AT+CMGS")) {
    Serial.print("Unexpected echo: ");
    Serial.println(line);
    return false;
  }

  line = Serial1.readString();
  line.trim();
  if (line.compareTo(">") != 0) {
    Serial.print("No prompt: ");
    Serial.println(line);
  }

  return true;
}

bool sendSms2() {
  Serial1.println();
  do {
    Serial1.readString();
    delay(100);
  } while (Serial1.available());
  
  Serial1.write(26);
  Serial1.flush();
  Serial1.read();
  
  String line; 
  int waitToSend = 10;
  do {
    line = Serial1.readStringUntil('\n');
    line.trim();
  } while (line.length() == 0 && --waitToSend > 0);
  
  if (!line.startsWith("+CMGS")) {
    Serial.print("Unexpected reply (sms): ");
    Serial.println(line);
    return false;
  }
  
  line = Serial1.readStringUntil('\n');
  if (line.compareTo("\r") != 0) {
    Serial.print("Unexpected return: ");
    Serial.println(line);
    return false;
  }
  
  line = Serial1.readStringUntil('\n');
  line.trim();
  if (line.compareTo("OK") != 0) {
    Serial.print("NOK: ");
    Serial.println(line);
    return false;
  }
  
  return true;
}

bool waitForRegistration() {
  while (true) {
    if (!queryCmd("AT+CGREG?", cmd)) {
      return false;
    }
    
    if (cmd.compareTo("0,1") == 0) {
      Serial.println("On Network");
      return true;
    }
    
    Serial.print("Waiting for network: ");
    Serial.println(cmd);
    delay(1000);
  }
}

bool sendInfo() {
  if (!waitForRegistration()) {
    return false;
  }

  String info;
  if (!queryCmd("AT+CBC", info)) {
    return false;
  }
  
  if (!sendSms1("7785225231")) {
    return false;
  }
  Serial1.print(info);
  if (!sendSms2()) {
    return false;
  }
  
  return true;
}


void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(GSM_STATUS_PIN, INPUT);
  pinMode(GSM_POWER_PIN, OUTPUT);
  digitalWrite(GSM_POWER_PIN, HIGH);
  
  Serial.begin(9600);
  Serial1.begin(9600);

  infoSeconds = INFO_PERIOD - 120;
}

void loop() // run over and over
{
  if (Serial.available()) {
    cmd = Serial.readStringUntil('\n');
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
    
  digitalWrite(LED_BUILTIN, HIGH);
  delay(200);
  
  if (++infoSeconds >= INFO_PERIOD) {
    infoSeconds = 0;
    
    turnOn();
    sendInfo();
    turnOff();
  }
  
  digitalWrite(LED_BUILTIN, LOW);
  delay(800);
}
