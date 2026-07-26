FROM node:18-alpine AS builder
# Установка минимального набора зависимостей для сборки
RUN apk add --no-cache python3 make g++

WORKDIR /app
COPY package.json ./
COPY package-lock.json ./

# Установка зависимостей
RUN npm ci --include=dev

COPY . ./
RUN npm run build

# Вторая стадия - только копирование статики
FROM alpine:latest
RUN apk --no-cache add ca-certificates
WORKDIR /app
COPY --from=builder /app/build ./build