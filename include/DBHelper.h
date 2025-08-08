#pragma once

#include <string>
#include <vector>
#include <memory>

#include <sqlite3.h>

/**
 * @brief Database helper class for SQLite operations
 *
 * Abstracts the application context from the SQLite database.
 * Provides methods related to transcriptions and chats.
 */
class DBHelper
{
public:
    /**
     * @brief Constructor
     * @param dbPath Path to the SQLite database file
     */
    explicit DBHelper(const std::string &dbPath);

    /**
     * @brief Destructor
     */
    ~DBHelper();

    /**
     * @brief Saves the transcription result to the database
     * @param result Transcription result as a string
     * @return true if the save operation was successful, false otherwise
     * @throws std::runtime_error if the save operation fails
     */
    bool SaveTranscriptionResult(const std::string &result);

private:
    sqlite3 *db_; ///< SQLite database handle

    /**
     * @brief create a new database file if it doesn't exist
     * @param dbPath Path to the database file
     * @return true if the database was created successfully, false otherwise
     * @note This method is used to ensure the database file exists before performing operations.
     * It will create a new database file if it doesn't already exist.
     * If the file already exists, it will not modify it.
     * @throws std::runtime_error if the database cannot be opened or created
     * @note This method is not part of the public API and is used internally by the
     * constructor to ensure the database is ready for use.
     */
    bool createDB(const std::string &dbPath);

    /**
     * @brief Execute a SQL query
     * @param query SQL query string
     * @return true if the query executed successfully, false otherwise
     * @throws std::runtime_error if the query fails
     */
    bool execute(const std::string &query);

    /**
     * @brief Get the database handle
     * @return Pointer to the SQLite database handle
     */
    sqlite3 *getDBHandle() const { return db_; }
};
