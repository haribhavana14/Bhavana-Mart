#include <drogon/drogon.h>

#include "../repositories/productrepository.h"
#include "../plugin/DatabasePlugin.h"

#include <cstdlib>
#include <functional>
#include <string>

int main()
{
    // Load Drogon configuration and DatabasePlugin
    drogon::app().loadConfigFile("./config.json");

    ProductRepository repo;

    // =========================
    // GET /products
    // =========================
    auto getProductsHandler =
        [&repo](const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        auto products = repo.getProducts();

        Json::Value result(Json::arrayValue);

        for (const auto& p : products)
        {
            Json::Value item;
            item["id"] = p.id;
            item["name"] = p.name;
            item["price"] = p.price;
            item["stock"] = p.stock;

            result.append(item);
        }

        auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
        callback(resp);
    };

    // =========================
    // POST /products
    // =========================
    auto postProductHandler =
        [&repo](const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
    {
        auto json = req->getJsonObject();

        if (!json)
        {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setStatusCode(drogon::k400BadRequest);
            resp->setBody("Invalid JSON");
            callback(resp);
            return;
        }

        Product p;

        p.id = (*json)["id"].asInt();
        p.name = (*json)["name"].asString();
        p.price = (*json)["price"].asDouble();
        p.stock = (*json)["stock"].asInt();

        repo.addProduct(p);

        Json::Value result;
        result["message"] = "Product added successfully";
        result["id"] = p.id;

        auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
        resp->setStatusCode(drogon::k201Created);

        callback(resp);
    };

    // =========================
    // Existing product routes
    // =========================
    drogon::app().registerHandler(
        "/products",
        getProductsHandler,
        {drogon::Get});

    drogon::app().registerHandler(
        "/products",
        postProductHandler,
        {drogon::Post});

    // =========================
    // Versioned API routes
    // =========================
    drogon::app().registerHandler(
        "/api/v1/products",
        getProductsHandler,
        {drogon::Get});

    drogon::app().registerHandler(
        "/api/v1/products",
        postProductHandler,
        {drogon::Post});

    // =========================
    // Health check
    // =========================
    drogon::app().registerHandler(
        "/api/v1/health",
        [&repo](const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback)
        {
            Json::Value result;

            if (repo.isDatabaseHealthy())
            {
                result["status"] = "UP";
                result["db"] = "UP";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(result);

                resp->setStatusCode(drogon::k200OK);
                callback(resp);
            }
            else
            {
                result["status"] = "DOWN";
                result["db"] = "DOWN";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(result);

                resp->setStatusCode(drogon::k503ServiceUnavailable);
                callback(resp);
            }
        },
        {drogon::Get});

    // =========================
    // Serve frontend
    // =========================
    drogon::app().setDocumentRoot("./src");
    drogon::app().setHomePage("index.html");

    // =========================
    // Render PORT
    // =========================
    int port = 8080;

    const char* portEnv = std::getenv("PORT");

    if (portEnv != nullptr)
    {
        port = std::stoi(portEnv);
    }

    // =========================
    // Start server
    // =========================
    drogon::app()
        .addListener("0.0.0.0", port)
        .run();

    return 0;
}