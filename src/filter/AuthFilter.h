#pragma once

#include <drogon/HttpFilter.h>

/**
 * @brief Ensures that protected endpoints have an authenticated session.
 */
class AuthFilter : public drogon::HttpFilter<AuthFilter>
{
public:
    /**
     * @brief Checks whether a valid user session exists.
     */
    void doFilter(
        const drogon::HttpRequestPtr& req,
        drogon::FilterCallback&& fcb,
        drogon::FilterChainCallback&& fccb) override;
};

