#pragma once

#include <drogon/drogon.h>
#include <string>
#include <vector>
#include "../models/product.h"

struct CartItem
{
    int product_id = 0;
    std::string name;
    std::string description;
    std::string category;
    std::string image_url;
    long long price_cents = 0;
    int quantity = 0;
    int stock_qty = 0;
};

enum class CartOperationResult
{
    Success,
    InvalidQuantity,
    ProductNotFound,
    InsufficientStock,
    CartItemNotFound
};

class ProductRepository
{
public:
    ProductRepository() = default;

    int addProduct(const Product& product);
    std::vector<Product> getProducts();
    bool updateProductForSeller(const Product& product);
    bool deleteProductForSeller(int id, int sellerId);

    std::vector<CartItem> getCartItems(int userId);
    CartOperationResult addCartItem(int userId, int productId, int quantity);
    CartOperationResult updateCartItemQuantity(int userId, int productId, int quantity);
    bool removeCartItem(int userId, int productId);

    bool isDatabaseHealthy();

private:
    drogon::orm::DbClientPtr getClient();
};
