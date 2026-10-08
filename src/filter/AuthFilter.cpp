#include "AuthFilter.h"

void AuthFilter::doFilter(
    const drogon::HttpRequestPtr& req,
    drogon::FilterCallback&& fcb,
    drogon::FilterChainCallback&& fccb)
{
    const auto userId =
        req->session()->getOptional<int>("user_id");

    if (!userId.has_value() || userId.value() <= 0)
    {
        Json::Value body;

        body["success"] = false;
        body["data"] = Json::nullValue;
        body["error"]["code"] = "UNAUTHORIZED";

        auto response =
            drogon::HttpResponse::newHttpJsonResponse(body);

        response->setStatusCode(
            drogon::k401Unauthorized);

        fcb(response);
        return;
    }

    fccb();
}

