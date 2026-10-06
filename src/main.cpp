#include <drogon/drogon.h>

#include "../repositories/productrepository.h"
#include "../plugin/DatabasePlugin.h"

#include <cstdlib>
#include <functional>
#include <string>

int main()
{
    // Load Drogon configuration and PostgreSQL plugin.
    drogon::app().loadConfigFile("./config.json");

    // Enable server-side sessions with a 20-minute idle timeout.
    drogon::app().enableSession(1200);

    ProductRepository repo;

    // GET /products and /api/v1/products
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

                    // Money is stored as integer minor units.
                    item["price"] =
                        static_cast<Json::Int64>(p.price.cents);

                    item["stock"] = p.stock_qty;
                    item["category"] = p.category;
                    item["image_url"] = p.image_url;

                    result.append(item);
                }

                callback(
                    drogon::HttpResponse::newHttpJsonResponse(result));
            }
            catch (...)
            {
                Json::Value error;

                error["success"] = false;
                error["data"] = Json::nullValue;
                error["error"]["code"] = "DATABASE_ERROR";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    drogon::k500InternalServerError);

                callback(response);
            }
        };

    // POST /products and /api/v1/products
    auto postProductHandler =
        [&repo](
            const drogon::HttpRequestPtr& request,
            std::function<void(
                const drogon::HttpResponsePtr&)>&& callback)
        {
            auto json = request->getJsonObject();

            if (!json)
            {
                Json::Value error;

                error["success"] = false;
                error["data"] = Json::nullValue;
                error["error"]["code"] = "INVALID_JSON";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    drogon::k400BadRequest);

                callback(response);
                return;
            }

            Product product;

            product.id =
                (*json)["id"].asInt();

            product.seller_id =
                (*json)["seller_id"].asInt();

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
                Json::Value error;

                error["success"] = false;
                error["data"] = Json::nullValue;
                error["error"]["code"] =
                    "INVALID_PRODUCT_DATA";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    drogon::k400BadRequest);

                callback(response);
                return;
            }

            try
            {
                repo.addProduct(product);

                Json::Value data;
                data["id"] = product.id;

                Json::Value responseJson;

                responseJson["success"] = true;
                responseJson["data"] = data;
                responseJson["error"] = Json::nullValue;

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        responseJson);

                response->setStatusCode(
                    drogon::k201Created);

                callback(response);
            }
            catch (...)
            {
                Json::Value error;

                error["success"] = false;
                error["data"] = Json::nullValue;
                error["error"]["code"] =
                    "DATABASE_ERROR";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(error);

                response->setStatusCode(
                    drogon::k500InternalServerError);

                callback(response);
            }
        };

    drogon::app().registerHandler(
        "/products",
        getProductsHandler,
        {drogon::Get});

    drogon::app().registerHandler(
        "/products",
        postProductHandler,
        {drogon::Post});

    drogon::app().registerHandler(
        "/api/v1/products",
        getProductsHandler,
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/v1/products",
        postProductHandler,
        {drogon::Post});

    // Health check
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
                    drogon::HttpResponse::newHttpJsonResponse(
                        result));
            }
            else
            {
                result["status"] = "DOWN";
                result["db"] = "DOWN";

                auto response =
                    drogon::HttpResponse::newHttpJsonResponse(
                        result);

                response->setStatusCode(
                    drogon::k503ServiceUnavailable);

                callback(response);
            }
        },
        {drogon::Get});

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
