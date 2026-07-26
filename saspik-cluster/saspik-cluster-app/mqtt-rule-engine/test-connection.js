const mqtt = require('mqtt');

const client = mqtt.connect('mqtt://localhost:1883', {
  username: 'admin',
  password: 'password123',
  keepalive: 60,
  reconnectPeriod: 1000,
  connectTimeout: 30000,
  clientId: '',
  clean: true,
});

client.on('connect', () => {
  console.log('Connected successfully');
  client.end();
  process.exit(0);
});

client.on('error', (err) => {
  console.error('Connection error:', err);
  process.exit(1);
});

setTimeout(() => {
  console.error('Timeout');
  process.exit(1);
}, 5000);