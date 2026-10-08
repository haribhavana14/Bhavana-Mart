#pragma once

#include <drogon/drogon.h>

#include <string>

class MigrationService
{
public:
    explicit MigrationService(
        drogon::orm::DbClientPtr db_client);

    void Run(
        const std::string& migrations_directory,
        const std::string& seed_file);

private:
    drogon::orm::DbClientPtr db_client_;

    void ApplyMigration(
        const std::string& version,
        const std::string& sql);

    static std::string ReadFile(
        const std::string& file_path);
};
