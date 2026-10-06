#pragma once

#include <string>

/**
 * @brief Represents a marketplace user.
 */
struct User
{
    int id{0};
    std::string name;
    std::string email;
    std::string password_hash;
    std::string role;
};
