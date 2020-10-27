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

  const battery = {
    'Dimensions': dimensions,
    'MeasureName': 'battery_percent',
    'MeasureValue': `${data[1]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };

  const batteryVoltage = {
    'Dimensions': dimensions,
    'MeasureName': 'battery_voltage',
    'MeasureValue': `${data[2] / 1000}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };

  const vinVoltage = {
    'Dimensions': dimensions,
    'MeasureName': 'vin_voltage',
    'MeasureValue': `${data[3]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };
  const batteryVoltage2 = {
    'Dimensions': dimensions,
    'MeasureName': 'battery_voltage2',
    'MeasureValue': `${data[4]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };
  const solarVoltage = {
    'Dimensions': dimensions,
    'MeasureName': 'solar_voltage',
    'MeasureValue': `${data[5]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  };

  return [battery, batteryVoltage, vinVoltage, batteryVoltage2, solarVoltage];
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
