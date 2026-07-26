# API

## API кластера   
HTTP API для управления юнитами, объектами и правилами. Базовый URL: `http://<cluster-host>:<port>/api/v1`   
   
## Units   
|      Метод   <br> |              Путь   <br> |                                            Описание   <br> |
|:------------------|:-------------------------|:-----------------------------------------------------------|
|        GET   <br> |     `/units/list`   <br> |                         Получить список всех юнитов   <br> |
|       POST   <br> | `/units/getByIds`   <br> |                                Получить юниты по ID   <br> |

**POST /units/getByIds**   
```
{
  "id": ["unit-id-1", "unit-id-2"]
}

```
 --- 
   
## Objects   
| Метод   <br> | Путь   <br>                          | Описание   <br>                              |
| :----------- | :----------------------------------- | :------------------------------------------- |
| POST   <br>  | `/objects/list`   <br>               | Получить список объектов   <br>              |
| POST   <br>  | `/objects/getByIds`   <br>           | Получить объекты по ID   <br>                |
| POST   <br>  | `/objects/command/{deviceId}`   <br> | Отправить команду устройству   <br>          |
| POST   <br>  | `/objects/getLastSensorsData`   <br> | Получить последние показания сенсоров   <br> |

**POST /objects/list**   
```
{
  "type": "sensors" | "devices"
}

```
**POST /objects/getByIds**   
```
{
  "id": ["sensor-id-1", "device-id-1"],
}

```
**POST /objects/command/{deviceId}**   
```
{
  "value": "on"
}

```
**POST /objects/getLastSensorsData**   
```
{
  "id": ["sensor-id-1", "sensor-id-2"]
}

```
 --- 
   
## Rules   
|      Метод   <br> |              Путь   <br> |                                            Описание   <br> |
|:------------------|:-------------------------|:-----------------------------------------------------------|
|        GET   <br> |     `/rules/list`   <br> |                         Получить список всех правил   <br> |
|       POST   <br> | `/rules/getByIds`   <br> |                              Получить правила по ID   <br> |

**POST /rules/getByIds**   
```
{
  "id": ["rule-id-1", "rule-id-2"]
}

```
 --- 
   
## History   
|      Метод   <br> |            Путь   <br> |                                       Описание   <br> |
|:------------------|:-----------------------|:------------------------------------------------------|
|        GET   <br> | `/history/list`   <br> |                       Получить историю событий   <br> |

   
