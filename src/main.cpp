#include <drogon/drogon.h>

#include "../repositories/productrepository.h"

#include "../plugin/DatabasePlugin.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

#include <functional>

#include <optional>

#include <string>
#include <stdexcept>

static std::optional<int> getUserId(

    const drogon::HttpRequestPtr& req)

{

    return req->session()->getOptional<int>("user_id");

}

static bool isSeller(

    const drogon::HttpRequestPtr& req)

{

    auto role =

        req->session()->getOptional<std::string>("role");

    return role.has_value() &&

           role.value() == "SELLER";

}

static bool isBuyer(const drogon::HttpRequestPtr& req)
{
    auto role = req->session()->getOptional<std::string>("role");
    return role.has_value() && role.value() == "BUYER";
}

static bool isAdmin(const drogon::HttpRequestPtr& req)
{
    auto role = req->session()->getOptional<std::string>("role");
    return role.has_value() && role.value() == "ADMIN";
}
static drogon::HttpResponsePtr errorResponse(

    drogon::HttpStatusCode status,

    const std::string& code)

{

    Json::Value body;

    body["success"] = false;

    body["data"] = Json::nullValue;

    body["error"]["code"] = code;

    auto response =

        drogon::HttpResponse::newHttpJsonResponse(body);

    response->setStatusCode(status);

    return response;

}

static drogon::HttpResponsePtr cartOperationError(CartOperationResult result)
{
    switch (result)
    {
        case CartOperationResult::InvalidQuantity:
            return errorResponse(drogon::k400BadRequest, "INVALID_QUANTITY");
        case CartOperationResult::ProductNotFound:
            return errorResponse(drogon::k404NotFound, "PRODUCT_NOT_FOUND");
        case CartOperationResult::InsufficientStock:
            return errorResponse(drogon::k409Conflict, "INSUFFICIENT_STOCK");
        case CartOperationResult::CartItemNotFound:
            return errorResponse(drogon::k404NotFound, "CART_ITEM_NOT_FOUND");
        case CartOperationResult::Success:
            break;
    }
    return errorResponse(drogon::k500InternalServerError, "CART_OPERATION_FAILED");
}

static std::string toLower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char ch) {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

int main()

{

    drogon::app().loadConfigFile("./config.json");

    drogon::app().enableSession(1200);

    ProductRepository repo;

    // =========================

    // BUYER BROWSE, SEARCH AND FILTER PRODUCTS

    // =========================

    auto getProductsHandler =

        [&repo](

            const drogon::HttpRequestPtr& request,

            std::function<void(

                const drogon::HttpResponsePtr&)>&& callback)

    {

        try

        {

            // F3: optional query parameters for buyer search and category filtering.
            std::string keyword = request->getParameter("keyword");
            if (keyword.empty())
                keyword = request->getParameter("q");

            std::string category = request->getParameter("category");
            keyword = toLower(keyword);
            category = toLower(category);

            auto products = repo.getProducts();
            Json::Value result(Json::arrayValue);

            for (const auto& p : products)
            {
                const std::string name = toLower(p.name);
                const std::string description = toLower(p.description);
                const std::string productCategory = toLower(p.category);

                if (!keyword.empty() &&
                    name.find(keyword) == std::string::npos &&
                    description.find(keyword) == std::string::npos &&
                    productCategory.find(keyword) == std::string::npos)
                {
                    continue;
                }

                if (!category.empty() && category != "all" &&
                    productCategory != category)
                {
                    continue;
                }

                Json::Value item;

                item["id"] = p.id;

                item["seller_id"] = p.seller_id;

                item["name"] = p.name;

                item["description"] = p.description;

                item["price"] =

                    static_cast<Json::Int64>(

                        p.price.cents);

                item["stock"] = p.stock_qty;

                item["category"] = p.category;

                item["image_url"] = p.image_url;

                result.append(item);

            }

            callback(

                drogon::HttpResponse::

                    newHttpJsonResponse(result));

        }

        catch (...)

        {

            callback(errorResponse(

                drogon::k500InternalServerError,

                "DATABASE_ERROR"));

        }

    };

    // =========================

    // SELLER CREATE PRODUCT

    // =========================

    auto postProductHandler =

        [&repo](

            const drogon::HttpRequestPtr& request,

            std::function<void(

                const drogon::HttpResponsePtr&)>&& callback)

    {

        if (!isSeller(request))

        {

            callback(errorResponse(

                drogon::k403Forbidden,

                "SELLER_ONLY"));

            return;

        }

        auto userId = getUserId(request);

        if (!userId.has_value())

        {

            callback(errorResponse(

                drogon::k401Unauthorized,

                "UNAUTHORIZED"));

            return;

        }

        auto json = request->getJsonObject();

        if (!json)

        {

            callback(errorResponse(

                drogon::k400BadRequest,

                "INVALID_JSON"));

            return;

        }

        Product product;

        // Seller ID comes from server-side session.

        product.seller_id = userId.value();

        product.name =

            (*json)["name"].asString();

        product.description =

            (*json)["description"].asString();

        const double price =

            (*json)["price"].asDouble();

        product.price.cents =

            static_cast<long long>(

                price * 100.0 + 0.5);

        product.stock_qty =

            (*json)["stock"].asInt();

        product.category =

            (*json)["category"].asString();

        product.image_url =

            (*json)["image_url"].asString();

        if (product.name.empty() ||

            product.price.cents < 0 ||

            product.stock_qty < 0)

        {

            callback(errorResponse(

                drogon::k400BadRequest,

                "INVALID_PRODUCT_DATA"));

            return;

        }

        try

        {

            const int productId =

                repo.addProduct(product);

            Json::Value data;

            data["id"] = productId;

            data["seller_id"] = product.seller_id;

            Json::Value body;

            body["success"] = true;

            body["data"] = data;

            body["error"] = Json::nullValue;

            auto response =

                drogon::HttpResponse::newHttpJsonResponse(

                    body);

            response->setStatusCode(

                drogon::k201Created);

            callback(response);

        }

        catch (...)

        {

            callback(errorResponse(

                drogon::k500InternalServerError,

                "DATABASE_ERROR"));

        }

    };

    // =========================

    // SELLER UPDATE PRODUCT

    // PUT /api/v1/products/{id}

    // =========================

    auto updateProductHandler =

        [&repo](

            const drogon::HttpRequestPtr& request,

            std::function<void(

                const drogon::HttpResponsePtr&)>&& callback, const std::string& productIdParam)

    {

        if (!isSeller(request))

        {

            callback(errorResponse(

                drogon::k403Forbidden,

                "SELLER_ONLY"));

            return;

        }

        auto userId = getUserId(request);

        if (!userId.has_value())

        {

            callback(errorResponse(

                drogon::k401Unauthorized,

                "UNAUTHORIZED"));

            return;

        }

        auto json = request->getJsonObject();

        if (!json)

        {

            callback(errorResponse(

                drogon::k400BadRequest,

                "INVALID_JSON"));

            return;

        }

        Product product;

        product.id =

            std::stoi(productIdParam);

        product.seller_id =

            userId.value();

        product.name =

            (*json)["name"].asString();

        product.description =

            (*json)["description"].asString();

        const double price =

            (*json)["price"].asDouble();

        product.price.cents =

            static_cast<long long>(

                price * 100.0 + 0.5);

        product.stock_qty =

            (*json)["stock"].asInt();

        product.category =

            (*json)["category"].asString();

        product.image_url =

            (*json)["image_url"].asString();

        if (product.id <= 0 ||

            product.name.empty() ||

            product.price.cents < 0 ||

            product.stock_qty < 0)

        {

            callback(errorResponse(

                drogon::k400BadRequest,

                "INVALID_PRODUCT_DATA"));

            return;

        }

        try

        {

            if (!repo.updateProductForSeller(product))

            {

                callback(errorResponse(

                    drogon::k404NotFound,

                    "PRODUCT_NOT_FOUND"));

                return;

            }

            Json::Value data;

            data["id"] = product.id;

            Json::Value body;

            body["success"] = true;

            body["data"] = data;

            body["error"] = Json::nullValue;

            callback(

                drogon::HttpResponse::

                    newHttpJsonResponse(body));

        }

        catch (...)

        {

            callback(errorResponse(

                drogon::k500InternalServerError,

                "DATABASE_ERROR"));

        }

    };

    // =========================

    // SELLER DELETE PRODUCT

    // DELETE /api/v1/products/{id}

    // =========================

    auto deleteProductHandler =

        [&repo](

            const drogon::HttpRequestPtr& request,

            std::function<void(

                const drogon::HttpResponsePtr&)>&& callback, const std::string& productIdParam)

    {

        if (!isSeller(request))

        {

            callback(errorResponse(

                drogon::k403Forbidden,

                "SELLER_ONLY"));

            return;

        }

        auto userId = getUserId(request);

        if (!userId.has_value())

        {

            callback(errorResponse(

                drogon::k401Unauthorized,

                "UNAUTHORIZED"));

            return;

        }

        try

        {

            const int productId =

                std::stoi(productIdParam);

            if (!repo.deleteProductForSeller(

                    productId,

                    userId.value()))

            {

                callback(errorResponse(

                    drogon::k404NotFound,

                    "PRODUCT_NOT_FOUND"));

                return;

            }

            Json::Value data;

            data["message"] =

                "Product deleted successfully";

            Json::Value body;

            body["success"] = true;

            body["data"] = data;

            body["error"] = Json::nullValue;

            callback(

                drogon::HttpResponse::

                    newHttpJsonResponse(body));

        }

        catch (...)

        {

            callback(errorResponse(

                drogon::k400BadRequest,

                "INVALID_PRODUCT_ID"));

        }

    };

    // =========================

    // SELLER MY PRODUCTS

    // =========================

    auto getSellerProductsHandler =

        [&repo](

            const drogon::HttpRequestPtr& request,

            std::function<void(

                const drogon::HttpResponsePtr&)>&& callback)

    {

        if (!isSeller(request))

        {

            callback(errorResponse(

                drogon::k403Forbidden,

                "SELLER_ONLY"));

            return;

        }

        auto userId = getUserId(request);

        if (!userId.has_value())

        {

            callback(errorResponse(

                drogon::k401Unauthorized,

                "UNAUTHORIZED"));

            return;

        }

        try

        {

            auto products = repo.getProducts();

            Json::Value result(Json::arrayValue);

            for (const auto& p : products)

            {

                if (p.seller_id != userId.value())

                    continue;

                Json::Value item;

                item["id"] = p.id;

                item["seller_id"] = p.seller_id;

                item["name"] = p.name;

                item["description"] = p.description;

                item["price"] =

                    static_cast<Json::Int64>(

                        p.price.cents);

                item["stock"] = p.stock_qty;

                item["category"] = p.category;

                item["image_url"] = p.image_url;

                result.append(item);

            }

            callback(

                drogon::HttpResponse::

                    newHttpJsonResponse(result));

        }

        catch (...)

        {

            callback(errorResponse(

                drogon::k500InternalServerError,

                "DATABASE_ERROR"));

        }

    };

    // =========================
    // F4: BUYER SHOPPING CART
    // =========================

    auto getCartHandler =
        [&repo](const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        if (!isBuyer(request))
        {
            callback(errorResponse(drogon::k401Unauthorized, "BUYER_LOGIN_REQUIRED"));
            return;
        }

        auto userId = getUserId(request);
        if (!userId.has_value())
        {
            callback(errorResponse(drogon::k401Unauthorized, "UNAUTHORIZED"));
            return;
        }

        try
        {
            const auto cartItems = repo.getCartItems(userId.value());
            Json::Value data;
            data["items"] = Json::Value(Json::arrayValue);
            Json::Int64 totalCents = 0;

            for (const auto& item : cartItems)
            {
                const Json::Int64 subtotal =
                    static_cast<Json::Int64>(item.price_cents) * item.quantity;
                Json::Value row;
                row["product_id"] = item.product_id;
                row["name"] = item.name;
                row["description"] = item.description;
                row["category"] = item.category;
                row["image_url"] = item.image_url;
                row["price"] = static_cast<Json::Int64>(item.price_cents);
                row["quantity"] = item.quantity;
                row["stock"] = item.stock_qty;
                row["subtotal"] = subtotal;
                data["items"].append(row);
                totalCents += subtotal;
            }

            data["total_cents"] = totalCents;
            data["currency"] = "INR";

            Json::Value body;
            body["success"] = true;
            body["data"] = data;
            body["error"] = Json::nullValue;
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        }
        catch (...)
        {
            callback(errorResponse(drogon::k500InternalServerError, "DATABASE_ERROR"));
        }
    };

    auto addCartItemHandler =
        [&repo](const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        if (!isBuyer(request))
        {
            callback(errorResponse(drogon::k401Unauthorized, "BUYER_LOGIN_REQUIRED"));
            return;
        }

        auto userId = getUserId(request);
        auto json = request->getJsonObject();
        if (!userId.has_value())
        {
            callback(errorResponse(drogon::k401Unauthorized, "UNAUTHORIZED"));
            return;
        }
        if (!json)
        {
            callback(errorResponse(drogon::k400BadRequest, "INVALID_JSON"));
            return;
        }

        const int productId = (*json)["product_id"].asInt();
        const int quantity = (*json).isMember("quantity")
                                 ? (*json)["quantity"].asInt()
                                 : 1;
        if (productId <= 0 || quantity <= 0)
        {
            callback(errorResponse(drogon::k400BadRequest, "INVALID_CART_DATA"));
            return;
        }

        try
        {
            const auto result = repo.addCartItem(userId.value(), productId, quantity);
            if (result != CartOperationResult::Success)
            {
                callback(cartOperationError(result));
                return;
            }

            Json::Value data;
            data["product_id"] = productId;
            data["message"] = "Product added to cart";
            Json::Value body;
            body["success"] = true;
            body["data"] = data;
            body["error"] = Json::nullValue;
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        }
        catch (...)
        {
            callback(errorResponse(drogon::k500InternalServerError, "DATABASE_ERROR"));
        }
    };

    auto updateCartItemHandler =
        [&repo](const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const std::string& productIdParam)
    {
        if (!isBuyer(request))
        {
            callback(errorResponse(drogon::k401Unauthorized, "BUYER_LOGIN_REQUIRED"));
            return;
        }

        auto userId = getUserId(request);
        auto json = request->getJsonObject();
        if (!userId.has_value())
        {
            callback(errorResponse(drogon::k401Unauthorized, "UNAUTHORIZED"));
            return;
        }
        if (!json)
        {
            callback(errorResponse(drogon::k400BadRequest, "INVALID_JSON"));
            return;
        }

        try
        {
            const int productId = std::stoi(productIdParam);
            const int quantity = (*json)["quantity"].asInt();
            if (productId <= 0 || quantity <= 0)
            {
                callback(errorResponse(drogon::k400BadRequest, "INVALID_CART_DATA"));
                return;
            }

            const auto result = repo.updateCartItemQuantity(userId.value(), productId, quantity);
            if (result != CartOperationResult::Success)
            {
                callback(cartOperationError(result));
                return;
            }

            Json::Value data;
            data["product_id"] = productId;
            data["quantity"] = quantity;
            Json::Value body;
            body["success"] = true;
            body["data"] = data;
            body["error"] = Json::nullValue;
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        }
        catch (const std::invalid_argument&)
        {
            callback(errorResponse(drogon::k400BadRequest, "INVALID_PRODUCT_ID"));
        }
        catch (const std::out_of_range&)
        {
            callback(errorResponse(drogon::k400BadRequest, "INVALID_PRODUCT_ID"));
        }
        catch (...)
        {
            callback(errorResponse(drogon::k500InternalServerError, "DATABASE_ERROR"));
        }
    };

    auto removeCartItemHandler =
        [&repo](const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const std::string& productIdParam)
    {
        if (!isBuyer(request))
        {
            callback(errorResponse(drogon::k401Unauthorized, "BUYER_LOGIN_REQUIRED"));
            return;
        }

        auto userId = getUserId(request);
        if (!userId.has_value())
        {
            callback(errorResponse(drogon::k401Unauthorized, "UNAUTHORIZED"));
            return;
        }

        try
        {
            const int productId = std::stoi(productIdParam);
            if (productId <= 0)
            {
                callback(errorResponse(drogon::k400BadRequest, "INVALID_PRODUCT_ID"));
                return;
            }
            if (!repo.removeCartItem(userId.value(), productId))
            {
                callback(errorResponse(drogon::k404NotFound, "CART_ITEM_NOT_FOUND"));
                return;
            }

            Json::Value data;
            data["product_id"] = productId;
            data["message"] = "Product removed from cart";
            Json::Value body;
            body["success"] = true;
            body["data"] = data;
            body["error"] = Json::nullValue;
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        }
        catch (const std::invalid_argument&)
        {
            callback(errorResponse(drogon::k400BadRequest, "INVALID_PRODUCT_ID"));
        }
        catch (const std::out_of_range&)
        {
            callback(errorResponse(drogon::k400BadRequest, "INVALID_PRODUCT_ID"));
        }
        catch (...)
        {
            callback(errorResponse(drogon::k500InternalServerError, "DATABASE_ERROR"));
        }
    };

    // =========================

    // PRODUCT ROUTES

    // =========================

    drogon::app().registerHandler(

        "/api/v1/products",

        getProductsHandler,

        {drogon::Get});

    drogon::app().registerHandler(

        "/api/v1/products",

        postProductHandler,

        {drogon::Post});

    drogon::app().registerHandler(

        "/api/v1/products/{id}",

        updateProductHandler,

        {drogon::Put});

    drogon::app().registerHandler(

        "/api/v1/products/{id}",

        deleteProductHandler,

        {drogon::Delete});

    drogon::app().registerHandler(

        "/api/v1/seller/products",

        getSellerProductsHandler,

        {drogon::Get});

    // F7: Admin-only user listing.
    auto adminUsersHandler =
        [&repo](const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        if (!isAdmin(request))
        {
            callback(errorResponse(drogon::k403Forbidden, "ADMIN_ONLY"));
            return;
        }

        try
        {
            Json::Value data;
            data["users"] = Json::Value(Json::arrayValue);

            for (const auto& user : repo.getAdminUsers())
            {
                Json::Value row;
                row["id"] = user.id;
                row["name"] = user.name;
                row["email"] = user.email;
                row["role"] = user.role;
                row["created_at"] = user.created_at;
                data["users"].append(row);
            }

            Json::Value body;
            body["success"] = true;
            body["data"] = data;
            body["error"] = Json::nullValue;
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        }
        catch (...)
        {
            callback(errorResponse(drogon::k500InternalServerError,
                                   "DATABASE_ERROR"));
        }
    };

    // F7: Admin-only order listing.
    auto adminOrdersHandler =
        [&repo](const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        if (!isAdmin(request))
        {
            callback(errorResponse(drogon::k403Forbidden, "ADMIN_ONLY"));
            return;
        }

        try
        {
            Json::Value data;
            data["orders"] = Json::Value(Json::arrayValue);

            for (const auto& order : repo.getAdminOrders())
            {
                Json::Value row;
                row["id"] = order.id;
                row["buyer_id"] = order.buyer_id;
                row["buyer_name"] = order.buyer_name;
                row["buyer_email"] = order.buyer_email;
                row["status"] = order.status;
                row["created_at"] = order.created_at;
                row["currency"] = "INR";
                row["total_cents"] =
                    static_cast<Json::Int64>(order.total_cents);
                row["item_count"] =
                    static_cast<Json::Int64>(order.item_count);
                data["orders"].append(row);
            }

            Json::Value body;
            body["success"] = true;
            body["data"] = data;
            body["error"] = Json::nullValue;
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        }
        catch (...)
        {
            callback(errorResponse(drogon::k500InternalServerError,
                                   "DATABASE_ERROR"));
        }
    };

    // F7: Admin removes a product listing.
    auto adminDeleteProductHandler =
        [&repo](const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback,
                const std::string& productIdParam)
    {
        if (!isAdmin(request))
        {
            callback(errorResponse(drogon::k403Forbidden, "ADMIN_ONLY"));
            return;
        }

        try
        {
            const int productId = std::stoi(productIdParam);
            const auto result = repo.removeProductAsAdmin(productId);

            if (result == AdminDeleteProductResult::NotFound)
            {
                callback(errorResponse(drogon::k404NotFound,
                                       "PRODUCT_NOT_FOUND"));
                return;
            }

            if (result == AdminDeleteProductResult::ProductHasOrders)
            {
                callback(errorResponse(drogon::k409Conflict,
                                       "PRODUCT_HAS_ORDER_HISTORY"));
                return;
            }

            Json::Value data;
            data["product_id"] = productId;
            data["message"] = "Product listing removed";

            Json::Value body;
            body["success"] = true;
            body["data"] = data;
            body["error"] = Json::nullValue;
            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        }
        catch (const std::invalid_argument&)
        {
            callback(errorResponse(drogon::k400BadRequest,
                                   "INVALID_PRODUCT_ID"));
        }
        catch (const std::out_of_range&)
        {
            callback(errorResponse(drogon::k400BadRequest,
                                   "INVALID_PRODUCT_ID"));
        }
        catch (...)
        {
            callback(errorResponse(drogon::k500InternalServerError,
                                   "DATABASE_ERROR"));
        }
    };
    // F6: Buyer order history.
    auto buyerOrderHistoryHandler =
        [&repo](const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        if (!isBuyer(request))
        {
            callback(errorResponse(
                drogon::k403Forbidden, "BUYER_ONLY"));
            return;
        }

        auto userId = getUserId(request);
        if (!userId.has_value())
        {
            callback(errorResponse(
                drogon::k401Unauthorized, "UNAUTHORIZED"));
            return;
        }

        try
        {
            auto orders = repo.getBuyerOrders(userId.value());

            Json::Value data;
            data["orders"] = Json::Value(Json::arrayValue);

            for (const auto& order : orders)
            {
                Json::Value row;
                row["id"] = order.id;
                row["status"] = order.status;
                row["created_at"] = order.created_at;
                row["currency"] = "INR";
                row["total_cents"] =
                    static_cast<Json::Int64>(order.total_cents);
                row["items"] = Json::Value(Json::arrayValue);

                for (const auto& item : order.items)
                {
                    Json::Value product;
                    product["product_id"] = item.product_id;
                    product["product_name"] = item.product_name;
                    product["quantity"] = item.quantity;
                    product["seller_id"] = item.seller_id;
                    product["unit_price_cents"] =
                        static_cast<Json::Int64>(item.unit_price_cents);
                    product["subtotal_cents"] =
                        static_cast<Json::Int64>(
                            item.unit_price_cents * item.quantity);

                    row["items"].append(product);
                }

                data["orders"].append(row);
            }

            Json::Value body;
            body["success"] = true;
            body["data"] = data;
            body["error"] = Json::nullValue;

            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        }
        catch (...)
        {
            callback(errorResponse(
                drogon::k500InternalServerError, "DATABASE_ERROR"));
        }
    };

    // F6: Seller's incoming order lines.
    auto sellerOrdersHandler =
        [&repo](const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        if (!isSeller(request))
        {
            callback(errorResponse(
                drogon::k403Forbidden, "SELLER_ONLY"));
            return;
        }

        auto userId = getUserId(request);
        if (!userId.has_value())
        {
            callback(errorResponse(
                drogon::k401Unauthorized, "UNAUTHORIZED"));
            return;
        }

        try
        {
            auto orders = repo.getSellerOrders(userId.value());

            Json::Value data;
            data["orders"] = Json::Value(Json::arrayValue);

            for (const auto& order : orders)
            {
                Json::Value row;
                row["order_id"] = order.order_id;
                row["buyer_id"] = order.buyer_id;
                row["status"] = order.status;
                row["created_at"] = order.created_at;
                row["currency"] = "INR";
                row["order_total_cents"] =
                    static_cast<Json::Int64>(order.total_cents);
                row["product_id"] = order.product_id;
                row["product_name"] = order.product_name;
                row["quantity"] = order.quantity;
                row["unit_price_cents"] =
                    static_cast<Json::Int64>(order.unit_price_cents);

                data["orders"].append(row);
            }

            Json::Value body;
            body["success"] = true;
            body["data"] = data;
            body["error"] = Json::nullValue;

            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        }
        catch (...)
        {
            callback(errorResponse(
                drogon::k500InternalServerError, "DATABASE_ERROR"));
        }
    };
    // F5: Buyer checkout with simulated payment.
    auto checkoutHandler =
        [&repo](const drogon::HttpRequestPtr& request,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        if (!isBuyer(request))
        {
            callback(errorResponse(drogon::k403Forbidden, "BUYER_ONLY"));
            return;
        }

        auto userId = getUserId(request);
        if (!userId.has_value())
        {
            callback(errorResponse(drogon::k401Unauthorized, "UNAUTHORIZED"));
            return;
        }

        try
        {
            const auto result = repo.checkoutCart(userId.value());

            if (result.status == CheckoutStatus::CartEmpty)
            {
                callback(errorResponse(drogon::k400BadRequest, "CART_EMPTY"));
                return;
            }

            if (result.status == CheckoutStatus::InsufficientStock)
            {
                callback(errorResponse(
                    drogon::k409Conflict, "INSUFFICIENT_STOCK"));
                return;
            }

            Json::Value data;
            data["order_id"] = result.order_id;
            data["status"] = "CONFIRMED";
            data["currency"] = "INR";
            data["total_cents"] =
                static_cast<Json::Int64>(result.total_cents);
            data["payment"]["method"] = "MOCK";
            data["payment"]["status"] = "SIMULATED_SUCCESS";

            Json::Value body;
            body["success"] = true;
            body["data"] = data;
            body["error"] = Json::nullValue;

            callback(drogon::HttpResponse::newHttpJsonResponse(body));
        }
        catch (...)
        {
            callback(errorResponse(
                drogon::k500InternalServerError, "CHECKOUT_FAILED"));
        }
    };
    // F4: buyer shopping cart endpoints.
    drogon::app().registerHandler(
        "/api/v1/cart", getCartHandler, {drogon::Get});

    drogon::app().registerHandler(
        "/api/v1/cart/items", addCartItemHandler, {drogon::Post});

    drogon::app().registerHandler(
        "/api/v1/cart/items/{id}", updateCartItemHandler, {drogon::Put});

    drogon::app().registerHandler(
        "/api/v1/cart/items/{id}", removeCartItemHandler, {drogon::Delete});

    drogon::app().registerHandler("/api/v1/checkout", checkoutHandler, {drogon::Post});

    // F6: Order history and seller incoming orders.
    drogon::app().registerHandler(
        "/api/v1/orders",
        buyerOrderHistoryHandler,
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/v1/seller/orders",
        sellerOrdersHandler,
        {drogon::Get});
    // F7: Admin endpoints.
    drogon::app().registerHandler(
        "/api/v1/admin/users", adminUsersHandler, {drogon::Get});

    drogon::app().registerHandler(
        "/api/v1/admin/orders", adminOrdersHandler, {drogon::Get});

    drogon::app().registerHandler(
        "/api/v1/admin/products/{id}",
        adminDeleteProductHandler, {drogon::Delete});
    // Preserve old routes.

    drogon::app().registerHandler(

        "/products",

        getProductsHandler,

        {drogon::Get});

    drogon::app().registerHandler(

        "/products",

        postProductHandler,

        {drogon::Post});

    // =========================

    // HEALTH CHECK

    // =========================

    drogon::app().registerHandler(

        "/api/v1/health",

        [&repo](

            const drogon::HttpRequestPtr&,

            std::function<void(

                const drogon::HttpResponsePtr&)>&& callback)

        {

            Json::Value result;

            if (repo.isDatabaseHealthy())

            {

                result["status"] = "UP";

                result["db"] = "UP";

                callback(

                    drogon::HttpResponse::

                        newHttpJsonResponse(result));

            }

            else

            {

                result["status"] = "DOWN";

                result["db"] = "DOWN";

                auto response =

                    drogon::HttpResponse::

                        newHttpJsonResponse(result);

                response->setStatusCode(

                    drogon::k503ServiceUnavailable);

                callback(response);

            }

        },

        {drogon::Get});

    // =========================

    // FRONTEND

    // =========================

    drogon::app().setDocumentRoot("./src");

    drogon::app().setHomePage("index.html");

    int port = 8080;

    const char* portEnv =

        std::getenv("PORT");

    if (portEnv)

        port = std::stoi(portEnv);

    drogon::app()

        .addListener("0.0.0.0", port)

        .run();

    return 0;

}

