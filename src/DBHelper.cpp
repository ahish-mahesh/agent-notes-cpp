#include "DBHelper.h"

#include <string>
#include <vector>
#include <memory>

#include <sqlite3.h>

DBHelper::DBHelper(const std::string &dbPath)
    : db_(nullptr)
{
    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK)
    {
        throw std::runtime_error("Failed to open database: " + dbPath);
    }

    // Initialize the database if it doesn't exist
    if (!createDB(dbPath))
    {
        createDB(dbPath);
    }
}

DBHelper::~DBHelper()
{
    if (db_)
    {
        sqlite3_close(db_);
    }
}

bool DBHelper::execute(const std::string &query)
{
    char *errMsg = nullptr;
    int result = sqlite3_exec(db_, query.c_str(), nullptr, nullptr, &errMsg);

    if (result != SQLITE_OK)
    {
        std::string error = "SQL error: " + std::string(errMsg);
        sqlite3_free(errMsg);
        throw std::runtime_error(error);
    }

    return true;
}

DBHelper::SaveTranscriptionResult(const std::string &result)
{
    if (result.empty())
    {
        return false; // Nothing to save
    }

    std::string query = "INSERT INTO transcriptions (result) VALUES ('" + sqlite3_mprintf("%q", result.c_str()) + "');";

    try
    {
        return execute(query);
    }
    catch (const std::runtime_error &e)
    {
        throw std::runtime_error("Failed to save transcription result: " + std::string(e.what()));
    }
}

DBHelper::createDB(const std::string &dbPath)
{
    if (db_)
    {
        return true; // Database already exists
    }

    if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK)
    {
        throw std::runtime_error("Failed to create database: " + dbPath);
    }

    // Create the transcriptions table if it doesn't exist
    std::string createTableQuery = "CREATE TABLE IF NOT EXISTS transcriptions ("
                                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                   "result TEXT NOT NULL, "
                                   "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";

    try
    {
        return execute(createTableQuery);
    }
    catch (const std::runtime_error &e)
    {
        throw std::runtime_error("Failed to create transcriptions table: " + std::string(e.what()));
    }
}

sqlite3 *DBHelper::getDBHandle() const
{
    return db_;
}
