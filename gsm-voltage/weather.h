#define ANEMO_PIN (14)
#define DIRECTION_PIN (A3)
// #define DEBUG

#define ANEMO_SAMPLES_COUNT (60)
byte anemoSamples[ANEMO_SAMPLES_COUNT];
unsigned int anemoSamplesIndex = -1;

#define ANEMO_MEASURING_PERIOD (60000)
volatile unsigned int anemoCount = 0;   
unsigned long lastAnemoMeasurement;

void anemoAddCount();

#define WIND_DIR_MAX_VOLTAGE_DEVIATION (10)
#define DIRECTION_COUNT (8)
#define DIRECTION_DISTRIBUTION_SAMPLES (12)
unsigned int lastDirectionIndex = -1;
byte directionDistribution[DIRECTION_DISTRIBUTION_SAMPLES][DIRECTION_COUNT];

void setupWeather() {
  pinMode(ANEMO_PIN, INPUT);     
  pinMode(DIRECTION_PIN, INPUT);  

  lastAnemoMeasurement = millis();
  PCintPort::attachInterrupt(ANEMO_PIN, anemoAddCount, RISING); // attach a PinChange Interrupt to our pin on the rising edge

  for (int i = 0; i < DIRECTION_DISTRIBUTION_SAMPLES; i++) {
    for (int d = 0; d < DIRECTION_COUNT; d++) {
      directionDistribution[i][d] = 0;
    }
  }
}


void measureAnemo() {
  unsigned long now = millis();
  long measuringPeriod = now - lastAnemoMeasurement;

  if (measuringPeriod < ANEMO_MEASURING_PERIOD) {
    return;
  }

  unsigned int count = anemoCount;
  anemoCount = 0;
  lastAnemoMeasurement = now;

  byte speedAvg = (byte)(2.41f * ((float)count / ((float)measuringPeriod / 1000.f)));
  
  if (anemoSamplesIndex < 0) {
    for (int i = 0; i < ANEMO_SAMPLES_COUNT; i++) {
      anemoSamples[i] = speedAvg;
    }
  }
  anemoSamplesIndex = (anemoSamplesIndex + 1) % ANEMO_SAMPLES_COUNT;
  anemoSamples[anemoSamplesIndex] = speedAvg;

#ifdef DEBUG
  Serial.print("Anemo ");
  Serial.println(speedAvg);
#endif
}

void anemoAddCount()
{
  ++anemoCount;
}

int readDirection() {
  int voltage = analogRead(DIRECTION_PIN);
  if (abs(voltage - 235) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 0;
  } else if (abs(voltage - 134) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 1;    
  } else if (abs(voltage - 77) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 2;    
  } else if (abs(voltage - 390) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 3;    
  } else if(abs(voltage - 734) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 4;    
  } else if (abs(voltage - 838) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 5;    
  } else if (abs(voltage - 930) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 6;    
  } else if (abs(voltage - 560) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 7;    
  }
  // TODO: add between (parallel resistors)
  return -1;
}

void measureDirection()  {
  if (cycleStartSecond % 2 != 0) {
    return;
  }
  
  int direction = readDirection();
#ifdef DEBUG
  Serial.print("Direction ");
  Serial.println(direction);
#endif
  
  if (direction < 0) {
    return;
  }

  int secondOfHour = cycleStartSecond % 3600;
  int sampleIndex = secondOfHour / (3600 / DIRECTION_DISTRIBUTION_SAMPLES);
  
  if (sampleIndex != lastDirectionIndex) {
    for (int i = 0; i < DIRECTION_COUNT; i++) {
      directionDistribution[sampleIndex][i] = 0;
    }
    lastDirectionIndex = sampleIndex;
  }

  directionDistribution[sampleIndex][direction]++;
}

void printAnemo(bool debug, int samplesBack) {
  unsigned int averageSpeed = 0;
  byte maxSpeed = 0;
  for (int i = 0; i < samplesBack; i++) {
    int index = (anemoSamplesIndex + ANEMO_SAMPLES_COUNT - i) % ANEMO_SAMPLES_COUNT;
    if (anemoSamples[index] > maxSpeed) {
      maxSpeed = anemoSamples[index];
    }
    averageSpeed += anemoSamples[index];
  }
  averageSpeed /= samplesBack;

  unsigned long  stdDevSum = 0;
  for (int i = 0; i < samplesBack; i++) {
    int index = (anemoSamplesIndex + ANEMO_SAMPLES_COUNT - i) % ANEMO_SAMPLES_COUNT;
    stdDevSum += (anemoSamples[index] - averageSpeed) * (anemoSamples[index] - averageSpeed);
  }
  float stdDevSpeed = sqrt(stdDevSum / (samplesBack - 1.f));

  if (debug) {
    Serial.print("Anemo stats ");
    Serial.print(samplesBack);
    Serial.print(" ");
    Serial.print(averageSpeed);
    Serial.print(" ");
    Serial.print(stdDevSpeed);
    Serial.print(" ");
    Serial.print(maxSpeed);
    Serial.println();
  } else {
    Serial1.print(",");
    Serial1.print(averageSpeed);
    Serial1.print(",");
    Serial1.print(stdDevSpeed);
    Serial1.print(",");
    Serial1.print(maxSpeed);
  }
}

void printDirection(bool debug, int samplesBack) {
  unsigned long dist[DIRECTION_DISTRIBUTION_SAMPLES];
  unsigned long total = 0;
  
  for (int d = 0; d < DIRECTION_COUNT; d++) {
    dist[d] = 0;
    for (int i = 0; i < samplesBack; i++) {
      int index = (lastDirectionIndex + DIRECTION_DISTRIBUTION_SAMPLES - i) % DIRECTION_DISTRIBUTION_SAMPLES;
      dist[d] += directionDistribution[index][d];
    }
    total += dist[d];
  }
  for (int d = 0; d < DIRECTION_COUNT; d++) {
    dist[d] = 100 * dist[d] / total;
  }
  
  if (debug) {
    Serial.print("Direction dist ");
    Serial.print(samplesBack);
    for (int d = 0; d < DIRECTION_COUNT; d++) {
      Serial.print(" ");
      Serial.print(dist[d]);
    }
    Serial.println();
  } else {
    for (int d = 0; d < DIRECTION_COUNT; d++) {
      Serial1.print(",");
      Serial1.print(dist[d]);
    }    
  }
}
