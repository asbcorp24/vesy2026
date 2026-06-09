# SmartBasket

SmartBasket is an ESP32-based smart basket for retail and warehouse inventory control.

The basket measures total weight with 4 load cells through 4 HX711 modules, calculates how many items remain based on the weight of one product, registers itself to a retailer and store, and sends structured data to a server.

## Features

- 4-corner weighing with `HX711 x4`
- total weight measurement
- item count calculation from `unitWeight`
- remaining quantity, removed quantity, and fill percent calculation
- basket registration in the structure `retailer -> store -> basket`
- local web UI on ESP32
- data sending over `HTTP`, `MQTT`, and `ESP-NOW`
- settings storage in `NVS`
- OLED display via `U8g2`
- calibration from local menu and web UI
- Python server with monitoring and admin pages

## Project Structure

- [src](C:/Users/user/Documents/vesy/src) - ESP32 firmware
- [include](C:/Users/user/Documents/vesy/include) - headers and configuration
- [data](C:/Users/user/Documents/vesy/data) - ESP32 web UI for `LittleFS`
- [server](C:/Users/user/Documents/vesy/server) - Python backend, monitor, and admin UI
- [platformio.ini](C:/Users/user/Documents/vesy/platformio.ini) - PlatformIO build config

## Technology Stack

### Firmware

- `ESP32 DevKit`
- `Arduino framework`
- `PlatformIO`
- `ArduinoJson`
- `U8g2`
- `PubSubClient`
- `HX711`
- `DHT sensor library`
- `Preferences` / `NVS`
- `WiFi`, `WebServer`, `HTTPClient`, `ESP-NOW`
- `LittleFS`

### Server

- `Python`
- `FastAPI`
- `Uvicorn`
- `SQLite`
- plain HTML admin and monitoring pages

## Hardware

- `ESP32 DevKit`
- `4 x HX711`
- `4 x load cells up to 10 kg`
- `OLED 0.96" I2C`
- `DHT11`
- `2 buttons`

## Pinout

The following pins are defined in firmware in [include/Config.h](C:/Users/user/Documents/vesy/include/Config.h).

### HX711 connections

| Corner | HX711 DOUT | HX711 SCK |
| --- | --- | --- |
| 1 | GPIO `32` | GPIO `4` |
| 2 | GPIO `33` | GPIO `16` |
| 3 | GPIO `25` | GPIO `17` |
| 4 | GPIO `26` | GPIO `5` |

### Other pins

| Device | Pin |
| --- | --- |
| DHT11 data | GPIO `27` |
| Menu button | GPIO `18` |
| Select button | GPIO `19` |

### OLED

- Display type in firmware: `U8G2_SSD1306_128X64_NONAME_F_HW_I2C`
- I2C address: `0x3C`
- `USE_SH1106 = false` by default

Note:
- The firmware does not call `Wire.begin(...)` with custom pins.
- On a typical ESP32 DevKit this usually means default I2C pins:
  - `SDA -> GPIO 21`
  - `SCL -> GPIO 22`

If your board uses different I2C wiring, add explicit `Wire.begin(SDA, SCL)` later in firmware.

## Current Device Logic

The basket stores:

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

The basket calculates:

- `totalWeight`
- `quantity`
- `fullQuantity`
- `removedQuantity`
- `fillPercent`

## Registration Flow

The intended workflow is:

1. Start the Python server.
2. Create a retailer in the admin page.
3. Get the generated retailer PIN.
4. Create one or more stores for that retailer.
5. Open the basket web UI on ESP32.
6. Enter the registration server URL.
7. Enter the retailer PIN.
8. Load available stores.
9. Select a store.
10. Register the basket with its `basketId`, product, and physical location.

After registration, the basket sends telemetry already linked to the correct retailer and store.

## Data Sent By Basket

The basket sends structured JSON like:

```json
{
  "basketId": "basket-01",
  "retailerId": "retailer-01",
  "retailerName": "Demo Retailer",
  "retailerPin": "482615",
  "storeId": "store-01",
  "storeName": "Store Center",
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

## MQTT

Base topic:

```text
smartbasket/state
```

Per-basket topic:

```text
smartbasket/state/<basketId>
```

MQTT client ID format:

```text
smartbasket-<basketId>
```

## Build And Flash

### Firmware

Build and upload firmware:

```bash
pio run -t upload
```

Upload web files to `LittleFS`:

```bash
pio run -t uploadfs
```

After boot:

- AP SSID: `SmartBasket_AP`
- AP password: `12345678`
- local address: `http://192.168.4.1`

### Python Server

Install:

```bash
cd server
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
```

Run:

```bash
uvicorn app:app --host 0.0.0.0 --port 8000
```

## Server Pages

- monitor: `http://localhost:8000/`
- admin: `http://localhost:8000/admin`
- API docs: `http://localhost:8000/docs`

## Server API

### Admin

- `POST /api/admin/retailers`
- `GET /api/admin/retailers`
- `POST /api/admin/stores`
- `GET /api/admin/stores`

### Registration

- `GET /api/registration/retailer-by-pin?pin=<PIN>`
- `POST /api/registration/register-basket`

### Basket Data

- `POST /api/baskets/ingest`
- `GET /api/baskets`
- `GET /api/baskets/{basketId}`
- `GET /api/baskets/{basketId}/history`
- `GET /health`

## Demo Data

On first server start, demo data is created automatically:

- retailer: `Demo Retailer`
- PIN: `482615`
- stores:
  - `store-01`
  - `store-02`

## Notes

- `MPU-6050` is not integrated into firmware yet.
- OLED is currently configured for `SSD1306`-compatible mode in code.
- If your OLED is actually `SH1106`, switch the display driver in firmware.
