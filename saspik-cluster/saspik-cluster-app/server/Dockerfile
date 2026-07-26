FROM node:18-alpine AS builder
# Установка минимального набора зависимостей для сборки
RUN apk add --no-cache python3 make g++

WORKDIR /app
COPY package.json ./
COPY package-lock.json ./

# Установка всех зависимостей, включая devDependencies для сборки
RUN npm ci && npm cache clean --force

COPY . ./
RUN npm run build

# Финальный этап с минимальным образом
FROM node:18-alpine AS production
WORKDIR /app

# Копируем только production зависимости и собранное приложение
COPY --from=builder /app/package.json ./
COPY --from=builder /app/package-lock.json ./
COPY --from=builder /app/dist ./dist

# Установка только production зависимостей
RUN npm ci --only=production && npm cache clean --force

EXPOSE 3001
# Указываем команду запуска
CMD ["node", "dist/index.js"]
