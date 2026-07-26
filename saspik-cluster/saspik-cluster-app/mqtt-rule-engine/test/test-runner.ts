import { MockMqttAdapter } from './mock-mqtt-adapter';
import { RuleEngine } from '../src/core';
import { RuleBuilder } from '../src/config';
import * as fs from 'fs';
import * as path from 'path';

/**
 * Тестовый запуск движка правил с мок-адаптером.
 */
async function runTest() {
  console.log('=== Запуск тестового клиента MQTT Rule Engine ===');

  // 1. Создаём мок-адаптер
  const mockAdapter = new MockMqttAdapter();
  await mockAdapter.connect();

  // 2. Создаём движок
  const engine = new RuleEngine(mockAdapter);

  // 3. Загружаем правила из sample-rules.json
  const configPath = path.join(__dirname, 'sample-rules.json');
  const configContent = fs.readFileSync(configPath, 'utf-8');
  const config = JSON.parse(configContent);
  const rules = RuleBuilder.buildFromConfig(config);

  await engine.loadRules(rules);
  console.log(`Загружено правил: ${rules.length}`);

  // // 4. Симулируем входящие сообщения
  // console.log('\n--- Симуляция сообщений ---');

  // // Сообщение, которое должно сработать по правилу 1 (температура > 30)
  // console.log('\n1. Отправляем sensors/temperature с value=35:');
  // mockAdapter.simulateMessage(
  //   'sensors/temperature',
  //   JSON.stringify({ value: 35, unit: 'C', timestamp: '2025-01-01T12:00:00Z' })
  // );

  // // Сообщение, которое НЕ должно сработать (температура <= 30)
  // console.log('\n2. Отправляем sensors/temperature с value=25:');
  // mockAdapter.simulateMessage(
  //   'sensors/temperature',
  //   JSON.stringify({ value: 25, unit: 'C' })
  // );

  // // Сообщение по wildcard топику sensors/room1/status с payload "OK"
  // console.log('\n3. Отправляем sensors/room1/status с payload "OK":');
  // mockAdapter.simulateMessage('sensors/room1/status', 'OK');

  // // Сообщение по другому wildcard device/server/online
  // console.log('\n4. Отправляем device/server/online с payload "online":');
  // mockAdapter.simulateMessage('device/server/online', 'online');

  // // Сообщение без условия (правило 3)
  // console.log('\n5. Отправляем control/light с JSON:');
  // mockAdapter.simulateMessage(
  //   'control/light',
  //   JSON.stringify({ command: 'turn_on', brightness: 80 })
  // );

  // // 5. Даём время на выполнение асинхронных действий
  // console.log('\n--- Ожидание завершения асинхронных действий ---');
  // await new Promise(resolve => setTimeout(resolve, 100));

  // // 6. Проверяем опубликованные сообщения
  // console.log('\n--- Опубликованные сообщения ---');
  // const published = mockAdapter.getPublishedMessages();
  // if (published.length === 0) {
  //   console.log('Нет опубликованных сообщений.');
  // } else {
  //   published.forEach((msg, idx) => {
  //     console.log(`${idx + 1}. Топик: ${msg.topic}`);
  //     console.log(`   Payload: ${msg.payload.toString()}`);
  //     console.log(`   QoS: ${msg.options?.qos ?? 0}, Retain: ${msg.options?.retain ?? false}`);
  //   });
  // }

  // // 7. Проверяем подписки
  // console.log('\n--- Активные подписки ---');
  // const subscriptions = mockAdapter.getSubscriptions();
  // console.log(Array.from(subscriptions).join(', ') || 'Нет подписок');

  // // 8. Завершаем
  // await mockAdapter.disconnect();
  // console.log('\n=== Тест завершён ===');
}

// Запускаем тест
runTest().catch((err) => {
  console.error('Ошибка при выполнении теста:', err);
  process.exit(1);
});