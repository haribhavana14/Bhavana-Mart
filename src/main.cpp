#include <drogon/drogon.h>
#include "../repositories/productrepository.h"

#include <cstdlib>
#include <string>

int main() {
    ProductRepository repo;

    // Seed/demo products
    repo.addProduct({1, "Laptop", 57000, 10});
    repo.addProduct({2, "Mouse", 800, 25});
    repo.addProduct({3, "Keyboard", 1200, 15});

    auto getProductsHandler =
        [&repo](const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {

            auto products = repo.getProducts();
            Json::Value result(Json::arrayValue);

            for (const auto& p : products) {
                Json::Value item;
                item["id"] = p.id;
                item["name"] = p.name;
                item["price"] = p.price;
                item["stock"] = p.stock;
                result.append(item);
            }

            callback(
                drogon::HttpResponse::newHttpJsonResponse(result)
            );
        };

    auto postProductHandler =
        [&repo](const drogon::HttpRequestPtr& req,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {

            auto json = req->getJsonObject();

            if (!json) {
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

            auto resp =
                drogon::HttpResponse::newHttpJsonResponse(result);

            resp->setStatusCode(drogon::k201Created);

            callback(resp);
        };

    // Existing API
    drogon::app().registerHandler(
        "/products",
        getProductsHandler,
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/products",
        postProductHandler,
        {drogon::Post}
    );

    // Versioned API
    drogon::app().registerHandler(
        "/api/v1/products",
        getProductsHandler,
        {drogon::Get}
    );

    drogon::app().registerHandler(
        "/api/v1/products",
        postProductHandler,
        {drogon::Post}
    );

    // Health check
    drogon::app().registerHandler(
        "/api/v1/health",
        [&repo](const drogon::HttpRequestPtr&,
                std::function<void(const drogon::HttpResponsePtr&)>&& callback) {

            Json::Value result;

            if (repo.isDatabaseHealthy()) {
                result["status"] = "UP";
                result["db"] = "UP";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(result);

                resp->setStatusCode(drogon::k200OK);
                callback(resp);
            } else {
                result["status"] = "DOWN";
                result["db"] = "DOWN";

                auto resp =
                    drogon::HttpResponse::newHttpJsonResponse(result);

                resp->setStatusCode(drogon::k500InternalServerError);
                callback(resp);
            }
        },
        {drogon::Get}
    );

    // Render provides PORT; use 8080 locally
    int port = 8080;

    const char* portEnv = std::getenv("PORT");

    if (portEnv != nullptr) {
        port = std::stoi(portEnv);
    }

    drogon::app()
        .addListener("0.0.0.0", port)
        .run();

    return 0;
}