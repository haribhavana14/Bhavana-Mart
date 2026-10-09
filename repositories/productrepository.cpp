#include "productrepository.h"
#include "../plugin/DatabasePlugin.h"

#include <stdexcept>
#include <limits>
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

std::vector<CartItem> ProductRepository::getCartItems(int userId)
{
    auto db = getClient();
    auto result = db->execSqlSync(
        "SELECT p.id AS product_id, p.name, p.description, "
        "p.category, p.image_url, p.price_cents, p.stock_qty, c.quantity "
        "FROM cart_items c JOIN products p ON p.id = c.product_id "
        "WHERE c.user_id = $1 ORDER BY c.id",
        userId);

    std::vector<CartItem> items;
    for (const auto& row : result)
    {
        CartItem item;
        item.product_id = row["product_id"].as<int>();
        item.name = row["name"].as<std::string>();
        item.description = row["description"].as<std::string>();
        item.category = row["category"].as<std::string>();
        item.image_url = row["image_url"].as<std::string>();
        item.price_cents = row["price_cents"].as<long long>();
        item.stock_qty = row["stock_qty"].as<int>();
        item.quantity = row["quantity"].as<int>();
        items.push_back(item);
    }
    return items;
}

CartOperationResult ProductRepository::addCartItem(
    int userId, int productId, int quantity)
{
    if (userId <= 0 || productId <= 0 || quantity <= 0)
        return CartOperationResult::InvalidQuantity;

    auto db = getClient();
    auto transaction = db->newTransaction();

    auto products = transaction->execSqlSync(
        "SELECT stock_qty FROM products WHERE id = $1 FOR UPDATE",
        productId);

    if (products.empty())
        return CartOperationResult::ProductNotFound;

    int stock = products[0]["stock_qty"].as<int>();

    auto cart = transaction->execSqlSync(
        "SELECT quantity FROM cart_items "
        "WHERE user_id = $1 AND product_id = $2 FOR UPDATE",
        userId, productId);

    int current = cart.empty() ? 0 : cart[0]["quantity"].as<int>();

    if (quantity > stock - current)
        return CartOperationResult::InsufficientStock;

    if (cart.empty())
    {
        transaction->execSqlSync(
            "INSERT INTO cart_items (user_id, product_id, quantity) "
            "VALUES ($1, $2, $3)",
            userId, productId, quantity);
    }
    else
    {
        transaction->execSqlSync(
            "UPDATE cart_items SET quantity = $1 "
            "WHERE user_id = $2 AND product_id = $3",
            current + quantity, userId, productId);
    }

    return CartOperationResult::Success;
}

CartOperationResult ProductRepository::updateCartItemQuantity(
    int userId, int productId, int quantity)
{
    if (userId <= 0 || productId <= 0 || quantity <= 0)
        return CartOperationResult::InvalidQuantity;

    auto db = getClient();
    auto transaction = db->newTransaction();

    auto products = transaction->execSqlSync(
        "SELECT stock_qty FROM products WHERE id = $1 FOR UPDATE",
        productId);

    if (products.empty())
        return CartOperationResult::ProductNotFound;

    auto cart = transaction->execSqlSync(
        "SELECT quantity FROM cart_items "
        "WHERE user_id = $1 AND product_id = $2 FOR UPDATE",
        userId, productId);

    if (cart.empty())
        return CartOperationResult::CartItemNotFound;

    int stock = products[0]["stock_qty"].as<int>();
    if (quantity > stock)
        return CartOperationResult::InsufficientStock;

    transaction->execSqlSync(
        "UPDATE cart_items SET quantity = $1 "
        "WHERE user_id = $2 AND product_id = $3",
        quantity, userId, productId);

    return CartOperationResult::Success;
}

bool ProductRepository::removeCartItem(int userId, int productId)
{
    auto db = getClient();
    auto result = db->execSqlSync(
        "DELETE FROM cart_items WHERE user_id = $1 AND product_id = $2",
        userId, productId);

    return result.affectedRows() > 0;
}


// F5: Create an order from the buyer's cart using mock payment.
CheckoutResult ProductRepository::checkoutCart(int userId)
{
    if (userId <= 0)
        return {CheckoutStatus::CartEmpty, 0, 0};

    auto db = getClient();
    auto transaction = db->newTransaction();

    auto rows = transaction->execSqlSync(
        "SELECT c.product_id, c.quantity, "
        "p.price_cents, p.stock_qty "
        "FROM cart_items c "
        "JOIN products p ON p.id = c.product_id "
        "WHERE c.user_id = $1 "
        "ORDER BY p.id FOR UPDATE OF c, p",
        userId);

    if (rows.empty())
        return {CheckoutStatus::CartEmpty, 0, 0};

    long long totalCents = 0;

    // Validate every item and calculate total before writing the order.
    for (const auto& row : rows)
    {
        const int quantity = row["quantity"].as<int>();
        const int stock = row["stock_qty"].as<int>();
        const long long price = row["price_cents"].as<long long>();

        if (quantity <= 0 || quantity > stock)
            return {CheckoutStatus::InsufficientStock, 0, 0};

        if (price < 0 ||
            price > (std::numeric_limits<long long>::max() - totalCents)
                        / quantity)
            throw std::runtime_error("Order total is out of range");

        totalCents += price * quantity;
    }

    // Mock payment succeeds immediately; no real payment is collected.
    auto orderRows = transaction->execSqlSync(
        "INSERT INTO orders (buyer_id, status, total_amount_cents) "
        "VALUES ($1, $2, $3) RETURNING id",
        userId,
        std::string("CONFIRMED"),
        totalCents);

    const int orderId = orderRows[0]["id"].as<int>();

    for (const auto& row : rows)
    {
        const int productId = row["product_id"].as<int>();
        const int quantity = row["quantity"].as<int>();
        const long long price = row["price_cents"].as<long long>();

        transaction->execSqlSync(
            "INSERT INTO order_items "
            "(order_id, product_id, quantity, unit_price_cents) "
            "VALUES ($1, $2, $3, $4)",
            orderId, productId, quantity, price);

        transaction->execSqlSync(
            "UPDATE products SET stock_qty = stock_qty - $1 "
            "WHERE id = $2 AND stock_qty >= $1",
            quantity, productId);
    }

    transaction->execSqlSync(
        "DELETE FROM cart_items WHERE user_id = $1",
        userId);

    return {CheckoutStatus::Success, orderId, totalCents};
}


// F6: Buyer order history, including products in each order.
std::vector<BuyerOrderSummary> ProductRepository::getBuyerOrders(
    int buyerId)
{
    auto db = getClient();

    auto orders = db->execSqlSync(
        "SELECT id, status, total_amount_cents, created_at "
        "FROM orders WHERE buyer_id = $1 "
        "ORDER BY created_at DESC, id DESC",
        buyerId);

    std::vector<BuyerOrderSummary> result;

    for (const auto& row : orders)
    {
        BuyerOrderSummary order;
        order.id = row["id"].as<int>();
        order.status = row["status"].as<std::string>();
        order.created_at = row["created_at"].as<std::string>();
        order.total_cents = row["total_amount_cents"].as<long long>();

        auto items = db->execSqlSync(
            "SELECT oi.product_id, p.seller_id, "
            "p.name AS product_name, oi.quantity, "
            "oi.unit_price_cents "
            "FROM order_items oi "
            "JOIN products p ON p.id = oi.product_id "
            "WHERE oi.order_id = $1 ORDER BY oi.id",
            order.id);

        for (const auto& itemRow : items)
        {
            OrderItemSummary item;
            item.product_id = itemRow["product_id"].as<int>();
            item.seller_id = itemRow["seller_id"].as<int>();
            item.product_name =
                itemRow["product_name"].as<std::string>();
            item.quantity = itemRow["quantity"].as<int>();
            item.unit_price_cents =
                itemRow["unit_price_cents"].as<long long>();

            order.items.push_back(item);
        }

        result.push_back(order);
    }

    return result;
}

// F6: Seller sees orders containing that seller's products.
std::vector<SellerOrderLine> ProductRepository::getSellerOrders(
    int sellerId)
{
    auto db = getClient();

    auto rows = db->execSqlSync(
        "SELECT o.id AS order_id, o.buyer_id, o.status, "
        "o.created_at, o.total_amount_cents, "
        "oi.product_id, p.name AS product_name, "
        "oi.quantity, oi.unit_price_cents "
        "FROM orders o "
        "JOIN order_items oi ON oi.order_id = o.id "
        "JOIN products p ON p.id = oi.product_id "
        "WHERE p.seller_id = $1 "
        "ORDER BY o.created_at DESC, o.id DESC, oi.id",
        sellerId);

    std::vector<SellerOrderLine> result;

    for (const auto& row : rows)
    {
        SellerOrderLine item;
        item.order_id = row["order_id"].as<int>();
        item.buyer_id = row["buyer_id"].as<int>();
        item.status = row["status"].as<std::string>();
        item.created_at = row["created_at"].as<std::string>();
        item.total_cents =
            row["total_amount_cents"].as<long long>();
        item.product_id = row["product_id"].as<int>();
        item.product_name = row["product_name"].as<std::string>();
        item.quantity = row["quantity"].as<int>();
        item.unit_price_cents =
            row["unit_price_cents"].as<long long>();

        result.push_back(item);
    }

    return result;
}


// F7: Admin sees users without exposing password hashes.
std::vector<AdminUserSummary> ProductRepository::getAdminUsers()
{
    auto db = getClient();
    auto rows = db->execSqlSync(
        "SELECT id, name, email, role, created_at::text AS created_at "
        "FROM users ORDER BY id");

    std::vector<AdminUserSummary> result;

    for (const auto& row : rows)
    {
        AdminUserSummary user;
        user.id = row["id"].as<int>();
        user.name = row["name"].as<std::string>();
        user.email = row["email"].as<std::string>();
        user.role = row["role"].as<std::string>();
        user.created_at = row["created_at"].as<std::string>();
        result.push_back(user);
    }

    return result;
}

// F7: Admin sees all orders and buyer details.
std::vector<AdminOrderSummary> ProductRepository::getAdminOrders()
{
    auto db = getClient();
    auto rows = db->execSqlSync(
        "SELECT o.id, o.buyer_id, u.name AS buyer_name, "
        "u.email AS buyer_email, o.status, "
        "o.created_at::text AS created_at, "
        "o.total_amount_cents, COUNT(oi.id) AS item_count "
        "FROM orders o "
        "JOIN users u ON u.id = o.buyer_id "
        "LEFT JOIN order_items oi ON oi.order_id = o.id "
        "GROUP BY o.id, u.name, u.email "
        "ORDER BY o.created_at DESC, o.id DESC");

    std::vector<AdminOrderSummary> result;

    for (const auto& row : rows)
    {
        AdminOrderSummary order;
        order.id = row["id"].as<int>();
        order.buyer_id = row["buyer_id"].as<int>();
        order.buyer_name = row["buyer_name"].as<std::string>();
        order.buyer_email = row["buyer_email"].as<std::string>();
        order.status = row["status"].as<std::string>();
        order.created_at = row["created_at"].as<std::string>();
        order.total_cents = row["total_amount_cents"].as<long long>();
        order.item_count = row["item_count"].as<long long>();
        result.push_back(order);
    }

    return result;
}

// F7: Remove a listing only when it is not referenced by order history.
AdminDeleteProductResult ProductRepository::removeProductAsAdmin(
    int productId)
{
    if (productId <= 0)
        return AdminDeleteProductResult::NotFound;

    auto db = getClient();

    auto references = db->execSqlSync(
        "SELECT COUNT(*) AS ref_count FROM order_items "
        "WHERE product_id = $1",
        productId);

    if (references[0]["ref_count"].as<long long>() > 0)
        return AdminDeleteProductResult::ProductHasOrders;

    auto deleted = db->execSqlSync(
        "DELETE FROM products WHERE id = $1",
        productId);

    return deleted.affectedRows() > 0
        ? AdminDeleteProductResult::Success
        : AdminDeleteProductResult::NotFound;
}


// F8: Get product ratings and reviews.
ProductReviewList ProductRepository::getProductReviews(int productId)
{
    ProductReviewList result;

    if (productId <= 0)
        return result;

    auto db = getClient();

    auto product = db->execSqlSync(
        "SELECT id FROM products WHERE id = $1",
        productId);

    if (product.empty())
        return result;

    result.product_exists = true;

    auto aggregate = db->execSqlSync(
        "SELECT COALESCE(AVG(rating), 0)::float8 AS average_rating, "
        "COUNT(*) AS review_count FROM reviews WHERE product_id = $1",
        productId);

    result.average_rating =
        aggregate[0]["average_rating"].as<double>();
    result.review_count =
        aggregate[0]["review_count"].as<int>();

    auto rows = db->execSqlSync(
        "SELECT r.id, r.product_id, r.user_id, u.name AS reviewer_name, "
        "r.rating, r.comment, r.created_at::text AS created_at "
        "FROM reviews r JOIN users u ON u.id = r.user_id "
        "WHERE r.product_id = $1 "
        "ORDER BY r.created_at DESC, r.id DESC",
        productId);

    for (const auto& row : rows)
    {
        ProductReview review;
        review.id = row["id"].as<int>();
        review.product_id = row["product_id"].as<int>();
        review.user_id = row["user_id"].as<int>();
        review.reviewer_name = row["reviewer_name"].as<std::string>();
        review.rating = row["rating"].as<int>();
        review.comment = row["comment"].as<std::string>();
        review.created_at = row["created_at"].as<std::string>();
        result.reviews.push_back(review);
    }

    return result;
}

// F8: Only buyers with a delivered order for the product may review it.
ReviewSubmissionStatus ProductRepository::addProductReview(
    int userId, int productId, int rating, const std::string& comment)
{
    if (userId <= 0 || productId <= 0 ||
        rating < 1 || rating > 5 || comment.size() > 1000)
        return ReviewSubmissionStatus::InvalidData;

    auto db = getClient();
    auto transaction = db->newTransaction();

    auto product = transaction->execSqlSync(
        "SELECT id FROM products WHERE id = $1",
        productId);

    if (product.empty())
        return ReviewSubmissionStatus::ProductNotFound;

    auto completedOrder = transaction->execSqlSync(
        "SELECT 1 FROM orders o "
        "JOIN order_items oi ON oi.order_id = o.id "
        "WHERE o.buyer_id = $1 AND oi.product_id = $2 "
        "AND o.status = $3 LIMIT 1",
        userId, productId, std::string("DELIVERED"));

    if (completedOrder.empty())
        return ReviewSubmissionStatus::OrderNotCompleted;

    auto existing = transaction->execSqlSync(
        "SELECT id FROM reviews WHERE user_id = $1 "
        "AND product_id = $2 LIMIT 1",
        userId, productId);

    if (!existing.empty())
        return ReviewSubmissionStatus::AlreadyReviewed;

    transaction->execSqlSync(
        "INSERT INTO reviews (product_id, user_id, rating, comment) "
        "VALUES ($1, $2, $3, $4)",
        productId, userId, rating, comment);

    return ReviewSubmissionStatus::Success;
}
