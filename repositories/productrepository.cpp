#include "productrepository.h"
#include "../plugin/DatabasePlugin.h"

#include <stdexcept>

drogon::orm::DbClientPtr ProductRepository::getClient()
{
    auto plugin = drogon::app().getPlugin<DatabasePlugin>();

    if (!plugin)
        throw std::runtime_error("DatabasePlugin is not available");

    auto client = plugin->getClient();

    if (!client)
        throw std::runtime_error("Database client is not available");

    return client;
}

void ProductRepository::ensureSchema()
{
    auto db = getClient();

    db->execSqlSync(
        "CREATE TABLE IF NOT EXISTS users ("
        "id SERIAL PRIMARY KEY,"
        "name VARCHAR(100) NOT NULL,"
        "email VARCHAR(255) UNIQUE NOT NULL,"
        "password_hash TEXT NOT NULL,"
        "role VARCHAR(10) NOT NULL CHECK "
        "(role IN ('BUYER','SELLER','ADMIN')),"
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ")");

    db->execSqlSync(
        "CREATE TABLE IF NOT EXISTS products ("
        "id SERIAL PRIMARY KEY,"
        "seller_id INTEGER REFERENCES users(id),"
        "name VARCHAR(255) NOT NULL,"
        "description TEXT,"
        "price_cents BIGINT NOT NULL,"
        "stock_qty INTEGER NOT NULL DEFAULT 0,"
        "category VARCHAR(100),"
        "image_url TEXT,"
        "created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ")");

    db->execSqlSync(
        "ALTER TABLE products "
        "ADD COLUMN IF NOT EXISTS seller_id INTEGER");

    db->execSqlSync(
        "ALTER TABLE products "
        "ADD COLUMN IF NOT EXISTS description TEXT");

    db->execSqlSync(
        "ALTER TABLE products "
        "ADD COLUMN IF NOT EXISTS category VARCHAR(100)");

    db->execSqlSync(
        "ALTER TABLE products "
        "ADD COLUMN IF NOT EXISTS image_url TEXT");

    db->execSqlSync(
        "ALTER TABLE products "
        "ADD COLUMN IF NOT EXISTS created_at "
        "TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP");
}

void ProductRepository::addProduct(const Product& product)
{
    ensureSchema();

    auto db = getClient();

    db->execSqlSync(
        "INSERT INTO products "
        "(id, seller_id, name, description, price_cents, "
        "stock_qty, category, image_url) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8) "
        "ON CONFLICT (id) DO UPDATE SET "
        "seller_id = EXCLUDED.seller_id, "
        "name = EXCLUDED.name, "
        "description = EXCLUDED.description, "
        "price_cents = EXCLUDED.price_cents, "
        "stock_qty = EXCLUDED.stock_qty, "
        "category = EXCLUDED.category, "
        "image_url = EXCLUDED.image_url",
        product.id,
        product.seller_id,
        product.name,
        product.description,
        product.price.cents,
        product.stock_qty,
        product.category,
        product.image_url);
}

std::vector<Product> ProductRepository::getProducts()
{
    ensureSchema();

    auto db = getClient();

    auto result = db->execSqlSync(
        "SELECT id, seller_id, name, description, "
        "price_cents, stock_qty, category, image_url "
        "FROM products "
        "ORDER BY id");

    std::vector<Product> products;

    for (const auto& row : result)
    {
        Product p;

        p.id = row["id"].as<int>();

        if (row["seller_id"].isNull())
            p.seller_id = 0;
        else
            p.seller_id = row["seller_id"].as<int>();

        p.name = row["name"].as<std::string>();

        if (row["description"].isNull())
            p.description = "";
        else
            p.description = row["description"].as<std::string>();

        p.price.cents =
            row["price_cents"].as<long long>();

        p.stock_qty =
            row["stock_qty"].as<int>();

        if (row["category"].isNull())
            p.category = "";
        else
            p.category = row["category"].as<std::string>();

        if (row["image_url"].isNull())
            p.image_url = "";
        else
            p.image_url = row["image_url"].as<std::string>();

        products.push_back(p);
    }

    return products;
}

bool ProductRepository::updateProduct(const Product& product)
{
    ensureSchema();

    auto db = getClient();

    auto result = db->execSqlSync(
        "UPDATE products SET "
        "seller_id = $1, "
        "name = $2, "
        "description = $3, "
        "price_cents = $4, "
        "stock_qty = $5, "
        "category = $6, "
        "image_url = $7 "
        "WHERE id = $8",
        product.seller_id,
        product.name,
        product.description,
        product.price.cents,
        product.stock_qty,
        product.category,
        product.image_url,
        product.id);

    return result.affectedRows() > 0;
}

bool ProductRepository::deleteProduct(int id)
{
    ensureSchema();

    auto db = getClient();

    auto result = db->execSqlSync(
        "DELETE FROM products WHERE id = $1",
        id);

    return result.affectedRows() > 0;
}

bool ProductRepository::isDatabaseHealthy()
{
    try
    {
        auto db = getClient();

        db->execSqlSync("SELECT 1");

        return true;
    }
    catch (...)
    {
        return false;
    }
}
