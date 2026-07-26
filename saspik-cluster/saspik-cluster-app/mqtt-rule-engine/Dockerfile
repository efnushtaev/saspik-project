FROM node:22-alpine AS builder

WORKDIR /build

COPY package*.json ./
RUN npm ci --ignore-scripts

COPY tsconfig.json ./
COPY src/ ./src/
RUN npm run build

FROM node:22-alpine

RUN apk add --no-cache tini

WORKDIR /app

COPY package*.json ./
RUN npm ci --omit=dev --ignore-scripts && npm cache clean --force

COPY --from=builder /build/dist/ ./dist/
COPY rules.json ./

USER node

ENV NODE_ENV=production \
    MQTT_BROKER_URL=mqtt://localhost:1883 \
    MQTT_USERNAME=admin \
    MQTT_PASSWORD=password123 \
    CONFIG_PATH=./rules.json

HEALTHCHECK --interval=30s --timeout=10s --retries=3 --start-period=15s \
  CMD node -e "process.exit(0)" || exit 1

ENTRYPOINT ["/sbin/tini", "--"]
CMD ["node", "dist/src/index.js"]
