#pragma once

#include <memory>
#include <string>

#include "../model/User.h"
#include "../repository/AuthRepository.h"

/**
 * @brief Handles authentication business logic.
 */
class AuthService
{
public:
    /**
     * @brief Creates the service with a user repository.
     */
    explicit AuthService(std::shared_ptr<IAuthRepository> repository);

    /**
     * @brief Registers a new buyer or seller.
     */
    int RegisterUser(const std::string& name,
                     const std::string& email,
                     const std::string& password,
                     const std::string& role);

    /**
     * @brief Authenticates a user by email and password.
     */
    User LoginUser(const std::string& email,
                   const std::string& password);

private:
    std::shared_ptr<IAuthRepository> repository_;

    static void ValidateRegistration(const std::string& name,
                                     const std::string& email,
                                     const std::string& password,
                                     const std::string& role);

    static void ValidateEmail(const std::string& email);
};
