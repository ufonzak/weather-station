#define GSM_STATUS_PIN (11)
#define GSM_POWER_PIN (12)
#define GSM_RESET_PIN (10)

void pipeData(int seconds) {
  char buffer[16];
  do {
    wdt_reset();
    siz_t read = Serial1.readBytes(buffer, sizeof(buffer));
    Serial.write(buffer, read);
  } while (Serial1.available() || --seconds > 0);
}

bool turnOn() {
  Serial.println("Powering On...");

  bool isOn = digitalRead(GSM_STATUS_PIN) == HIGH;
  if (isOn) {
    return true;
  }

  digitalWrite(GSM_POWER_PIN, LOW);
  safeDelay(2000);
  digitalWrite(GSM_POWER_PIN, HIGH);

  for (int att = 5; att > 0 && !isOn; att--) {
    safeDelay(1000);
    isOn = digitalRead(GSM_STATUS_PIN) == HIGH;
  }
  if (!isOn) {
    Serial.println("Cannot turn on");
    return false;
  }

  Serial1.println("AT");
  pipeData(10);

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
  safeDelay(2000);
  digitalWrite(GSM_POWER_PIN, HIGH);

  for (int att = 5; att > 0 && isOn; att--) {
    safeDelay(1000);
    isOn = digitalRead(GSM_STATUS_PIN) == HIGH;
  }
  if (isOn) {
    Serial.println("Cannot turn off");
    return false;
  }

  pipeData(0);

  Serial.println("OFF");
  return true;
}

bool readReply(char* reply, siz_t bufferSize, int timeout) {
  siz_t read = serial1ReadLine(reply, bufferSize, timeout);
  if (read == 0) {
    return false;
  }
  if (strcmp(reply, "\r") != 0) {
    Serial.print("(expected \\r): \"");
    Serial.print(reply);
    Serial.println("\"");
    return false;
  }

  read = serial1ReadLine(reply, bufferSize, timeout);
  if (read == 0) {
    Serial.print("No reply");
    return false;
  }

  trimSpaces(reply);
  return true;
}

bool readExpected(const char* expected, char* buffer, siz_t bufferSize, int timeout = 1) {
  if (!readReply(buffer, bufferSize, timeout)) {
    return false;
  }

  if (strcmp(buffer, expected) != 0) {
    Serial.print("Unexpected: \"");
    Serial.print(buffer);
    Serial.print("!=");
    Serial.print(expected);
    Serial.println("\"");
    return false;
  }

  return true;
}

bool readOk(int timeout = 1) {
  char buffer[8];
  return readExpected("OK", buffer, sizeof(buffer), timeout);
}

bool queryCmd(const char* toSend, char* reply, siz_t bufferSize, int timeout = 1) {
  Serial1.println(toSend);
  if (!readReply(reply, bufferSize, timeout)) {
    return false;
  }
  char* replyBegin = strchr(reply, ':');
  if (!replyBegin || strncmp(toSend + 2, reply, replyBegin - reply) != 0) {
    Serial.print("Error (query resp): \"");
    Serial.print(reply);
    Serial.println("\"");
    return false;
  }

  strcpy(reply, replyBegin + 1);
  trimSpaces(reply);

  return readOk();
}

bool setCmd(const char* toSend, int timeout = 1) {
  Serial1.println(toSend);
  return readOk(timeout);
}

/*
bool waitForData(int secondsToWait) {
  while(!Serial1.available() && --secondsToWait > 0) {
    safeDelay(1000);
  }
  return secondsToWait > 0;
}

bool sendSms1(const char* number, char* buffer, siz_t bufferSize) {
  Serial1.print("AT+CMGS=\"");
  Serial1.print(number);
  Serial1.println("\"");

  siz_t read = serial1ReadUntil(' ', buffer, bufferSize - 1);
  trimSpaces(buffer);

  if (strcmp(buffer, ">") != 0) {
    Serial.print("No prompt: \"");
    Serial.print(buffer);
    Serial.println("\"");
    return false;
  }

  return true;
}

bool sendSms2(char* buffer, siz_t bufferSize) {
  Serial1.write(26);
  Serial1.flush();

  if (!readReply(buffer, bufferSize, 15)) {
    return false;
  }
  if (!startsWith(buffer, "+CMGS")) {
    Serial.print("Unexpected reply (sms): ");
    Serial.print(buffer);
    Serial.println("\"");
    return false;
  }

  return readOk();
}
*/

bool waitForRegistration(int secondsToWait, char* buffer, siz_t bufferSize) {
  for (int att = secondsToWait; att > 0; att--) {
    if (!queryCmd("AT+CGREG?", buffer, bufferSize)) {
      return false;
    }

    if (strcmp(buffer, "0,1") == 0) {
      Serial.println("On Network");
      return true;
    }
    
    safeDelay(1000);
  }
  return false;
}

bool waitForGprs(int secondsToWait, char* buffer, siz_t bufferSize) {
  for (int att = secondsToWait; att > 0; att--) {
    if (!queryCmd("AT+SAPBR=2,1", buffer, bufferSize)) {
      return false;
    }
    if (startsWith(buffer, "1,1,")) {
      Serial.print("GPRS: ");
      Serial.println(buffer);
      return true;
    }

    safeDelay(1000);
  }
  return false;
}

void setupGsm()
{
  pinMode(GSM_STATUS_PIN, INPUT);
  pinMode(GSM_POWER_PIN, OUTPUT);
  pinMode(GSM_RESET_PIN, OUTPUT);

  digitalWrite(GSM_RESET_PIN, HIGH);
  digitalWrite(GSM_POWER_PIN, HIGH);

  Serial1.setTimeout(1000);
  Serial1.begin(9600);
}
