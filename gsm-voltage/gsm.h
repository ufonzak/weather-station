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
    Serial.print(reply);
    Serial.println("\"");
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
    Serial.print(reply);
    Serial.println("\"");
    return false;
  }

  reply = line.substring(dataBegin + 1);
  reply.trim();

  line = line.substring(0, dataBegin);
  String cmdPart(toSend + 2);
  if (!cmdPart.startsWith(line)) {
    Serial.print("Not a valid reply: ");
    Serial.print(line);
    Serial.println("\"");
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
    Serial.print(line);
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
    Serial.print(reply);
    Serial.println("\"");
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
