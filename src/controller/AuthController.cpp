#include "AuthController.h"

#include "../repository/AuthRepository.h"

#include <stdexcept>

AuthController::AuthController()
{
    repository_ =
        std::make_shared<AuthRepository>();

    service_ =
        std::make_unique<AuthService>(repository_);
}

drogon::HttpResponsePtr AuthController::errorResponse(
    const std::string& code,
    drogon::HttpStatusCode status)
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

void AuthController::registerUser(
    const drogon::HttpRequestPtr& req,
    std::function<void(
        const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();

    if (!json)
    {
        callback(errorResponse(
            "INVALID_JSON",
            drogon::k400BadRequest));
        return;
    }

    const std::string name =
        (*json)["name"].asString();

    const std::string email =
        (*json)["email"].asString();

    const std::string password =
        (*json)["password"].asString();

    const std::string role =
        (*json)["role"].asString();

    try
    {
        const int userId =
            service_->RegisterUser(
                name,
                email,
                password,
                role);

        Json::Value data;
        data["id"] = userId;

        Json::Value body;

        body["success"] = true;
        body["data"] = data;
        body["error"] = Json::nullValue;

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(body);

        response->setStatusCode(
            drogon::k201Created);

        callback(response);
    }
    catch (const std::invalid_argument&)
    {
        callback(errorResponse(
            "INVALID_REGISTRATION_DATA",
            drogon::k400BadRequest));
    }
    catch (...)
    {
        callback(errorResponse(
            "REGISTRATION_FAILED",
            drogon::k500InternalServerError));
    }
}

void AuthController::loginUser(
    const drogon::HttpRequestPtr& req,
    std::function<void(
        const drogon::HttpResponsePtr&)>&& callback)
{
    auto json = req->getJsonObject();

    if (!json)
    {
        callback(errorResponse(
            "INVALID_JSON",
            drogon::k400BadRequest));
        return;
    }

    const std::string email =
        (*json)["email"].asString();

    const std::string password =
        (*json)["password"].asString();

    try
    {
        const User user =
            service_->LoginUser(
                email,
                password);

        req->session()->clear();
        req->session()->changeSessionIdToClient();

        req->session()->insert(
            "user_id",
            user.id);

        req->session()->insert(
            "role",
            user.role);

        req->session()->insert(
            "user_name",
            user.name);

        Json::Value data;

        data["id"] = user.id;
        data["name"] = user.name;
        data["email"] = user.email;
        data["role"] = user.role;

        Json::Value body;

        body["success"] = true;
        body["data"] = data;
        body["error"] = Json::nullValue;

        callback(
            drogon::HttpResponse::newHttpJsonResponse(body));
    }
    catch (const std::invalid_argument&)
    {
        callback(errorResponse(
            "INVALID_LOGIN_DATA",
            drogon::k400BadRequest));
    }
    catch (...)
    {
        callback(errorResponse(
            "INVALID_CREDENTIALS",
            drogon::k401Unauthorized));
    }
}

void AuthController::logoutUser(
    const drogon::HttpRequestPtr& req,
    std::function<void(
        const drogon::HttpResponsePtr&)>&& callback)
{
    req->session()->clear();
    req->session()->changeSessionIdToClient();

    Json::Value body;

    body["success"] = true;
    body["data"]["message"] = "Logged out";
    body["error"] = Json::nullValue;

    callback(
        drogon::HttpResponse::newHttpJsonResponse(body));
}

void AuthController::currentUser(
    const drogon::HttpRequestPtr& req,
    std::function<void(
        const drogon::HttpResponsePtr&)>&& callback)
{
    const auto userId =
        req->session()->getOptional<int>("user_id");

    if (!userId.has_value())
    {
        callback(errorResponse(
            "UNAUTHORIZED",
            drogon::k401Unauthorized));
        return;
    }

    Json::Value data;

    data["id"] = userId.value();

    data["name"] =
        req->session()->get<std::string>(
            "user_name");

    data["role"] =
        req->session()->get<std::string>(
            "role");

    Json::Value body;

    body["success"] = true;
    body["data"] = data;
    body["error"] = Json::nullValue;

    callback(
        drogon::HttpResponse::newHttpJsonResponse(body));
}
