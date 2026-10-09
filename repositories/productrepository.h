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

enum class CheckoutStatus
{
    Success,
    CartEmpty,
    InsufficientStock
};

struct CheckoutResult
{
    CheckoutStatus status{CheckoutStatus::Success};
    int order_id{0};
    long long total_cents{0};
};


struct OrderItemSummary
{
    int product_id = 0;
    int seller_id = 0;
    std::string product_name;
    int quantity = 0;
    long long unit_price_cents = 0;
};

struct BuyerOrderSummary
{
    int id = 0;
    std::string status;
    std::string created_at;
    long long total_cents = 0;
    std::vector<OrderItemSummary> items;
};

struct SellerOrderLine
{
    int order_id = 0;
    int buyer_id = 0;
    std::string status;
    std::string created_at;
    long long total_cents = 0;
    int product_id = 0;
    std::string product_name;
    int quantity = 0;
    long long unit_price_cents = 0;
};

struct AdminUserSummary
{
    int id = 0;
    std::string name;
    std::string email;
    std::string role;
    std::string created_at;
};

struct AdminOrderSummary
{
    int id = 0;
    int buyer_id = 0;
    std::string buyer_name;
    std::string buyer_email;
    std::string status;
    std::string created_at;
    long long total_cents = 0;
    long long item_count = 0;
};

enum class AdminDeleteProductResult
{
    Success,
    NotFound,
    ProductHasOrders
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
    CheckoutResult checkoutCart(int userId);
    std::vector<BuyerOrderSummary> getBuyerOrders(int buyerId);
    std::vector<SellerOrderLine> getSellerOrders(int sellerId);
    std::vector<AdminUserSummary> getAdminUsers();
    std::vector<AdminOrderSummary> getAdminOrders();
    AdminDeleteProductResult removeProductAsAdmin(int productId);

    bool isDatabaseHealthy();

private:
    drogon::orm::DbClientPtr getClient();
};
