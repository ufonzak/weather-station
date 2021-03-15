#define GSM_STATUS_PIN (11)
#define GSM_POWER_PIN (12)
#define GSM_RESET_PIN (10)

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

  Serial1.println("AT");

  int seconds = 10;
  char buffer[16];
  do {
    size_t read = Serial1.readBytes(buffer, sizeof(buffer));
    Serial.write(buffer, read);
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

  char buffer[16];
  do {
    size_t read = Serial1.readBytes(buffer, sizeof(buffer));
    Serial.write(buffer, read);
  } while (Serial1.available());

  Serial.println("OFF");
  return true;
}

bool readReply(char* reply, size_t bufferSize) {
  size_t read = serial1ReadLine(reply, bufferSize - 1);
  if (!read) {
    return false;
  }
  if (strcmp(reply, "\r") != 0) {
    Serial.print("Unexpected return (expected \\r): \"");
    Serial.print(reply);
    Serial.println("\"");
    return false;
  }

  read = serial1ReadLine(reply, bufferSize - 1);
  if (!read) {
    Serial.print("No reply");
    return false;
  }

  trimSpaces(reply);
  return true;
}


bool readOk() {
  char buffer[8];
  if (!readReply(buffer, sizeof(buffer))) {
    Serial.print("NOK: \"");
    Serial.print(buffer);
    Serial.println("\"");
    return false;
  }

  return strcmp(buffer, "OK") == 0;
}


bool queryCmd(const char* toSend, char* reply, size_t bufferSize) {
  Serial1.println(toSend);
  if (!readReply(reply, bufferSize)) {
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

bool setCmd(const char* toSend) {
  Serial1.println(toSend);
  return readOk();
}

bool sendSms1(const char* number, char* buffer, size_t bufferSize) {
  Serial1.print("AT+CMGS=\"");
  Serial1.print(number);
  Serial1.println("\"");

  size_t read = Serial1.readBytesUntil(' ', buffer, bufferSize - 1);
  buffer[read] = '\0';
  trimSpaces(buffer);

  if (strcmp(buffer, ">") != 0) {
    Serial.print("No prompt: \"");
    Serial.print(buffer);
    Serial.println("\"");
    return false;
  }

  return true;
}

bool waitForData(int secondsToWait) {
  while(!Serial1.available() && --secondsToWait > 0) {
    delay(1000);
  }
  return secondsToWait > 0;
}

bool sendSms2(char* buffer, size_t bufferSize) {
  Serial1.write(26);
  Serial1.flush();

  if (!waitForData(15)) {
    return false;
  }

  if (!readReply(buffer, bufferSize)) {
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

bool waitForRegistration(int secondsToWait, char* buffer, size_t bufferSize) {
  for (int att = secondsToWait; att > 0; att--) {
    if (!queryCmd("AT+CGREG?", buffer, bufferSize)) {
      return false;
    }

    if (strcmp(buffer, "0,1") == 0) {
      Serial.println("On Network");
      return true;
    }

    Serial.print("Waiting for network: ");
    Serial.println(buffer);
    delay(1000);
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
