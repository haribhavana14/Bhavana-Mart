#include <drogon/drogon.h>

#include "../repositories/productrepository.h"
#include "../plugin/DatabasePlugin.h"

#include <cstdlib>
#include <functional>
#include <optional>
#include <string>

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

int main()
{
    drogon::app().loadConfigFile("./config.json");

    drogon::app().enableSession(1200);

    ProductRepository repo;

    // =========================
    // GET ALL PRODUCTS
    // =========================
    auto getProductsHandler =
        [&repo](
            const drogon::HttpRequestPtr&,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
    {
        try
        {
            auto products = repo.getProducts();

            Json::Value result(Json::arrayValue);

            for (const auto& p : products)
            {
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

        product.id =
            std::stoi(request->getParameter("1"));

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
            const int productId =
                std::stoi(request->getParameter("1"));

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
        "/api/v1/products/{1}",
        updateProductHandler,
        {drogon::Put});

    drogon::app().registerHandler(
        "/api/v1/products/{1}",
        deleteProductHandler,
        {drogon::Delete});

    drogon::app().registerHandler(
        "/api/v1/seller/products",
        getSellerProductsHandler,
        {drogon::Get});

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

