#include "AuthRepository.h"

#include "../../plugin/DatabasePlugin.h"

#include <stdexcept>

drogon::orm::DbClientPtr AuthRepository::getClient()
{
    auto plugin = drogon::app().getPlugin<DatabasePlugin>();

    if (!plugin)
        throw std::runtime_error("DatabasePlugin is not available");

    auto client = plugin->getClient();

    if (!client)
        throw std::runtime_error("Database client is not available");

    return client;
}

std::optional<User> AuthRepository::findByEmail(
    const std::string& email)
{
    auto db = getClient();

    auto result = db->execSqlSync(
        "SELECT id, name, email, password_hash, role "
        "FROM users "
        "WHERE email = $1 "
        "LIMIT 1",
        email);

    if (result.empty())
        return std::nullopt;

    User user;
    user.id = result[0]["id"].as<int>();
    user.name = result[0]["name"].as<std::string>();
    user.email = result[0]["email"].as<std::string>();
    user.password_hash =
        result[0]["password_hash"].as<std::string>();
    user.role = result[0]["role"].as<std::string>();

    return user;
}

int AuthRepository::createUser(const User& user)
{
    auto db = getClient();

    auto result = db->execSqlSync(
        "INSERT INTO users "
        "(name, email, password_hash, role) "
        "VALUES ($1, $2, $3, $4) "
        "RETURNING id",
        user.name,
        user.email,
        user.password_hash,
        user.role);

    if (result.empty())
        throw std::runtime_error("User creation failed");

    return result[0]["id"].as<int>();
}
