#pragma once

#include "../models/product.h"
#include <vector>

class ProductRepository {
public:
    ProductRepository();
    
    void addProduct(const Product& product);
    std::vector<Product> getProducts();

private:
    void createTable();
};
