#pragma once

#include <drogon/drogon.h>

class DatabasePlugin : public drogon::Plugin<DatabasePlugin>
{
public:
    void initAndStart(const Json::Value &config) override;
    void shutdown() override;

    drogon::orm::DbClientPtr getClient() const;

private:
    drogon::orm::DbClientPtr dbClient_;
};