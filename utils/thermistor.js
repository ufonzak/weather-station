const THERMISTOR_RESISTOR2 = (10000);
const THERMISTOR_R = (10000);
const THERMISTOR_NOMINAL_TEMP = (25.0);
const THERMISTOR_BCOEFFICIENT = (3950.0);
const ABSOLUTE_ZERO_TEMP = (-273.15);

function compute(reading) {
  let resistance = 1023 / reading - 1;
  resistance = THERMISTOR_RESISTOR2 * resistance;

  let steinhart;
  steinhart = resistance / THERMISTOR_R;     // (R/Ro)
  steinhart = Math.log(steinhart);                // ln(R/Ro)
  steinhart /= THERMISTOR_BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (THERMISTOR_NOMINAL_TEMP - ABSOLUTE_ZERO_TEMP);  // + (1/To)
  steinhart = 1.0 / steinhart;
  steinhart += ABSOLUTE_ZERO_TEMP;             // convert absolute temp to C
  return steinhart
}

for (let i = 0; i <= 1023; i++) {
  console.log(i, compute(i));
}
