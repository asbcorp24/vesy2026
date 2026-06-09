from __future__ import annotations

from pathlib import Path
from typing import Any, Literal

from fastapi import FastAPI, HTTPException
from fastapi.responses import HTMLResponse
from pydantic import BaseModel, Field

from storage import BasketStorage


class BasketPayload(BaseModel):
    basketId: str = Field(min_length=1, max_length=24)
    retailerId: str | None = Field(default="", max_length=24)
    retailerName: str | None = Field(default="", max_length=48)
    retailerPin: str | None = Field(default="", max_length=16)
    storeId: str | None = Field(default="", max_length=24)
    storeName: str | None = Field(default="", max_length=48)
    basketLocation: str | None = Field(default="", max_length=48)
    productName: str | None = Field(default="", max_length=48)
    productCode: str | None = Field(default="", max_length=32)
    totalWeight: float = 0.0
    unitWeight: float = 0.0
    fullWeight: float = 0.0
    quantity: int = 0
    fullQuantity: int = 0
    removedQuantity: int = 0
    fillPercent: float = 0.0
    temperature: float | None = None
    humidity: float | None = None
    centerX: float | None = None
    centerY: float | None = None
    tiltX: float | None = None
    tiltY: float | None = None
    mode: Literal["mass", "pieces"] = "pieces"


class RetailerCreatePayload(BaseModel):
    name: str = Field(min_length=1, max_length=64)


class StoreCreatePayload(BaseModel):
    retailerId: str = Field(min_length=1, max_length=24)
    name: str = Field(min_length=1, max_length=64)
    location: str = Field(default="", max_length=96)
    code: str = Field(default="", max_length=24)


class BasketRegistrationPayload(BaseModel):
    pinCode: str = Field(min_length=4, max_length=16)
    storeId: str = Field(min_length=1, max_length=24)
    basketId: str = Field(min_length=1, max_length=24)
    basketLocation: str = Field(default="", max_length=48)
    productName: str = Field(default="", max_length=48)
    productCode: str = Field(default="", max_length=32)


BASE_DIR = Path(__file__).resolve().parent
DATA_DIR = BASE_DIR / "data"
DB_PATH = DATA_DIR / "smartbasket.db"
DASHBOARD_PATH = BASE_DIR / "dashboard.html"
ADMIN_PATH = BASE_DIR / "admin.html"

storage = BasketStorage(DB_PATH)
app = FastAPI(title="SmartBasket Server", version="2.0.0")


@app.get("/", response_class=HTMLResponse)
def dashboard() -> str:
    return DASHBOARD_PATH.read_text(encoding="utf-8")


@app.get("/admin", response_class=HTMLResponse)
def admin_page() -> str:
    return ADMIN_PATH.read_text(encoding="utf-8")


@app.get("/health")
def health() -> dict[str, str]:
    return {"status": "ok"}


@app.post("/api/admin/retailers")
def create_retailer(payload: RetailerCreatePayload) -> dict[str, Any]:
    return storage.create_retailer(payload.name)


@app.get("/api/admin/retailers")
def list_retailers() -> dict[str, Any]:
    return {"items": storage.list_retailers()}


@app.post("/api/admin/stores")
def create_store(payload: StoreCreatePayload) -> dict[str, Any]:
    try:
        return storage.create_store(payload.retailerId, payload.name, payload.location, payload.code)
    except ValueError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc


@app.get("/api/admin/stores")
def list_stores(retailerId: str | None = None) -> dict[str, Any]:
    return {"items": storage.list_stores(retailerId)}


@app.get("/api/registration/retailer-by-pin")
def retailer_by_pin(pin: str) -> dict[str, Any]:
    retailer = storage.get_retailer_by_pin(pin)
    if retailer is None:
        raise HTTPException(status_code=404, detail="Retailer PIN not found")
    return retailer


@app.post("/api/registration/register-basket")
def register_basket(payload: BasketRegistrationPayload) -> dict[str, Any]:
    retailer = storage.get_retailer_by_pin(payload.pinCode)
    if retailer is None:
        raise HTTPException(status_code=404, detail="Retailer PIN not found")

    store = next((item for item in retailer["stores"] if item["storeId"] == payload.storeId), None)
    if store is None:
        raise HTTPException(status_code=404, detail="Store not found for retailer")

    return storage.register_basket(
        retailer_id=retailer["retailerId"],
        retailer_name=retailer["retailerName"],
        store_id=store["storeId"],
        store_name=store["storeName"],
        basket_id=payload.basketId,
        basket_location=payload.basketLocation,
        product_name=payload.productName,
        product_code=payload.productCode,
    )


@app.post("/api/baskets/ingest")
def ingest_basket(payload: BasketPayload) -> dict[str, Any]:
    body = payload.model_dump()
    storage.save_payload(body)
    return {
        "ok": True,
        "basketId": payload.basketId,
        "quantity": payload.quantity,
        "totalWeight": payload.totalWeight,
    }


@app.get("/api/baskets")
def list_baskets() -> dict[str, Any]:
    return {"items": storage.list_baskets()}


@app.get("/api/baskets/{basket_id}")
def get_basket(basket_id: str) -> dict[str, Any]:
    basket = storage.get_basket(basket_id)
    if basket is None:
        raise HTTPException(status_code=404, detail="Basket not found")
    return basket


@app.get("/api/baskets/{basket_id}/history")
def get_basket_history(basket_id: str, limit: int = 100) -> dict[str, Any]:
    if limit < 1:
        limit = 1
    if limit > 500:
        limit = 500
    return {"items": storage.get_history(basket_id, limit=limit)}
