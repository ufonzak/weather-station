const https = require('https');
const agent = new https.Agent({
  maxSockets: 5000,
});
const AWS = require('aws-sdk');
const writeClient = new AWS.TimestreamWrite({
  maxRetries: 10,
  httpOptions: {
    timeout: 20000,
    agent: agent,
  },
});

function processRecord(record) {
  const message = JSON.parse(record.Sns.Message);
  const text = message.messageBody.replace(/\s/g, '');
  const data = text.split(',').map(value => +value);

  const ts = record.Sns.Timestamp ? new Date(record.Sns.Timestamp).valueOf() : Date.now();

  const dimensions = [
    {'Name': 'station', 'Value': message.originationNumber},
  ];

  const uptime = {
    'Dimensions': dimensions,
    'MeasureName': 'uptime',
    'MeasureValue': `${data[0]}`,
    'MeasureValueType': 'BIGINT',
    'Time': ts.toString()
  };

  const battery = {
    'Dimensions': dimensions,
    'MeasureName': 'battery_percent',
    'MeasureValue': `${data[2]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };

  const batteryVoltage = {
    'Dimensions': dimensions,
    'MeasureName': 'battery_voltage',
    'MeasureValue': `${data[3] / 1000}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };

  const vinVoltage = {
    'Dimensions': dimensions,
    'MeasureName': 'vin_voltage',
    'MeasureValue': `${data[4]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };
  const batteryVoltage2 = {
    'Dimensions': dimensions,
    'MeasureName': 'battery_voltage2',
    'MeasureValue': `${data[5]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };
  const solarVoltage = {
    'Dimensions': dimensions,
    'MeasureName': 'solar_voltage',
    'MeasureValue': `${data[6]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };

  const windSpeedAvg1h = {
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_1h_avg',
    'MeasureValue': `${data[7]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };
  const windSpeedStdDev1h = {
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_1h_std_dev',
    'MeasureValue': `${data[8]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };
  const windSpeedMax1h = {
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_1h_max',
    'MeasureValue': `${data[9]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };

  const windSpeedAvg10m = {
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_10m_avg',
    'MeasureValue': `${data[10]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };
  const windSpeedStdDev10m = {
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_10m_std_dev',
    'MeasureValue': `${data[11]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };
  const windSpeedMax10m = {
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_10m_max',
    'MeasureValue': `${data[12]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };

  const records = [
    uptime, battery, batteryVoltage, vinVoltage, batteryVoltage2, solarVoltage,
    windSpeedAvg1h, windSpeedStdDev1h, windSpeedMax1h,
    windSpeedAvg10m, windSpeedStdDev10m, windSpeedMax10m
  ];

  for (let direction = 0; direction < 8; direction++) {
    records.push({
      'Dimensions': dimensions,
      'MeasureName': `wind_direction${direction}_1h`,
      'MeasureValue': `${data[13 + direction]}`,
      'MeasureValueType': 'DOUBLE',
      'Time': ts.toString()
    });
  }

  for (let direction = 0; direction < 8; direction++) {
    records.push({
      'Dimensions': dimensions,
      'MeasureName': `wind_direction${direction}_10m`,
      'MeasureValue': `${data[21 + direction]}`,
      'MeasureValueType': 'DOUBLE',
      'Time': ts.toString()
    });
  }

  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'precipitation',
    'MeasureValue': `${data[29]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  });

  return records;
}

exports.handler = async (event) => {
  const records = [];
  for (const record of event.Records) {
    records.push(...processRecord(record));
  }
  const params = {
    DatabaseName: 'weather',
    TableName: 'records',
    Records: records,
  };
  await writeClient.writeRecords(params).promise();
};
