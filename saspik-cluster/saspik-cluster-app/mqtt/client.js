const mqtt = require('mqtt');

const brokerUrl = 'mqtt://localhost:1883';
const topic = 'sensor/unitId1/dht22';
const clientId = '';
const username = 'admin';
const password = 'password123'; // This should match the password in passwordfile

// Use MQTT 5 to receive reason codes for authorization errors
const client = mqtt.connect(brokerUrl, {
  clientId,
  protocolVersion: 5,
  username,
  password
});

// Error handling
client.on('error', (err) => {
  console.error('MQTT client error event:', err);
  client.end();
});

client.on('close', () => {
  console.log('MQTT client closed');
  client.end();
});

client.on('offline', () => {
  console.log('MQTT client offline');
  client.end();
});

client.on('connect', () => {
  console.log('Connected to MQTT broker (MQTT 5)');
  
  // Subscribe to the topic
  client.subscribe(topic, (err) => {
    if (err) {
      console.error('Subscription error:', err);
    } else {
      console.log(`Subscribed to ${topic}`);
    }
  });
  setTimeout(() => {
    // Publish with QoS 1 to receive PUBACK with reason code
    client.publish(topic, '{"value": 35, "unit": "C", "timestamp": "2025-01-01T12:00:00Z"}', { qos: 1 }, (err) => {
      if (err) {
        console.error('Publish error callback:', err);
        console.error('Error code:', err.code);
        console.error('Error message:', err.message);
        client.end();
        return;
        // The error will contain reason code 135 (Not authorized)
        // You can handle denial here
      } else {
        console.log(`Published message to ${topic} (callback success)`);
        console.log('Note: Success callback indicates broker accepted the publish.');
      }
    });
  }, 1000);
});

client.on('message', (receivedTopic, message) => {
  console.log(`Received message on ${receivedTopic}: ${message.toString()}`);
});

// Optional: listen to packetreceive for debugging
client.on('packetreceive', (packet) => {
  if (packet.cmd === 'puback' && packet.reasonCode !== 0) {
    console.log(`DEBUG: PUBACK with reason code ${packet.reasonCode} (denial)`);
  }
});