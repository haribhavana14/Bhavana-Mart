#include "DatabasePlugin.h"

#include <cstdlib>
#include <stdexcept>

void DatabasePlugin::initAndStart(const Json::Value &config)
{
    (void)config;

    const char *databaseUrl = std::getenv("DATABASE_URL");

    if (!databaseUrl)
    {
        throw std::runtime_error("DATABASE_URL is not set");
    }

    dbClient_ = drogon::orm::DbClient::newPgClient(
        databaseUrl,
        4
    );
}

void DatabasePlugin::shutdown()
{
    dbClient_.reset();
}

drogon::orm::DbClientPtr DatabasePlugin::getClient() const
{
    return dbClient_;
}