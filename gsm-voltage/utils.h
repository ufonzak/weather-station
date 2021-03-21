typedef unsigned int siz_t;

bool startsWith(const char *str, const char *prefix)
{
  return strncmp(prefix, str, strlen(prefix)) == 0;
}

void trimSpaces(char* line) {
  int len = strlen(line);
  while(len > 0 && isspace((unsigned char)line[len - 1])) {
    len--;
    line[len] = '\0';
  }
  int start = 0;
  while(isspace((unsigned char)line[start])) {
    start++;
  }
  if (start > 0) {
    strcpy(line, line + start);
  }
}

siz_t serial1ReadUntil(char until, char* buffer, siz_t bufferSize, int timeout) {
  siz_t read;
  do {
    wdt_reset();
    read = Serial1.readBytesUntil(until, buffer, bufferSize - 1);
  } while(read == 0 && --timeout > 0);
  buffer[read] = '\0';
  return read;
}

inline siz_t serial1ReadLine(char* buffer, siz_t bufferSize, int timeout) {
  return serial1ReadUntil('\n', buffer, bufferSize, timeout);
}

siz_t serialReadLine(char* buffer, siz_t bufferSize) {
  siz_t read = Serial.readBytesUntil('\n', buffer, bufferSize - 1);
  buffer[read] = '\0';
  return read;
}

void safeDelay(unsigned int delayTime) {
  wdt_reset();
  delay(delayTime);
}

void appendComma(char** bufferPtr) {
  (*bufferPtr)[0] = ',';
  ++(*bufferPtr);
}

void appendFloat(char** bufferPtr, float value) {
  dtostrf(value, 4, 2, *bufferPtr);
  *bufferPtr += strlen(*bufferPtr);
}
