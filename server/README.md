# SmartBasket Server

Серверная часть для корзин SmartBasket на Flask.

Сервер принимает телеметрию от ESP32-корзин, хранит текущее состояние и историю событий, позволяет создавать ритейлоров и магазины, а также отдаёт две HTML-страницы:

- мониторинг корзин
- админку для управления ритейлорами и магазинами

## Технологии

- `Python`
- `Flask`
- `SQLite`
- HTML-страницы для мониторинга и администрирования

## Возможности

- приём данных корзин по HTTP
- хранение последнего состояния корзины
- хранение истории событий корзины
- создание ритейлоров
- генерация PIN-кодов ритейлоров
- создание магазинов для ритейлоров
- регистрация корзины в магазине по PIN
- мониторинг всех корзин через HTML-страницу
- админ-панель без Swagger

## Установка и запуск

```bash
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python app.py
```

Сервер стартует на:

- мониторинг: `http://localhost:8000/`
- админка: `http://localhost:8000/admin`
- health-check: `http://localhost:8000/health`

## Демо-данные

При первом запуске сервер автоматически создаёт демо-ритейлора:

- название: `Demo Retailer`
- PIN: `482615`

Это позволяет сразу протестировать регистрацию корзины из веб-интерфейса ESP32.

## Регистрация корзины

1. Откройте веб-интерфейс корзины на ESP32.
2. Укажите адрес сервера регистрации, например:

```text
http://192.168.1.50:8000
```

3. Введите PIN ритейлора.
4. Загрузите список магазинов.
5. Выберите магазин.
6. Укажите `basketId`, товар и расположение корзины.
7. Зарегистрируйте корзину.

После регистрации `HTTP endpoint` должен быть таким:

```text
http://192.168.1.50:8000/api/baskets/ingest
```

## API

### Проверка сервера

- `GET /health`

### Мониторинг

- `GET /api/baskets`
- `GET /api/baskets/<basket_id>`
- `GET /api/baskets/<basket_id>/history`

### Регистрация

- `GET /api/registration/retailer-by-pin?pin=<PIN>`
- `POST /api/registration/register-basket`

Пример регистрации корзины:

```json
{
  "pinCode": "482615",
  "storeId": "store-01",
  "basketId": "basket-01",
  "basketLocation": "Зал 1 / Полка 4",
  "productName": "Яблоко",
  "productCode": "APL-001"
}
```

### Администрирование

- `GET /api/admin/retailers`
- `POST /api/admin/retailers`
- `GET /api/admin/stores`
- `POST /api/admin/stores`

Пример создания ритейлора:

```json
{
  "name": "Retail Group 1"
}
```

Пример создания магазина:

```json
{
  "retailerId": "retailer-1234",
  "name": "Магазин Юг",
  "location": "Краснодар, ул. Пример",
  "code": "SOUTH-01"
}
```

### Телеметрия корзины

- `POST /api/baskets/ingest`

Пример полезной нагрузки:

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

## Хранение данных

Данные сохраняются в:

- `server/data/smartbasket.db`

Таблицы SQLite:

- `retailers`
- `stores`
- `basket_registry`
- `basket_state`
- `basket_events`
