#include "AuthService.h"

#include "../util/PasswordUtil.h"

#include <regex>
#include <stdexcept>

AuthService::AuthService(std::shared_ptr<IAuthRepository> repository)
    : repository_(std::move(repository))
{
    if (!repository_)
        throw std::invalid_argument("Authentication repository is required");
}

void AuthService::ValidateEmail(const std::string& email)
{
    static const std::regex pattern(
        R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");

    if (!std::regex_match(email, pattern))
        throw std::invalid_argument("Invalid email address");
}

void AuthService::ValidateRegistration(const std::string& name,
                                        const std::string& email,
                                        const std::string& password,
                                        const std::string& role)
{
    if (name.empty() || name.size() > 100)
        throw std::invalid_argument("Invalid name");

    if (email.empty() || email.size() > 255)
        throw std::invalid_argument("Invalid email");

    ValidateEmail(email);

    if (password.size() < 8)
        throw std::invalid_argument("Password must contain at least 8 characters");

    if (role != "BUYER" && role != "SELLER")
        throw std::invalid_argument("Invalid registration role");
}

int AuthService::RegisterUser(const std::string& name,
                              const std::string& email,
                              const std::string& password,
                              const std::string& role)
{
    ValidateRegistration(name, email, password, role);

    if (repository_->findByEmail(email).has_value())
        throw std::runtime_error("Email already registered");

    User user;
    user.name = name;
    user.email = email;
    user.password_hash = PasswordUtil::HashPassword(password);
    user.role = role;

    return repository_->createUser(user);
}

User AuthService::LoginUser(const std::string& email,
                            const std::string& password)
{
    if (email.empty() || password.empty())
        throw std::invalid_argument("Email and password are required");

    auto user = repository_->findByEmail(email);

    if (!user.has_value())
        throw std::runtime_error("Invalid email or password");

    if (!PasswordUtil::VerifyPassword(password, user->password_hash))
        throw std::runtime_error("Invalid email or password");

    return user.value();
}
