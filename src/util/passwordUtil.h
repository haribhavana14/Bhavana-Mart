#pragma once

#include <string>

class PasswordUtil
{
public:
    static std::string HashPassword(const std::string& password);
    static bool VerifyPassword(const std::string& password,
                               const std::string& hash);
};