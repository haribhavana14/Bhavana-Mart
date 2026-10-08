#include "productrepository.h"
#include "../plugin/DatabasePlugin.h"

#include <stdexcept>
#include <string>
#include <vector>

drogon::orm::DbClientPtr ProductRepository::getClient()
{
    auto plugin = drogon::app().getPlugin<DatabasePlugin>();

    if (!plugin)
        throw std::runtime_error(
            "DatabasePlugin is not available");

    auto client = plugin->getClient();

    if (!client)
        throw std::runtime_error(
            "Database client is not available");

    return client;
}

int ProductRepository::addProduct(const Product& product)
{
    auto db = getClient();

    auto result = db->execSqlSync(
        "INSERT INTO products "
        "(seller_id, name, description, price_cents, "
        "stock_qty, category, image_url) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7) "
        "RETURNING id",
        product.seller_id,
        product.name,
        product.description,
        product.price.cents,
        product.stock_qty,
        product.category,
        product.image_url);

    return result[0]["id"].as<int>();
}

std::vector<Product> ProductRepository::getProducts()
{
    auto db = getClient();

    auto result = db->execSqlSync(
        "SELECT id, seller_id, name, description, "
        "price_cents, stock_qty, category, image_url "
        "FROM products "
        "ORDER BY id");

    std::vector<Product> products;

    for (const auto& row : result)
    {
        Product product;

        product.id = row["id"].as<int>();

        if (row["seller_id"].isNull())
            product.seller_id = 0;
        else
            product.seller_id =
                row["seller_id"].as<int>();

        product.name =
            row["name"].as<std::string>();

        if (row["description"].isNull())
            product.description = "";
        else
            product.description =
                row["description"].as<std::string>();

        product.price.cents =
            row["price_cents"].as<long long>();

        product.stock_qty =
            row["stock_qty"].as<int>();

        if (row["category"].isNull())
            product.category = "";
        else
            product.category =
                row["category"].as<std::string>();

        if (row["image_url"].isNull())
            product.image_url = "";
        else
            product.image_url =
                row["image_url"].as<std::string>();

        products.push_back(product);
    }

    return products;
}

bool ProductRepository::updateProductForSeller(
    const Product& product)
{
    auto db = getClient();

    auto result = db->execSqlSync(
        "UPDATE products SET "
        "name = $1, "
        "description = $2, "
        "price_cents = $3, "
        "stock_qty = $4, "
        "category = $5, "
        "image_url = $6 "
        "WHERE id = $7 AND seller_id = $8",
        product.name,
        product.description,
        product.price.cents,
        product.stock_qty,
        product.category,
        product.image_url,
        product.id,
        product.seller_id);

    return result.affectedRows() > 0;
}

bool ProductRepository::deleteProductForSeller(
    int id,
    int sellerId)
{
    auto db = getClient();

    auto result = db->execSqlSync(
        "DELETE FROM products "
        "WHERE id = $1 AND seller_id = $2",
        id,
        sellerId);

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
