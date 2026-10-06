#pragma once

#include <drogon/drogon.h>
#include <memory>
#include <optional>
#include <string>

#include "../model/User.h"

/**
 * @brief Repository interface for user persistence.
 */
class IAuthRepository
{
public:
    virtual ~IAuthRepository() = default;

    /**
     * @brief Finds a user by email.
     */
    virtual std::optional<User> findByEmail(
        const std::string& email) = 0;

    /**
     * @brief Creates a new user.
     * @return Newly created user ID.
     */
    virtual int createUser(const User& user) = 0;
};

/**
 * @brief PostgreSQL implementation of the authentication repository.
 */
class AuthRepository : public IAuthRepository
{
public:
    /**
     * @brief Finds a user by email.
     */
    std::optional<User> findByEmail(
        const std::string& email) override;

    /**
     * @brief Creates a new user.
     */
    int createUser(const User& user) override;

private:
    drogon::orm::DbClientPtr getClient();
};
