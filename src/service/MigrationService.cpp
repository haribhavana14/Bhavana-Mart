#include "MigrationService.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
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
            "Unable to open SQL file: " + file_path);

    std::ostringstream content;
    content << file.rdbuf();

    return content.str();
}

static std::string Trim(const std::string& value)
{
    const auto first = value.find_first_not_of(" \t\r\n");

    if (first == std::string::npos)
        return "";

    const auto last = value.find_last_not_of(" \t\r\n");

    return value.substr(first, last - first + 1);
}

static std::vector<std::string> SplitSqlStatements(
    const std::string& sql)
{
    std::vector<std::string> statements;
    std::string current;

    bool inSingleQuote = false;
    bool inDoubleQuote = false;
    bool inLineComment = false;
    bool inBlockComment = false;

    for (std::size_t i = 0; i < sql.size(); ++i)
    {
        const char c = sql[i];
        const char next =
            (i + 1 < sql.size()) ? sql[i + 1] : '\0';

        if (inLineComment)
        {
            if (c == '\n')
                inLineComment = false;

            continue;
        }

        if (inBlockComment)
        {
            if (c == '*' && next == '/')
            {
                inBlockComment = false;
                ++i;
            }

            continue;
        }

        if (!inSingleQuote && !inDoubleQuote)
        {
            if (c == '-' && next == '-')
            {
                inLineComment = true;
                ++i;
                continue;
            }

            if (c == '/' && next == '*')
            {
                inBlockComment = true;
                ++i;
                continue;
            }
        }

        if (c == '\'' && !inDoubleQuote)
        {
            current += c;

            if (inSingleQuote && next == '\'')
            {
                current += next;
                ++i;
                continue;
            }

            inSingleQuote = !inSingleQuote;
            continue;
        }

        if (c == '"' && !inSingleQuote)
        {
            current += c;

            if (inDoubleQuote && next == '"')
            {
                current += next;
                ++i;
                continue;
            }

            inDoubleQuote = !inDoubleQuote;
            continue;
        }

        if (c == ';' &&
            !inSingleQuote &&
            !inDoubleQuote)
        {
            const std::string statement = Trim(current);

            if (!statement.empty())
                statements.push_back(statement);

            current.clear();
            continue;
        }

        current += c;
    }

    const std::string lastStatement = Trim(current);

    if (!lastStatement.empty())
        statements.push_back(lastStatement);

    return statements;
}

void MigrationService::ApplyMigration(
    const std::string& version,
    const std::string& sql)
{
    auto transaction = db_client_->newTransaction();

    const auto applied = transaction->execSqlSync(
        "SELECT 1 FROM schema_migrations "
        "WHERE version = $1",
        version);

    if (!applied.empty())
        return;

    const auto statements = SplitSqlStatements(sql);

    if (statements.empty())
        throw std::runtime_error(
            "Migration contains no SQL statements: " + version);

    for (const auto& statement : statements)
    {
        transaction->execSqlSync(statement);
    }

    transaction->execSqlSync(
        "INSERT INTO schema_migrations "
        "(version) VALUES ($1)",
        version);
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

    const auto seed_statements =
        SplitSqlStatements(seed_sql);

    auto seed_transaction = db_client_->newTransaction();

    for (const auto& statement : seed_statements)
    {
        seed_transaction->execSqlSync(statement);
    }
}
