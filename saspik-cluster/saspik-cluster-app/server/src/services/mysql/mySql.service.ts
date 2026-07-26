import mysql from "mysql2/promise";
import { inject, injectable } from "inversify";

import { MySQL } from ".";
import { TYPES } from "../../types";
import { IConfigService } from "../../config/config.service.interface";

@injectable()
export class MySQLService implements MySQL {
  private pool;

  constructor(@inject(TYPES.ConfigService) config: IConfigService) {
    this.pool = mysql.createPool({
      port: Number(config.get("MYSQL_PORT")) || 3306,
      host: config.get("MYSQL_HOST") || "mysql",
      user: config.get("MYSQL_USER") || "app_user",
      password: config.get("MYSQL_PASSWORD") || "secure_password",
      database: config.get("MYSQL_DATABASE") || "users_db",
      waitForConnections: true,
      connectionLimit: 10,
      queueLimit: 0,
    });
  }

  public async initializeDatabase(maxRetries = 30, delayMs = 2000) {
    for (let attempt = 1; attempt <= maxRetries; attempt++) {
      try {
        const connection = await this.pool.getConnection();
        await connection.ping();
        connection.release();

        // Создание таблицы и начальных данных
        await this.pool.execute(`
				CREATE TABLE IF NOT EXISTS clickCount (
				id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT PRIMARY KEY,
				count_value VARCHAR(255) NOT NULL,
				created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP
				) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4
			`);

        await this.pool.execute(`
				INSERT IGNORE INTO clickCount (id, count_value) VALUES (1, '0')
			`);

        console.log("Database initialized successfully");
        return;
      } catch (error) {
        console.error(
          `Attempt ${attempt}/${maxRetries}: Database initialization failed -`,
          (error as Error).message,
        );

        if (attempt === maxRetries) {
          console.error("All connection attempts failed");
        }

        await new Promise((resolve) => setTimeout(resolve, delayMs));
      }
    }
  }
}
