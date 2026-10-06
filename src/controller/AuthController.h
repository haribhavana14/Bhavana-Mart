#pragma once

#include <drogon/HttpController.h>
#include "../service/AuthService.h"

#include <memory>

class AuthController : public drogon::HttpController<AuthController>
{
public:
    METHOD_LIST_BEGIN

    ADD_METHOD_TO(AuthController::registerUser,
                  "/api/v1/auth/register",
                  drogon::Post);

    ADD_METHOD_TO(AuthController::loginUser,
                  "/api/v1/auth/login",
                  drogon::Post);

    ADD_METHOD_TO(AuthController::logoutUser,
                  "/api/v1/auth/logout",
                  drogon::Post);

    ADD_METHOD_TO(AuthController::currentUser,
                  "/api/v1/auth/me",
                  drogon::Get);

    METHOD_LIST_END

    AuthController();

    void registerUser(
        const drogon::HttpRequestPtr& req,
        std::function<void(
            const drogon::HttpResponsePtr&)>&& callback);

    void loginUser(
        const drogon::HttpRequestPtr& req,
        std::function<void(
            const drogon::HttpResponsePtr&)>&& callback);

    void logoutUser(
        const drogon::HttpRequestPtr& req,
        std::function<void(
            const drogon::HttpResponsePtr&)>&& callback);

    void currentUser(
        const drogon::HttpRequestPtr& req,
        std::function<void(
            const drogon::HttpResponsePtr&)>&& callback);

private:
    std::shared_ptr<IAuthRepository> repository_;
    std::unique_ptr<AuthService> service_;

    static drogon::HttpResponsePtr errorResponse(
        const std::string& code,
        drogon::HttpStatusCode status);
};