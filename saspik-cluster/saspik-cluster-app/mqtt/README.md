[↑ ATSAP Cluster App](../README.md)

# MQTT Broker Configuration

This directory contains the Eclipse Mosquitto MQTT broker configuration for the ATSAP cluster application.

## Overview

The MQTT broker provides real-time messaging between:
- IoT devices/clients
- Backend server (REST API)
- WebSocket clients

## Files

- `Dockerfile` - Builds Mosquitto 2.0 image with custom config
- `mosquitto.conf` - Main broker configuration
- `mosquitto.acl` - Access Control List (topic permissions)
- `client.js` - Minimal test client
- `test-*.js` - Various test scripts

## Configuration Details

### Broker Settings (`mosquitto.conf`)
- Listens on port 1883 (MQTT) and 9001 (WebSockets)
- **Authentication enabled** (`allow_anonymous false`)
- Password file: `/mosquitto/config/passwordfile`
- Debug logging enabled
- ACL file: `/mosquitto/config/mosquitto.acl`

### Access Control (`mosquitto.acl`)

Current ACL provides:
1. **Default deny** — все неразрешённые топики блокируются
2. **Client namespaces**: `clients/%c/#` — каждый клиент может писать/читать в свой namespace
3. **Топики датчиков**: `sensor/#` — publish/subscribe для сенсоров
4. **Топики LED** (dev): `led/#` — publish/subscribe для LED-устройств
5. **Топики команд юнитов**: `units/#` — publish/subscribe для управления реле (добавлено 2026-07-27)

### Authentication Details

Authentication has been enabled with the following credentials:
- Username: `admin`
- Password: `password123`

The password file is generated using `mosquitto_passwd` and included in the Docker image.

To add more users or change passwords:
```bash
# Enter the container
docker exec -it atsap_mosquitto /bin/sh

# Add a new user
mosquitto_passwd -b /mosquitto/config/passwordfile username password

# Or change password for existing user
mosquitto_passwd -b /mosquitto/config/passwordfile admin newpassword
```

For production with clientId-based restrictions:
1. Remove the `pattern readwrite #` line (already commented out)
2. Authentication is already enabled in `mosquitto.conf`
3. Password file is already configured
4. Update ACL to use `user` or `client` directives if needed

## Client ID Based Topic Separation

The user requested: "для clientId=1 будут доступны топики 'test1', для clientId=2 будут доступны топики 'test2'"

### Implemented Solution
Due to Mosquitto ACL limitations with anonymous access, we implemented namespace-based separation:
- Client "1" → topics: `clients/1/test1`, `clients/1/#`
- Client "2" → topics: `clients/2/test2`, `clients/2/#`

### Alternative Approaches
1. **With authentication**: Use `client` directive in ACL for exact topic names
2. **Application-level validation**: Validate clientId in `LocalMqttService`
3. **Topic rewriting**: Server rewrites `test1` → `clients/1/test1`

## Testing

### Quick Test
```bash
cd mqtt
node client.js
```

**Note**: The client now uses authentication with username `admin` and password `password123`.

### Test Authentication
```bash
# Test basic authentication
node test_auth.js

# Test authentication with specific client ID
node test_auth_clientid2.js
```

### Test Client ID Restrictions
```bash
node test-clientid.js
```

### REST API Tests
```bash
# Publish message
curl -X POST http://localhost:3001/api/mqtt/publish \
  -H "Content-Type: application/json" \
  -d '{"topic":"test/topic","message":"Hello"}'

# Subscribe to topic
curl -X POST http://localhost:3001/api/mqtt/subscribe \
  -H "Content-Type: application/json" \
  -d '{"topic":"test/topic"}'
```

## Docker Deployment

The broker is included in `docker-compose.yml` as service `mosquitto`.

### Rebuild and restart:
```bash
docker compose build mosquitto
docker compose up -d mosquitto
```

### View logs:
```bash
docker compose logs -f mosquitto
```

## Troubleshooting

### Broker not starting
Check ACL syntax:
```bash
docker run --rm -it eclipse-mosquitto:2.0 mosquitto -c /mosquitto/config/mosquitto.conf --test
```

### Connection refused
- Ensure port 1883 is not blocked
- Check if another Mosquitto instance is running
- Verify Docker container is up: `docker ps | grep mosquitto`

### ACL not enforcing
- Mosquitto requires authentication for `client` directives
- With `allow_anonymous true`, only `pattern` directives work
- Check broker logs for ACL parse errors

## Next Steps for Production

1. **Authentication**: Already enabled with username/password authentication
2. **TLS/SSL**: Configure encrypted connections for secure communication
3. **Persistent storage**: Mount volume for `/mosquitto/data` to retain messages across restarts
4. **Monitoring**: Add health checks and metrics for broker performance
5. **High availability**: Consider cluster setup for multiple brokers
6. **ACL refinement**: Implement more granular access control using `user` or `client` directives in ACL

## Структура каталога

```
.
├── Dockerfile
├── client.js
├── mosquitto.acl
├── mosquitto.conf
├── package.json
├── passwordfile
└── README.md
```