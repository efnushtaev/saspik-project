#!/usr/bin/env node

/**
 * Главный входной файл MQTT Rule Engine Worker.
 * Запускает движок с конфигурацией из файла rules.json и включает горячую перезагрузку.
 */

import { MqttAdapter } from './mqtt';
import { RuleEngine } from './core';
import { ConfigWatcher } from './config';

/**
 * Основная функция запуска воркера.
 */
async function main() {
  console.log('=== MQTT Rule Engine Worker ===');

  // Параметры из переменных окружения
  const brokerUrl = process.env.MQTT_BROKER_URL;
  const configPath = process.env.CONFIG_PATH;
  const mqttUsername = process.env.MQTT_USERNAME;
  const mqttPassword = process.env.MQTT_PASSWORD;

  console.log(`Брокер: ${brokerUrl}`);
  console.log(`Конфигурация: ${configPath}`);
  
  // Опции подключения с аутентификацией
  const connectOptions = {
    username: mqttUsername,
    password: mqttPassword,
    keepalive: 60,
    reconnectPeriod: 1000,
    connectTimeout: 30 * 1000,
    clientid: '',
    clean: true,
  };
  console.log(`connectOptions: ${JSON.stringify(connectOptions)}`);

  // Создаём адаптер и движок
  const adapter = new MqttAdapter(brokerUrl, connectOptions);
  const engine = new RuleEngine(adapter);

  try {
    // Подключаемся к брокеру
    await adapter.connect();
    console.log('Подключение к брокеру успешно');

    // Запускаем наблюдатель за конфигурацией
    const watcher = new ConfigWatcher(configPath);
    watcher.start((rules) => {
      engine.loadRules(rules).catch((err) => {
        console.error('Ошибка загрузки правил:', err);
      });
    });

    // Обработка сигналов завершения
    process.on('SIGINT', async () => {
      console.log('\nПолучен SIGINT, завершаем работу...');
      watcher.stop();
      await adapter.disconnect();
      process.exit(0);
    });

    process.on('SIGTERM', async () => {
      console.log('\nПолучен SIGTERM, завершаем работу...');
      watcher.stop();
      await adapter.disconnect();
      process.exit(0);
    });

    console.log('Воркер запущен. Ожидание сообщений...');
    console.log('Нажмите Ctrl+C для остановки.');

    // Бесконечный цикл (можно заменить на что-то более элегантное)
    await new Promise(() => {});
  } catch (err) {
    console.error('Критическая ошибка при запуске:', err);
    process.exit(1);
  }
}

// Запуск
if (require.main === module) {
  main();
}

export { MqttAdapter, RuleEngine, ConfigWatcher };