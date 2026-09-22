#pragma once

#include <drogon/drogon.h>
#include <vector>

#include "../models/product.h"

class ProductRepository
{
public:
    ProductRepository() = default;

    void addProduct(const Product& product);
    std::vector<Product> getProducts();

    bool updateProduct(const Product& product);
    bool deleteProduct(int id);

    bool isDatabaseHealthy();

private:
    drogon::orm::DbClientPtr getClient();
    void ensureSchema();
};