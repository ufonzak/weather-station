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

function processRecord(event) {
  const { station } = event.pathParameters;
  let rawData = event.body;
  if (event.isBase64Encoded) {
    rawData = Buffer.from(rawData, 'base64').toString('ascii');
  }
  const data = rawData.split(',').map(value => +value);
  console.log(station, rawData);

  const dataFormatVersion = data[0];
  if (dataFormatVersion !== 1) {
    throw new Error(`Invalid version ${dataFormatVersion}`);
  }
  if (data.length !== 34) {
    throw new Error(`Invalid data length ${data.length}`);
  }

  const ts = event.requestContext.timeEpoch || Date.now();

  const dimensions = [
    {'Name': 'station', 'Value': station},
  ];
  const records = [];

  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'uptime',
    'MeasureValue': `${data[1]}`,
    'MeasureValueType': 'BIGINT',
    'Time': ts.toString(),
  });

  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'battery_percent',
    'MeasureValue': `${data[2]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });

  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'battery_voltage',
    'MeasureValue': `${data[3] / 1000}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });
  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'battery_temperature_max',
    'MeasureValue': `${data[4]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });

  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'vin_voltage',
    'MeasureValue': `${data[5]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });
  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'battery_voltage2',
    'MeasureValue': `${data[6]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });
  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'solar_voltage',
    'MeasureValue': `${data[7]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });

  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_1h_avg',
    'MeasureValue': `${data[8]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });
  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_1h_std_dev',
    'MeasureValue': `${data[9]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });
  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_1h_max',
    'MeasureValue': `${data[10]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });

  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_10m_avg',
    'MeasureValue': `${data[11]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });
  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_10m_std_dev',
    'MeasureValue': `${data[12]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });
  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'wind_speed_10m_max',
    'MeasureValue': `${data[13]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString(),
  });

  for (let direction = 0; direction < 8; direction++) {
    records.push({
      'Dimensions': dimensions,
      'MeasureName': `wind_direction${direction}_1h`,
      'MeasureValue': `${data[14 + direction]}`,
      'MeasureValueType': 'DOUBLE',
      'Time': ts.toString()
    });
  }

  for (let direction = 0; direction < 8; direction++) {
    records.push({
      'Dimensions': dimensions,
      'MeasureName': `wind_direction${direction}_10m`,
      'MeasureValue': `${data[22 + direction]}`,
      'MeasureValueType': 'DOUBLE',
      'Time': ts.toString()
    });
  }

  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'precipitation',
    'MeasureValue': `${data[30]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  });

  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'temperature',
    'MeasureValue': `${data[31]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  });

  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'pressure',
    'MeasureValue': `${data[32]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  });

  records.push({
    'Dimensions': dimensions,
    'MeasureName': 'humidity',
    'MeasureValue': `${data[33]}`,
    'MeasureValueType': 'DOUBLE',
    'Time': ts.toString()
  });

  const validRecords = records.filter(record => Number.isFinite(+record.MeasureValue));
  return validRecords;
}

exports.handler = async (event) => {
  const records = processRecord(event);
  const params = {
    DatabaseName: 'weather',
    TableName: 'records',
    Records: records,
  };
  await writeClient.writeRecords(params).promise();
  return { statusCode: 204 };
};

1,26,53,3825,19.92,4.43,3.82,4.90,0,0.00,0,0,0.00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0.00,23.11,1020.81,43
1, 0,51,3815,23.29,5.05,3.80,0.00,0,0.00,0,0,0.00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,00.00,0.00,0
