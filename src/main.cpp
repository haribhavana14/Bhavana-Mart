#include <drogon/drogon.h>
#include "../repositories/ProductRepository.h"

int main() {
    ProductRepository repo;

    repo.addProduct({1, "Laptop", 57000, 10});
    repo.addProduct({2, "Mouse", 800, 25});
    repo.addProduct({3, "Keyboard", 1200, 15});

    // GET /products
    drogon::app().registerHandler(
        "/products",
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

            callback(drogon::HttpResponse::newHttpJsonResponse(result));
        },
        {drogon::Get}
    );

    // POST /products
    drogon::app().registerHandler(
        "/products",
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

            auto resp = drogon::HttpResponse::newHttpJsonResponse(result);
            resp->setStatusCode(drogon::k201Created);
            callback(resp);
        },
        {drogon::Post}
    );

    drogon::app()
        .addListener("0.0.0.0", 8080)
        .run();
}