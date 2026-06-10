# SmartBasket

SmartBasket — это интеллектуальная корзина на базе ESP32 для ритейла, склада и учета остатков товара.

Корзина измеряет общий вес с помощью 4 тензодатчиков через 4 модуля HX711, рассчитывает количество оставшихся товаров по весу одной штуки, регистрируется в структуре `ритейлор -> магазин -> корзина` и отправляет данные на сервер.

## Возможности

- измерение веса по 4 углам через `HX711 x4`
- расчет общего веса
- расчет количества товаров по `unitWeight`
- расчет остатка, изъятого количества и процента заполнения
- хранение `basketId`, товара, расположения, ритейлора и магазина
- регистрация корзины по PIN ритейлора
- отправка данных по `HTTP`, `MQTT` и `ESP-NOW`
- локальный веб-интерфейс на ESP32
- хранение настроек в `NVS`
- загрузка фронтенда из `LittleFS`
- OLED-дисплей через `U8g2`
- калибровка из локального меню и веб-интерфейса
- Python-сервер с мониторингом и админкой

## Структура проекта

- [src](C:/Users/user/Documents/vesy/src) — прошивка ESP32
- [include](C:/Users/user/Documents/vesy/include) — заголовки и конфигурация
- [data](C:/Users/user/Documents/vesy/data) — веб-интерфейс корзины для `LittleFS`
- [server](C:/Users/user/Documents/vesy/server) — Python-сервер
- [platformio.ini](C:/Users/user/Documents/vesy/platformio.ini) — конфигурация PlatformIO

## Стек технологий

### Прошивка

- `ESP32 DevKit`
- `Arduino framework`
- `PlatformIO`
- `ArduinoJson`
- `U8g2`
- `PubSubClient`
- `HX711`
- `DHT sensor library`
- `Preferences / NVS`
- `WiFi`, `WebServer`, `HTTPClient`, `ESP-NOW`
- `LittleFS`

### Сервер

- `Python`
- `Flask`
- `SQLite`
- HTML-страницы для мониторинга и администрирования

## Оборудование

- `ESP32 DevKit`
- `4 x HX711`
- `4 x тензодатчик до 10 кг`
- `MPU-6050`
- `OLED 0.96" I2C`
- `DHT11`
- `2 кнопки`

## Распиновка

Пины заданы в [include/Config.h](C:/Users/user/Documents/vesy/include/Config.h).

### HX711

| Угол | HX711 DOUT | HX711 SCK |
| --- | --- | --- |
| 1 | GPIO `32` | GPIO `4` |
| 2 | GPIO `33` | GPIO `16` |
| 3 | GPIO `25` | GPIO `17` |
| 4 | GPIO `26` | GPIO `5` |

### Остальные устройства

| Устройство | Пин |
| --- | --- |
| DHT11 data | GPIO `27` |
| Кнопка меню | GPIO `18` |
| Кнопка выбора | GPIO `19` |
| I2C SDA | GPIO `21` |
| I2C SCL | GPIO `22` |

### MPU-6050

- I2C адрес: `0x68`
- `tiltX` и `tiltY` в телеметрии теперь берутся с MPU-6050 в градусах
- если MPU-6050 не ответил, прошивка оставляет прежний расчет `tiltX/tiltY` по распределению веса на 4 углах

### OLED

- драйвер в прошивке: `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`
- I2C-адрес: `0x3C`
- `USE_SH1106 = false` по умолчанию

Примечание:

- в прошивке сейчас нет явного `Wire.begin(SDA, SCL)`
- для типичного ESP32 DevKit это обычно означает стандартные I2C-пины:
  - `SDA -> GPIO 21`
  - `SCL -> GPIO 22`

Если на вашей плате дисплей подключен к другим линиям, это нужно будет отдельно задать в коде.

## Что хранит корзина

Корзина сохраняет:

- `basketId`
- `retailerId`
- `retailerName`
- `retailerPin`
- `storeId`
- `storeName`
- `basketLocation`
- `productName`
- `productCode`
- `registrationServerUrl`
- `wifiSsid`
- `wifiPassword`
- `mqttHost`
- `mqttPort`
- `httpEndpoint`
- `unitWeight`
- `fullWeight`
- `calibrationReferenceWeight`
- `calibration[4]`
- `changeThreshold`

## Что рассчитывает корзина

- `totalWeight`
- `quantity`
- `fullQuantity`
- `removedQuantity`
- `fillPercent`

## Сценарий регистрации корзины

Рабочий процесс такой:

1. Запускаете Python-сервер.
2. Создаете ритейлора в админке.
3. Получаете PIN ритейлора.
4. Создаете один или несколько магазинов для этого ритейлора.
5. Открываете веб-интерфейс корзины на ESP32.
6. Указываете адрес сервера регистрации.
7. Вводите PIN ритейлора.
8. Загружаете список магазинов.
9. Выбираете магазин.
10. Регистрируете корзину с `basketId`, товаром и физическим расположением.

После этого корзина отправляет телеметрию уже с привязкой к нужному ритейлору и магазину.

## Какие данные отправляет корзина

Корзина отправляет JSON такого типа:

```json
{
  "basketId": "basket-01",
  "retailerId": "retailer-01",
  "retailerName": "Demo Retailer",
  "retailerPin": "482615",
  "storeId": "store-01",
  "storeName": "Магазин Центр",
  "basketLocation": "Зал 1 / Полка 4",
  "productName": "Яблоко",
  "productCode": "APL-001",
  "totalWeight": 3250.0,
  "unitWeight": 250.0,
  "fullWeight": 5000.0,
  "quantity": 13,
  "fullQuantity": 20,
  "removedQuantity": 7,
  "fillPercent": 65.0,
  "temperature": 24.6,
  "humidity": 48.0,
  "centerX": 0.01,
  "centerY": -0.02,
  "tiltX": 0.1,
  "tiltY": -0.2,
  "mode": "pieces"
}
```

## MQTT

Базовый топик:

```text
smartbasket/state
```

Топик конкретной корзины:

```text
smartbasket/state/<basketId>
```

Формат MQTT client id:

```text
smartbasket-<basketId>
```

## Сборка и загрузка

### Прошивка

Сборка и загрузка прошивки:

```bash
pio run -t upload
```

Загрузка веб-файлов в `LittleFS`:

```bash
pio run -t uploadfs
```

После загрузки:

- SSID точки доступа: `SmartBasket_AP`
- пароль точки доступа: `12345678`
- локальный адрес: `http://192.168.4.1`

### Python-сервер

Установка:

```bash
cd server
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
```

Запуск:

```bash
python app.py
```

## Страницы сервера

- мониторинг: `http://localhost:8000/`
- админка: `http://localhost:8000/admin`
- документация API: `http://localhost:8000/docs`

## API сервера

### Администрирование

- `POST /api/admin/retailers`
- `GET /api/admin/retailers`
- `POST /api/admin/stores`
- `GET /api/admin/stores`

### Регистрация

- `GET /api/registration/retailer-by-pin?pin=<PIN>`
- `POST /api/registration/register-basket`

### Данные корзин

- `POST /api/baskets/ingest`
- `GET /api/baskets`
- `GET /api/baskets/{basketId}`
- `GET /api/baskets/{basketId}/history`
- `GET /health`

## Демо-данные

При первом запуске сервер создает:

- ритейлора: `Demo Retailer`
- PIN: `482615`
- магазины:
  - `store-01`
  - `store-02`

## Примечания

- OLED сейчас настроен под режим `SSD1306`
- если ваш дисплей на самом деле `SH1106`, драйвер в прошивке нужно будет переключить
