#define ANEMO_PIN (14)
#define DIRECTION_PIN (A3)

#define RAIN_PIN (16)
#define RAIN_BUCKET_COEF (0.2794)
volatile unsigned int rainCount = 0;   
// #define DEBUG

typedef struct {
  byte avg;
  byte gust;
} AnemoSample;

#define ANEMO_SAMPLES_COUNT (60)
AnemoSample anemoSamples[ANEMO_SAMPLES_COUNT];
unsigned int anemoSamplesIndex = -1;

#define ANEMO_MEASURING_PERIOD (60000)
volatile unsigned int anemoCount = 0;   
unsigned long lastAnemoMeasurement;

#define ANEMO_GUST_MEASURING_PERIOD (5000) // must divide ANEMO_MEASURING_PERIOD without remainder
unsigned int anemoLastGustCount;
unsigned long lastAnemoGustMeasurement;
byte anemoMaxGust;   

void anemoAddCount();
void rainAddCount();

#define WIND_DIR_MAX_VOLTAGE_DEVIATION (10)
#define DIRECTION_COUNT (8)
#define DIRECTION_DISTRIBUTION_SAMPLES (12)
unsigned int lastDirectionIndex = -1;
byte directionDistribution[DIRECTION_DISTRIBUTION_SAMPLES][DIRECTION_COUNT];

Adafruit_BME280 bme;

void resetAnemoGust();
void setupBme();

void setupWeather() {
  setupBme();

  pinMode(ANEMO_PIN, INPUT);     
  pinMode(DIRECTION_PIN, INPUT);  

  lastAnemoMeasurement = millis();
  lastAnemoGustMeasurement = lastAnemoMeasurement;
  resetAnemoGust();
  PCintPort::attachInterrupt(ANEMO_PIN, anemoAddCount, RISING);
  
  PCintPort::attachInterrupt(RAIN_PIN, rainAddCount, RISING);

  for (int i = 0; i < DIRECTION_DISTRIBUTION_SAMPLES; i++) {
    for (int d = 0; d < DIRECTION_COUNT; d++) {
      directionDistribution[i][d] = 0;
    }
  }
}

void measureAnemoGust(unsigned long now) {
  long measuringPeriod = now - lastAnemoGustMeasurement;  
  if (measuringPeriod < ANEMO_GUST_MEASURING_PERIOD) {
    return;
  }

  lastAnemoGustMeasurement = now;
  unsigned int count = anemoCount;
  unsigned int gustCount = count - anemoLastGustCount;
  anemoLastGustCount = count;

  byte gustSpeed = (byte)(2.41f * ((float)gustCount / ((float)measuringPeriod / 1000.f)));
  
  if (gustSpeed > anemoMaxGust) {
    anemoMaxGust = gustSpeed;
  }
}

void resetAnemoGust() {
  anemoLastGustCount = 0; 
  anemoMaxGust = 0;
}

void measureAnemo() {
  unsigned long now = millis();
  measureAnemoGust(now);
  
  long measuringPeriod = now - lastAnemoMeasurement;
  if (measuringPeriod < ANEMO_MEASURING_PERIOD) {
    return;
  }
  
  unsigned int count = anemoCount;
  anemoCount = 0;
  lastAnemoMeasurement = now;

  byte speedAvg = (byte)(2.41f * ((float)count / ((float)measuringPeriod / 1000.f)));
  
  byte maxGust = anemoMaxGust;
  resetAnemoGust();
  
  if (anemoSamplesIndex < 0) {
    for (int i = 0; i < ANEMO_SAMPLES_COUNT; i++) {
      anemoSamples[i].avg = speedAvg;
      anemoSamples[i].gust = maxGust;
    }
  }
  anemoSamplesIndex = (anemoSamplesIndex + 1) % ANEMO_SAMPLES_COUNT;
  anemoSamples[anemoSamplesIndex].avg = speedAvg;
  anemoSamples[anemoSamplesIndex].gust = maxGust;

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
  if (abs(voltage - 235) < WIND_DIR_MAX_VOLTAGE_DEVIATION || abs(voltage - 318) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 0;
  } else if (abs(voltage - 134) < WIND_DIR_MAX_VOLTAGE_DEVIATION || abs(voltage - 194) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 1;    
  } else if (abs(voltage - 77) < WIND_DIR_MAX_VOLTAGE_DEVIATION || abs(voltage - 422) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 2;    
  } else if (abs(voltage - 390) < WIND_DIR_MAX_VOLTAGE_DEVIATION || abs(voltage - 777) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 3;    
  } else if(abs(voltage - 734) < WIND_DIR_MAX_VOLTAGE_DEVIATION || abs(voltage - 897) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 4;    
  } else if (abs(voltage - 838) < WIND_DIR_MAX_VOLTAGE_DEVIATION || abs(voltage - 957) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 5;    
  } else if (abs(voltage - 930) < WIND_DIR_MAX_VOLTAGE_DEVIATION || abs(voltage - 940) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 6;    
  } else if (abs(voltage - 560) < WIND_DIR_MAX_VOLTAGE_DEVIATION || abs(voltage - 615) < WIND_DIR_MAX_VOLTAGE_DEVIATION) {
    return 7;    
  }
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
  byte maxGust = 0;
  for (int i = 0; i < samplesBack; i++) {
    int index = (anemoSamplesIndex + ANEMO_SAMPLES_COUNT - i) % ANEMO_SAMPLES_COUNT;
    if (anemoSamples[index].gust > maxGust) {
      maxGust = anemoSamples[index].gust;
    }
    averageSpeed += anemoSamples[index].avg;
  }
  averageSpeed /= samplesBack;

  unsigned long  stdDevSum = 0;
  for (int i = 0; i < samplesBack; i++) {
    int index = (anemoSamplesIndex + ANEMO_SAMPLES_COUNT - i) % ANEMO_SAMPLES_COUNT;
    stdDevSum += (anemoSamples[index].avg - averageSpeed) * (anemoSamples[index].avg - averageSpeed);
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
    Serial.print(maxGust);
    Serial.println();
  } else {
    Serial1.print(",");
    Serial1.print(averageSpeed);
    Serial1.print(",");
    Serial1.print(stdDevSpeed);
    Serial1.print(",");
    Serial1.print(maxGust);
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
    if (total > 0) {
      dist[d] = 100 * dist[d] / total;
    }
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

void rainAddCount() {
  ++rainCount;
}

void printPrecipitation(bool debug, bool clearValue) {
  unsigned int count;
  if (clearValue) {
    count = rainCount;
    rainCount = 0;
  } else {
    count = rainCount;
  }

  float precipitation = count * RAIN_BUCKET_COEF;
  if (debug) {
    Serial.print("Precipitation ");
    Serial.print(count);
    Serial.print(" ");
    Serial.println(precipitation);
  } else {
    Serial1.print(",");
    Serial1.print(precipitation);
  }
}

void setupBme() {
  bme.begin(0x76);
}

void printBmeData(bool debug) {
  float temperature = bme.readTemperature();
  bool isBroken = temperature != temperature || temperature < -140.0f;
  if (isBroken) {
    Serial.println("Restarting BME280");
    setupBme();
    temperature = bme.readTemperature();
  }
  
  float pressure = bme.readPressure() / 100.0F;
  int humidity = bme.readHumidity();
  
  if (debug) {
    Serial.print("BME280 ");
    Serial.print(temperature);
    Serial.print(" ");
    Serial.print(pressure);
    Serial.print(" ");
    Serial.println(humidity);
  } else {
    Serial1.print(",");
    Serial1.print(temperature);
    Serial1.print(",");
    Serial1.print(pressure);
    Serial1.print(",");
    Serial1.print(humidity);
  }
}
