FROM node:18-alpine AS frontend-builder
RUN apk add --no-cache python3 make g++
WORKDIR /app
COPY client/package.json client/package-lock.json ./
RUN npm ci --include=dev
COPY client/ ./
RUN npm run build

FROM nginx:alpine
RUN rm /etc/nginx/conf.d/default.conf
COPY nginx/nginx.conf /etc/nginx/conf.d/default.conf
COPY --from=frontend-builder /app/build /usr/share/nginx/html
