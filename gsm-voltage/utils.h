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

size_t serial1ReadUntil(char until, char* buffer, size_t bufferSize) {
  wdt_reset();
  size_t read = Serial1.readBytesUntil(until, buffer, bufferSize - 1);
  buffer[read] = '\0';
  return read;
}

inline size_t serial1ReadLine(char* buffer, size_t bufferSize) {
  return serial1ReadUntil('\n', buffer, bufferSize);
}

size_t serialReadLine(char* buffer, size_t bufferSize) {
  size_t read = Serial.readBytesUntil('\n', buffer, bufferSize - 1);
  buffer[read] = '\0';
  return read;
}

void safeDelay(unsigned int delayTime) {
  wdt_reset();
  delay(delayTime);
}
