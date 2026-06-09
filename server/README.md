# SmartBasket Server

Backend for SmartBasket baskets.

This server receives telemetry from baskets, stores the latest state and event history, manages retailers and stores, and provides two web pages:

- monitoring page
- admin page

## Technology

- `Python`
- `FastAPI`
- `Uvicorn`
- `SQLite`
- plain HTML pages for monitoring and admin

## Features

- receive basket data over HTTP
- store latest basket state
- store basket event history
- create retailers
- generate retailer PIN codes
- create stores for retailers
- register baskets to a retailer and store by PIN
- show all baskets in a monitoring page
- manage retailers and stores without Swagger

## Run

Create virtual environment and install dependencies:

```bash
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
```

Start server:

```bash
uvicorn app:app --host 0.0.0.0 --port 8000
```

## Pages

- monitor: `http://localhost:8000/`
- admin: `http://localhost:8000/admin`
- Swagger: `http://localhost:8000/docs`

## Demo Data

On first start the server creates demo data automatically.

Retailer:

- name: `Demo Retailer`
- PIN: `482615`

Stores:

- `store-01` / `Магазин Центр`
- `store-02` / `Магазин Север`

This allows you to test basket registration immediately from the ESP32 web UI.

## Basket Registration Flow

1. Open the basket web UI on ESP32.
2. Enter registration server URL, for example:

```text
http://192.168.1.50:8000
```

3. Enter retailer PIN.
4. Load stores for that retailer.
5. Select a store.
6. Enter `basketId`, product, and basket location.
7. Register basket.

After registration, set or keep telemetry endpoint:

```text
http://192.168.1.50:8000/api/baskets/ingest
```

## API

### Health

- `GET /health`

### Monitoring

- `GET /api/baskets`
- `GET /api/baskets/{basket_id}`
- `GET /api/baskets/{basket_id}/history`

### Registration

- `GET /api/registration/retailer-by-pin?pin=<PIN>`
- `POST /api/registration/register-basket`

Example request:

```json
{
  "pinCode": "482615",
  "storeId": "store-01",
  "basketId": "basket-01",
  "basketLocation": "Hall 1 / Shelf 4",
  "productName": "Apple",
  "productCode": "APL-001"
}
```

### Admin

- `GET /api/admin/retailers`
- `POST /api/admin/retailers`
- `GET /api/admin/stores`
- `POST /api/admin/stores`

Create retailer example:

```json
{
  "name": "Retail Group 1"
}
```

Create store example:

```json
{
  "retailerId": "retailer-1234",
  "name": "Store South",
  "location": "Krasnodar, Example street",
  "code": "SOUTH-01"
}
```

### Basket Telemetry

- `POST /api/baskets/ingest`

Example payload:

```json
{
  "basketId": "basket-01",
  "retailerId": "retailer-01",
  "retailerName": "Demo Retailer",
  "retailerPin": "482615",
  "storeId": "store-01",
  "storeName": "Магазин Центр",
  "basketLocation": "Hall 1 / Shelf 4",
  "productName": "Apple",
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

## Storage

Server stores data in:

- database file: `server/data/smartbasket.db`

SQLite tables:

- `retailers`
- `stores`
- `basket_registry`
- `basket_state`
- `basket_events`
