from __future__ import annotations

from pathlib import Path
from typing import Any

from flask import Flask, jsonify, request, Response

from storage import BasketStorage


BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"
DB_PATH = DATA_DIR / "smartbasket.db"
DASHBOARD_PATH = BASE_DIR / "dashboard.html"
ADMIN_PATH = BASE_DIR / "admin.html"

storage = BasketStorage(DB_PATH)
app = Flask(__name__)
app.config["JSON_AS_ASCII"] = False


@app.after_request
def add_cors_headers(response: Response) -> Response:
    response.headers["Access-Control-Allow-Origin"] = "*"
    response.headers["Access-Control-Allow-Headers"] = "Content-Type, Authorization"
    response.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS"
    return response


def json_error(message: str, status_code: int = 400) -> tuple[Response, int]:
    return jsonify({"error": message}), status_code


def get_json_body() -> dict[str, Any]:
    payload = request.get_json(silent=True)
    if not isinstance(payload, dict):
        raise ValueError("Invalid JSON body")
    return payload


def as_clean_str(payload: dict[str, Any], key: str, default: str = "", max_length: int | None = None) -> str:
    value = payload.get(key, default)
    if value is None:
        value = default
    value = str(value).strip()
    if max_length is not None:
        value = value[:max_length]
    return value


def as_float(payload: dict[str, Any], key: str, default: float = 0.0) -> float:
    value = payload.get(key, default)
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def as_int(payload: dict[str, Any], key: str, default: int = 0) -> int:
    value = payload.get(key, default)
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def validate_required(value: str, field_name: str, min_length: int = 1) -> str:
    if len(value) < min_length:
        raise ValueError(f"{field_name} is required")
    return value


@app.get("/")
def dashboard() -> str:
    return DASHBOARD_PATH.read_text(encoding="utf-8")


@app.get("/admin")
def admin_page() -> str:
    return ADMIN_PATH.read_text(encoding="utf-8")


@app.get("/health")
def health() -> Response:
    return jsonify({"status": "ok"})


@app.post("/api/admin/retailers")
def create_retailer() -> tuple[Response, int] | Response:
    try:
        payload = get_json_body()
        name = validate_required(as_clean_str(payload, "name", max_length=64), "name")
        return jsonify(storage.create_retailer(name))
    except ValueError as exc:
        return json_error(str(exc), 400)


@app.get("/api/admin/retailers")
def list_retailers() -> Response:
    return jsonify({"items": storage.list_retailers()})


@app.post("/api/admin/stores")
def create_store() -> tuple[Response, int] | Response:
    try:
        payload = get_json_body()
        retailer_id = validate_required(as_clean_str(payload, "retailerId", max_length=24), "retailerId")
        name = validate_required(as_clean_str(payload, "name", max_length=64), "name")
        location = as_clean_str(payload, "location", max_length=96)
        code = as_clean_str(payload, "code", max_length=24)
        return jsonify(storage.create_store(retailer_id, name, location, code))
    except ValueError as exc:
        status = 404 if str(exc) == "Retailer not found" else 400
        return json_error(str(exc), status)


@app.get("/api/admin/stores")
def list_stores() -> Response:
    retailer_id = request.args.get("retailerId", default=None, type=str)
    return jsonify({"items": storage.list_stores(retailer_id)})


@app.get("/api/registration/retailer-by-pin")
def retailer_by_pin() -> tuple[Response, int] | Response:
    pin = request.args.get("pin", default="", type=str).strip()
    retailer = storage.get_retailer_by_pin(pin)
    if retailer is None:
        return json_error("Retailer PIN not found", 404)
    return jsonify(retailer)


@app.get("/api/registration/store-by-pin")
def store_by_pin() -> tuple[Response, int] | Response:
    pin = request.args.get("pin", default="", type=str).strip()
    store = storage.get_store_by_pin(pin)
    if store is None:
        return json_error("Store PIN not found", 404)
    return jsonify(store)


@app.post("/api/registration/register-basket")
def register_basket() -> tuple[Response, int] | Response:
    try:
        payload = get_json_body()
        pin_code = validate_required(as_clean_str(payload, "pinCode", max_length=16), "pinCode", min_length=4)
        store_id = validate_required(as_clean_str(payload, "storeId", max_length=24), "storeId")
        basket_id = validate_required(as_clean_str(payload, "basketId", max_length=24), "basketId")
        basket_location = as_clean_str(payload, "basketLocation", max_length=48)
        product_name = as_clean_str(payload, "productName", max_length=48)
        product_code = as_clean_str(payload, "productCode", max_length=32)

        store = storage.get_store_by_pin(pin_code)
        if store is None:
            retailer = storage.get_retailer_by_pin(pin_code)
            if retailer is None:
                return json_error("Store PIN not found", 404)
            store = next((item for item in retailer["stores"] if item["storeId"] == store_id), None)
            if store is None:
                return json_error("Store not found for retailer", 404)
            retailer_id = retailer["retailerId"]
            retailer_name = retailer["retailerName"]
        else:
            if store["storeId"] != store_id:
                return json_error("Store PIN does not match selected store", 404)
            retailer_id = store["retailerId"]
            retailer_name = store["retailerName"]

        result = storage.register_basket(
            retailer_id=retailer_id,
            retailer_name=retailer_name,
            store_id=store["storeId"],
            store_name=store["storeName"],
            basket_id=basket_id,
            basket_location=basket_location,
            product_name=product_name,
            product_code=product_code,
        )
        return jsonify(result)
    except ValueError as exc:
        return json_error(str(exc), 400)


@app.post("/api/baskets/ingest")
def ingest_basket() -> tuple[Response, int] | Response:
    try:
        payload = get_json_body()
        basket_id = validate_required(as_clean_str(payload, "basketId", max_length=24), "basketId")

        body: dict[str, Any] = {
            "basketId": basket_id,
            "retailerId": as_clean_str(payload, "retailerId", max_length=24),
            "retailerName": as_clean_str(payload, "retailerName", max_length=48),
            "retailerPin": as_clean_str(payload, "retailerPin", max_length=16),
            "storeId": as_clean_str(payload, "storeId", max_length=24),
            "storeName": as_clean_str(payload, "storeName", max_length=48),
            "basketLocation": as_clean_str(payload, "basketLocation", max_length=48),
            "productName": as_clean_str(payload, "productName", max_length=48),
            "productCode": as_clean_str(payload, "productCode", max_length=32),
            "totalWeight": as_float(payload, "totalWeight"),
            "unitWeight": as_float(payload, "unitWeight"),
            "fullWeight": as_float(payload, "fullWeight"),
            "quantity": as_int(payload, "quantity"),
            "fullQuantity": as_int(payload, "fullQuantity"),
            "removedQuantity": as_int(payload, "removedQuantity"),
            "fillPercent": as_float(payload, "fillPercent"),
            "temperature": payload.get("temperature"),
            "humidity": payload.get("humidity"),
            "centerX": payload.get("centerX"),
            "centerY": payload.get("centerY"),
            "tiltX": payload.get("tiltX"),
            "tiltY": payload.get("tiltY"),
            "mode": as_clean_str(payload, "mode", default="pieces", max_length=16) or "pieces",
        }

        storage.save_payload(body)
        return jsonify({
            "ok": True,
            "basketId": body["basketId"],
            "quantity": body["quantity"],
            "totalWeight": body["totalWeight"],
        })
    except ValueError as exc:
        return json_error(str(exc), 400)


@app.get("/api/baskets")
def list_baskets() -> Response:
    return jsonify({"items": storage.list_baskets()})


@app.get("/api/baskets/<basket_id>")
def get_basket(basket_id: str) -> tuple[Response, int] | Response:
    basket = storage.get_basket(basket_id)
    if basket is None:
        return json_error("Basket not found", 404)
    return jsonify(basket)


@app.get("/api/baskets/<basket_id>/history")
def get_basket_history(basket_id: str) -> Response:
    limit = request.args.get("limit", default=100, type=int)
    limit = max(1, min(limit, 500))
    return jsonify({"items": storage.get_history(basket_id, limit=limit)})


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=8000, debug=True)
