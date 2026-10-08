#include "MigrationService.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>

MigrationService::MigrationService(
    drogon::orm::DbClientPtr db_client)
    : db_client_(std::move(db_client))
{
    if (!db_client_)
        throw std::invalid_argument("Database client is required");
}

std::string MigrationService::ReadFile(
    const std::string& file_path)
{
    std::ifstream file(file_path);

    if (!file.is_open())
        throw std::runtime_error(
            "Unable to open migration file");

    std::ostringstream content;
    content << file.rdbuf();

    return content.str();
}

void MigrationService::ApplyMigration(
    const std::string& version,
    const std::string& sql)
{
    db_client_->execSqlSync("BEGIN");

    try
    {
        auto applied = db_client_->execSqlSync(
            "SELECT 1 FROM schema_migrations "
            "WHERE version = $1",
            version);

        if (applied.empty())
        {
            db_client_->execSqlSync(sql);

            db_client_->execSqlSync(
                "INSERT INTO schema_migrations "
                "(version) VALUES ($1)",
                version);
        }

        db_client_->execSqlSync("COMMIT");
    }
    catch (...)
    {
        try
        {
            db_client_->execSqlSync("ROLLBACK");
        }
        catch (...)
        {
        }

        throw;
    }
}

void MigrationService::Run(
    const std::string& migrations_directory,
    const std::string& seed_file)
{
    db_client_->execSqlSync(
        "CREATE TABLE IF NOT EXISTS schema_migrations ("
        "version VARCHAR(255) PRIMARY KEY,"
        "applied_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP"
        ")");

    std::vector<std::filesystem::path> migration_files;

    for (const auto& entry :
         std::filesystem::directory_iterator(
             migrations_directory))
    {
        if (entry.is_regular_file() &&
            entry.path().extension() == ".sql")
        {
            migration_files.push_back(entry.path());
        }
    }

    std::sort(
        migration_files.begin(),
        migration_files.end());

    for (const auto& migration : migration_files)
    {
        const std::string version =
            migration.filename().string();

        const std::string sql =
            ReadFile(migration.string());

        ApplyMigration(version, sql);
    }

    const std::string seed_sql =
        ReadFile(seed_file);

    db_client_->execSqlSync(seed_sql);
}
