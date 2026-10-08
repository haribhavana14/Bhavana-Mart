#pragma once

#include <drogon/drogon.h>
#include <vector>
#include "../models/product.h"

class ProductRepository
{
public:
    ProductRepository() = default;

    int addProduct(const Product& product);
    std::vector<Product> getProducts();

    bool updateProductForSeller(const Product& product);
    bool deleteProductForSeller(int id, int sellerId);

    bool isDatabaseHealthy();

private:
    drogon::orm::DbClientPtr getClient();
};
