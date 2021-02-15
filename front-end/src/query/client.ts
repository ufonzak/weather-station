
import TimestreamQuery from 'aws-sdk/clients/timestreamquery';
import { environment } from '../environment';

export const client = new TimestreamQuery({
  region: environment.region,
  credentials: {
    accessKeyId: environment.accessKeyId,
    secretAccessKey: environment.secretAccessKey,
  },
});
