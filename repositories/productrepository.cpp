#include "productrepository.h"
#include "../plugin/DatabasePlugin.h"

#include <cmath>
#include <iostream>

drogon::orm::DbClientPtr ProductRepository::getClient()
{
    auto *plugin = drogon::app().getPlugin<DatabasePlugin>();

    if (!plugin)
    {
        throw std::runtime_error("DatabasePlugin is not available");
    }

    return plugin->getClient();
}

void ProductRepository::ensureSchema()
{
    auto db = getClient();

    db->execSqlSync(
        "CREATE TABLE IF NOT EXISTS users ("
        "id SERIAL PRIMARY KEY,"
        "name TEXT NOT NULL,"
        "email TEXT UNIQUE NOT NULL,"
        "password_hash TEXT NOT NULL,"
        "role TEXT NOT NULL CHECK(role IN ('BUYER','SELLER','ADMIN')),"
        "created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP"
        ");");

    db->execSqlSync(
        "CREATE TABLE IF NOT EXISTS products ("
        "id INTEGER PRIMARY KEY,"
        "seller_id INTEGER REFERENCES users(id) ON DELETE SET NULL,"
        "name TEXT NOT NULL,"
        "description TEXT DEFAULT '',"
        "price_cents BIGINT NOT NULL,"
        "stock_qty INTEGER NOT NULL,"
        "category TEXT DEFAULT '',"
        "image_url TEXT DEFAULT '',"
        "created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP"
        ");");
}

void ProductRepository::addProduct(const Product& product)
{
    ensureSchema();

    auto db = getClient();

    long long priceCents =
        static_cast<long long>(std::llround(product.price * 100.0));

    db->execSqlSync(
        "INSERT INTO products "
        "(id, name, price_cents, stock_qty) "
        "VALUES ($1, $2, $3, $4) "
        "ON CONFLICT (id) DO UPDATE SET "
        "name = EXCLUDED.name, "
        "price_cents = EXCLUDED.price_cents, "
        "stock_qty = EXCLUDED.stock_qty;",
        product.id,
        product.name,
        priceCents,
        product.stock);
}

std::vector<Product> ProductRepository::getProducts()
{
    ensureSchema();

    auto db = getClient();

    // Demo seed - inserted only when IDs don't already exist.
    db->execSqlSync(
        "INSERT INTO products (id, name, price_cents, stock_qty) "
        "VALUES "
        "(1, 'Laptop', 5700000, 10),"
        "(2, 'Mouse', 80000, 25),"
        "(3, 'Keyboard', 120000, 15),"
        "(4, 'Headphones', 150000, 20),"
        "(5, 'Monitor', 800000, 10) "
        "ON CONFLICT (id) DO NOTHING;");

    std::vector<Product> products;

    auto result = db->execSqlSync(
        "SELECT id, name, price_cents, stock_qty "
        "FROM products ORDER BY id;");

    for (auto row : result)
    {
        Product p;

        p.id = row["id"].as<int>();
        p.name = row["name"].as<std::string>();

        long long cents = row["price_cents"].as<long long>();
        p.price = cents / 100.0;

        p.stock = row["stock_qty"].as<int>();

        products.push_back(p);
    }

    return products;
}

bool ProductRepository::updateProduct(const Product& product)
{
    ensureSchema();

    auto db = getClient();

    long long priceCents =
        static_cast<long long>(std::llround(product.price * 100.0));

    auto result = db->execSqlSync(
        "UPDATE products "
        "SET name = $1, price_cents = $2, stock_qty = $3 "
        "WHERE id = $4;",
        product.name,
        priceCents,
        product.stock,
        product.id);

    return result.affectedRows() > 0;
}

bool ProductRepository::deleteProduct(int id)
{
    ensureSchema();

    auto db = getClient();

    auto result = db->execSqlSync(
        "DELETE FROM products WHERE id = $1;",
        id);

    return result.affectedRows() > 0;
}

bool ProductRepository::isDatabaseHealthy()
{
    try
    {
        auto db = getClient();
        auto result = db->execSqlSync("SELECT 1;");
        return result.size() == 1;
    }
    catch (...)
    {
        return false;
    }
}