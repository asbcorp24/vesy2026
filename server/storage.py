from __future__ import annotations

import json
import random
import sqlite3
import string
from pathlib import Path
from typing import Any


class BasketStorage:
    def __init__(self, db_path: Path) -> None:
        self.db_path = db_path
        self.db_path.parent.mkdir(parents=True, exist_ok=True)
        self._init_db()
        self._migrate_db()
        self._ensure_demo_data()

    def _connect(self) -> sqlite3.Connection:
        conn = sqlite3.connect(self.db_path)
        conn.row_factory = sqlite3.Row
        return conn

    def _init_db(self) -> None:
        with self._connect() as conn:
            conn.executescript(
                """
                CREATE TABLE IF NOT EXISTS retailers (
                    retailer_id TEXT PRIMARY KEY,
                    name TEXT NOT NULL,
                    pin_code TEXT NOT NULL UNIQUE,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );

                CREATE TABLE IF NOT EXISTS stores (
                    store_id TEXT PRIMARY KEY,
                    retailer_id TEXT NOT NULL,
                    name TEXT NOT NULL,
                    location TEXT,
                    code TEXT,
                    pin_code TEXT UNIQUE,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    FOREIGN KEY (retailer_id) REFERENCES retailers (retailer_id)
                );

                CREATE TABLE IF NOT EXISTS basket_registry (
                    basket_id TEXT PRIMARY KEY,
                    retailer_id TEXT NOT NULL,
                    retailer_name TEXT,
                    store_id TEXT NOT NULL,
                    store_name TEXT,
                    basket_location TEXT,
                    product_name TEXT,
                    product_code TEXT,
                    registered_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );

                CREATE TABLE IF NOT EXISTS basket_state (
                    basket_id TEXT PRIMARY KEY,
                    retailer_id TEXT,
                    retailer_name TEXT,
                    store_id TEXT,
                    store_name TEXT,
                    basket_location TEXT,
                    product_name TEXT,
                    product_code TEXT,
                    total_weight REAL,
                    unit_weight REAL,
                    full_weight REAL,
                    quantity INTEGER,
                    full_quantity INTEGER,
                    removed_quantity INTEGER,
                    fill_percent REAL,
                    temperature REAL,
                    humidity REAL,
                    center_x REAL,
                    center_y REAL,
                    tilt_x REAL,
                    tilt_y REAL,
                    mode TEXT,
                    last_payload TEXT NOT NULL,
                    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );

                CREATE TABLE IF NOT EXISTS basket_events (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    basket_id TEXT NOT NULL,
                    payload TEXT NOT NULL,
                    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
                );
                """
            )

    def _migrate_db(self) -> None:
        with self._connect() as conn:
            columns = {
                row["name"]
                for row in conn.execute("PRAGMA table_info(stores)").fetchall()
            }
            if "pin_code" not in columns:
                conn.execute("ALTER TABLE stores ADD COLUMN pin_code TEXT")
            conn.execute("CREATE UNIQUE INDEX IF NOT EXISTS idx_stores_pin_code ON stores(pin_code)")

            rows = conn.execute("SELECT store_id, pin_code FROM stores").fetchall()
            for row in rows:
                if row["pin_code"]:
                    continue
                conn.execute(
                    "UPDATE stores SET pin_code = ? WHERE store_id = ?",
                    (self._generate_unique_store_pin(conn), row["store_id"]),
                )

    def _ensure_demo_data(self) -> None:
        with self._connect() as conn:
            existing = conn.execute("SELECT COUNT(*) AS cnt FROM retailers").fetchone()["cnt"]
            if existing:
                return

            retailer_id = "retailer-01"
            retailer_pin = "482615"
            conn.execute(
                "INSERT INTO retailers (retailer_id, name, pin_code) VALUES (?, ?, ?)",
                (retailer_id, "Demo Retailer", retailer_pin),
            )
            stores = [
                ("store-01", retailer_id, "Магазин Центр", "Москва, Тверская", "CTR-01", "310101"),
                ("store-02", retailer_id, "Магазин Север", "Москва, Складочная", "NTH-02", "310102"),
            ]
            conn.executemany(
                "INSERT INTO stores (store_id, retailer_id, name, location, code, pin_code) VALUES (?, ?, ?, ?, ?, ?)",
                stores,
            )

    def _generate_pin(self) -> str:
        return "".join(random.choice(string.digits) for _ in range(6))

    def _generate_unique_store_pin(self, conn: sqlite3.Connection) -> str:
        while True:
            pin_code = self._generate_pin()
            exists = conn.execute(
                "SELECT 1 FROM stores WHERE pin_code = ?",
                (pin_code,),
            ).fetchone()
            if exists is None:
                return pin_code

    def create_retailer(self, name: str) -> dict[str, Any]:
        retailer_id = f"retailer-{random.randint(1000, 9999)}"
        pin_code = self._generate_pin()
        with self._connect() as conn:
            conn.execute(
                "INSERT INTO retailers (retailer_id, name, pin_code) VALUES (?, ?, ?)",
                (retailer_id, name, pin_code),
            )
        return {"retailerId": retailer_id, "name": name, "pinCode": pin_code}

    def list_retailers(self) -> list[dict[str, Any]]:
        with self._connect() as conn:
            rows = conn.execute(
                """
                SELECT retailer_id, name, pin_code, created_at
                FROM retailers
                ORDER BY created_at DESC, name ASC
                """
            ).fetchall()
        return [
            {
                "retailerId": row["retailer_id"],
                "name": row["name"],
                "pinCode": row["pin_code"],
                "createdAt": row["created_at"],
            }
            for row in rows
        ]

    def create_store(self, retailer_id: str, name: str, location: str = "", code: str = "") -> dict[str, Any]:
        store_id = f"store-{random.randint(1000, 9999)}"
        with self._connect() as conn:
            retailer = conn.execute(
                "SELECT retailer_id, name FROM retailers WHERE retailer_id = ?",
                (retailer_id,),
            ).fetchone()
            if retailer is None:
                raise ValueError("Retailer not found")

            pin_code = self._generate_unique_store_pin(conn)
            conn.execute(
                "INSERT INTO stores (store_id, retailer_id, name, location, code, pin_code) VALUES (?, ?, ?, ?, ?, ?)",
                (store_id, retailer_id, name, location, code, pin_code),
            )
        return {
            "storeId": store_id,
            "retailerId": retailer_id,
            "storeName": name,
            "location": location,
            "code": code,
            "pinCode": pin_code,
        }

    def list_stores(self, retailer_id: str | None = None) -> list[dict[str, Any]]:
        with self._connect() as conn:
            if retailer_id:
                rows = conn.execute(
                    """
                    SELECT s.store_id, s.retailer_id, r.name AS retailer_name, s.name, s.location, s.code, s.pin_code, s.created_at
                    FROM stores s
                    JOIN retailers r ON r.retailer_id = s.retailer_id
                    WHERE s.retailer_id = ?
                    ORDER BY s.created_at DESC, s.name ASC
                    """,
                    (retailer_id,),
                ).fetchall()
            else:
                rows = conn.execute(
                    """
                    SELECT s.store_id, s.retailer_id, r.name AS retailer_name, s.name, s.location, s.code, s.pin_code, s.created_at
                    FROM stores s
                    JOIN retailers r ON r.retailer_id = s.retailer_id
                    ORDER BY s.created_at DESC, s.name ASC
                    """
                ).fetchall()
        return [
            {
                "storeId": row["store_id"],
                "retailerId": row["retailer_id"],
                "retailerName": row["retailer_name"],
                "storeName": row["name"],
                "location": row["location"] or "",
                "code": row["code"] or "",
                "pinCode": row["pin_code"] or "",
                "createdAt": row["created_at"],
            }
            for row in rows
        ]

    def get_retailer_by_pin(self, pin_code: str) -> dict[str, Any] | None:
        with self._connect() as conn:
            retailer = conn.execute(
                "SELECT retailer_id, name, pin_code FROM retailers WHERE pin_code = ?",
                (pin_code,),
            ).fetchone()
            if retailer is None:
                return None
            stores = conn.execute(
                """
                SELECT store_id, name, location, code, pin_code
                FROM stores
                WHERE retailer_id = ?
                ORDER BY name
                """,
                (retailer["retailer_id"],),
            ).fetchall()

        return {
            "retailerId": retailer["retailer_id"],
            "retailerName": retailer["name"],
            "retailerPin": retailer["pin_code"],
            "stores": [
                {
                    "storeId": row["store_id"],
                    "storeName": row["name"],
                    "location": row["location"] or "",
                    "code": row["code"] or "",
                    "pinCode": row["pin_code"] or "",
                }
                for row in stores
            ],
        }

    def get_store_by_pin(self, pin_code: str) -> dict[str, Any] | None:
        with self._connect() as conn:
            row = conn.execute(
                """
                SELECT
                    s.store_id,
                    s.name AS store_name,
                    s.location,
                    s.code,
                    s.pin_code,
                    r.retailer_id,
                    r.name AS retailer_name,
                    r.pin_code AS retailer_pin
                FROM stores s
                JOIN retailers r ON r.retailer_id = s.retailer_id
                WHERE s.pin_code = ?
                """,
                (pin_code,),
            ).fetchone()

        if row is None:
            return None

        return {
            "storeId": row["store_id"],
            "storeName": row["store_name"],
            "location": row["location"] or "",
            "code": row["code"] or "",
            "storePin": row["pin_code"] or "",
            "retailerId": row["retailer_id"],
            "retailerName": row["retailer_name"],
            "retailerPin": row["retailer_pin"] or "",
        }

    def register_basket(
        self,
        retailer_id: str,
        retailer_name: str,
        store_id: str,
        store_name: str,
        basket_id: str,
        basket_location: str,
        product_name: str,
        product_code: str,
    ) -> dict[str, Any]:
        with self._connect() as conn:
            store = conn.execute(
                "SELECT pin_code FROM stores WHERE store_id = ?",
                (store_id,),
            ).fetchone()
            store_pin = store["pin_code"] if store else ""

            conn.execute(
                """
                INSERT INTO basket_registry (
                    basket_id,
                    retailer_id,
                    retailer_name,
                    store_id,
                    store_name,
                    basket_location,
                    product_name,
                    product_code,
                    registered_at,
                    updated_at
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP, CURRENT_TIMESTAMP)
                ON CONFLICT(basket_id) DO UPDATE SET
                    retailer_id = excluded.retailer_id,
                    retailer_name = excluded.retailer_name,
                    store_id = excluded.store_id,
                    store_name = excluded.store_name,
                    basket_location = excluded.basket_location,
                    product_name = excluded.product_name,
                    product_code = excluded.product_code,
                    updated_at = CURRENT_TIMESTAMP
                """,
                (
                    basket_id,
                    retailer_id,
                    retailer_name,
                    store_id,
                    store_name,
                    basket_location,
                    product_name,
                    product_code,
                ),
            )

        return {
            "basketId": basket_id,
            "retailerId": retailer_id,
            "retailerName": retailer_name,
            "storeId": store_id,
            "storeName": store_name,
            "storePin": store_pin,
            "basketLocation": basket_location,
            "productName": product_name,
            "productCode": product_code,
        }

    def save_payload(self, payload: dict[str, Any]) -> None:
        basket_id = payload.get("basketId") or "unknown"
        body = json.dumps(payload, ensure_ascii=False)

        with self._connect() as conn:
            registry = conn.execute(
                """
                SELECT retailer_id, retailer_name, store_id, store_name, basket_location, product_name, product_code
                FROM basket_registry
                WHERE basket_id = ?
                """,
                (basket_id,),
            ).fetchone()

            retailer_id = payload.get("retailerId") or (registry["retailer_id"] if registry else None)
            retailer_name = payload.get("retailerName") or (registry["retailer_name"] if registry else None)
            store_id = payload.get("storeId") or (registry["store_id"] if registry else None)
            store_name = payload.get("storeName") or (registry["store_name"] if registry else None)
            basket_location = payload.get("basketLocation") or (registry["basket_location"] if registry else None)
            product_name = payload.get("productName") or (registry["product_name"] if registry else None)
            product_code = payload.get("productCode") or (registry["product_code"] if registry else None)

            conn.execute(
                """
                INSERT INTO basket_state (
                    basket_id,
                    retailer_id,
                    retailer_name,
                    store_id,
                    store_name,
                    basket_location,
                    product_name,
                    product_code,
                    total_weight,
                    unit_weight,
                    full_weight,
                    quantity,
                    full_quantity,
                    removed_quantity,
                    fill_percent,
                    temperature,
                    humidity,
                    center_x,
                    center_y,
                    tilt_x,
                    tilt_y,
                    mode,
                    last_payload,
                    updated_at
                ) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP)
                ON CONFLICT(basket_id) DO UPDATE SET
                    retailer_id = excluded.retailer_id,
                    retailer_name = excluded.retailer_name,
                    store_id = excluded.store_id,
                    store_name = excluded.store_name,
                    basket_location = excluded.basket_location,
                    product_name = excluded.product_name,
                    product_code = excluded.product_code,
                    total_weight = excluded.total_weight,
                    unit_weight = excluded.unit_weight,
                    full_weight = excluded.full_weight,
                    quantity = excluded.quantity,
                    full_quantity = excluded.full_quantity,
                    removed_quantity = excluded.removed_quantity,
                    fill_percent = excluded.fill_percent,
                    temperature = excluded.temperature,
                    humidity = excluded.humidity,
                    center_x = excluded.center_x,
                    center_y = excluded.center_y,
                    tilt_x = excluded.tilt_x,
                    tilt_y = excluded.tilt_y,
                    mode = excluded.mode,
                    last_payload = excluded.last_payload,
                    updated_at = CURRENT_TIMESTAMP
                """,
                (
                    basket_id,
                    retailer_id,
                    retailer_name,
                    store_id,
                    store_name,
                    basket_location,
                    product_name,
                    product_code,
                    payload.get("totalWeight"),
                    payload.get("unitWeight"),
                    payload.get("fullWeight"),
                    payload.get("quantity"),
                    payload.get("fullQuantity"),
                    payload.get("removedQuantity"),
                    payload.get("fillPercent"),
                    payload.get("temperature"),
                    payload.get("humidity"),
                    payload.get("centerX"),
                    payload.get("centerY"),
                    payload.get("tiltX"),
                    payload.get("tiltY"),
                    payload.get("mode"),
                    body,
                ),
            )
            conn.execute(
                "INSERT INTO basket_events (basket_id, payload) VALUES (?, ?)",
                (basket_id, body),
            )

    def list_baskets(self) -> list[dict[str, Any]]:
        with self._connect() as conn:
            rows = conn.execute(
                """
                SELECT
                    basket_id,
                    retailer_id,
                    retailer_name,
                    store_id,
                    store_name,
                    basket_location,
                    product_name,
                    product_code,
                    total_weight,
                    quantity,
                    full_quantity,
                    removed_quantity,
                    fill_percent,
                    updated_at
                FROM basket_state
                ORDER BY updated_at DESC
                """
            ).fetchall()
        return [dict(row) for row in rows]

    def get_basket(self, basket_id: str) -> dict[str, Any] | None:
        with self._connect() as conn:
            row = conn.execute(
                "SELECT last_payload, updated_at FROM basket_state WHERE basket_id = ?",
                (basket_id,),
            ).fetchone()
        if row is None:
            return None
        payload = json.loads(row["last_payload"])
        payload["updatedAt"] = row["updated_at"]
        return payload

    def get_history(self, basket_id: str, limit: int = 100) -> list[dict[str, Any]]:
        with self._connect() as conn:
            rows = conn.execute(
                """
                SELECT payload, created_at
                FROM basket_events
                WHERE basket_id = ?
                ORDER BY id DESC
                LIMIT ?
                """,
                (basket_id, limit),
            ).fetchall()

        result: list[dict[str, Any]] = []
        for row in rows:
            payload = json.loads(row["payload"])
            payload["receivedAt"] = row["created_at"]
            result.append(payload)
        return result
