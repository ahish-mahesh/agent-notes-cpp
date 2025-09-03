#include "DBHelper.h"

#include <string>

#include <sqlite3.h>

DBHelper::DBHelper(const std::string &dbPath) : db_(nullptr) {
  if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
    throw std::runtime_error("Failed to open database: " + dbPath);
  }

  createDB(dbPath);
}

DBHelper::~DBHelper() {
  if (db_) {
    sqlite3_close(db_);
  }
}

bool DBHelper::execute(const std::string &query) {
  char *errMsg = nullptr;
  int result = sqlite3_exec(db_, query.c_str(), nullptr, nullptr, &errMsg);

  if (result != SQLITE_OK) {
    std::string error = "SQL error: " + std::string(errMsg);
    sqlite3_free(errMsg);
    throw std::runtime_error(error);
  }

  return true;
}

int DBHelper::executeWithReturn(const std::string &query) {
  sqlite3_stmt *stmt;
  int id = -1;

  if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare statement");
  }

  if (sqlite3_step(stmt) == SQLITE_ROW) {
    id = sqlite3_column_int(stmt, 0);
  }

  sqlite3_finalize(stmt);
  return id;
}

int DBHelper::SaveTranscriptionResult(const std::string &transcriptTitle,
                                      const std::string &result) {
  if (result.empty()) {
    return -1; // Nothing to save
  }

  // Query should return the newly created transcription ID
  std::string query =
      "INSERT INTO transcriptions (transcript_title, result) VALUES ('" +
      (std::string)sqlite3_mprintf("%q", transcriptTitle.c_str()) + "', '" +
      (std::string)sqlite3_mprintf("%q", result.c_str()) + "') RETURNING id;";

  try {
    return executeWithReturn(query);
  } catch (const std::runtime_error &e) {
    throw std::runtime_error("Failed to save transcription result: " +
                             std::string(e.what()));
  }
}

bool DBHelper::createDB(const std::string &dbPath) {
  if (db_) {
    return true; // Database already exists
  }

  if (sqlite3_open(dbPath.c_str(), &db_) != SQLITE_OK) {
    throw std::runtime_error("Failed to create database: " + dbPath);
  }

  // Create the transcriptions table if it doesn't exist
  std::string createTableQuery =
      "CREATE TABLE IF NOT EXISTS transcriptions ("
      "id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "result TEXT NOT NULL, "
      "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);";

  try {
    return execute(createTableQuery);
  } catch (const std::runtime_error &e) {
    throw std::runtime_error("Failed to create transcriptions table: " +
                             std::string(e.what()));
  }
}

bool DBHelper::SaveSummary(int transcriptId, const std::string &summary,
                           int tokens, int inferenceTimeMs) {
  if (summary.empty()) {
    return false; // Nothing to save
  }

  std::string query =
      "INSERT INTO summaries (transcript_id, summary_text, tokens_generated, "
      "inference_time_ms) VALUES (" +
      std::to_string(transcriptId) + ", '" +
      (std::string)sqlite3_mprintf("%q", summary.c_str()) + "', " +
      std::to_string(tokens) + ", " + std::to_string(inferenceTimeMs) + ");";

  try {
    return execute(query);
  } catch (const std::runtime_error &e) {
    throw std::runtime_error("Failed to save summary: " +
                             std::string(e.what()));
  }
}

std::unordered_map<int, std::string> DBHelper::GetAllTranscriptions() {
  std::unordered_map<int, std::string> transcriptions;
  std::string query = "SELECT id, transcript_title FROM transcriptions;";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare statement");
  }

  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *idText = sqlite3_column_text(stmt, 0);
    const unsigned char *titleText = sqlite3_column_text(stmt, 1);
    if (idText && titleText) {
      int id = std::stoi(reinterpret_cast<const char *>(idText));
      std::string title =
          std::string(reinterpret_cast<const char *>(titleText));
      transcriptions[id] = title;
    }
  }

  sqlite3_finalize(stmt);
  return transcriptions;
}

std::string DBHelper::GetTranscriptionById(int id) {
  std::string query =
      "SELECT result FROM transcriptions WHERE id = " + std::to_string(id) +
      ";";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare statement");
  }

  std::string result;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *text = sqlite3_column_text(stmt, 0);
    if (text) {
      result = std::string(reinterpret_cast<const char *>(text));
    }
  }

  sqlite3_finalize(stmt);
  return result;
}

std::string DBHelper::GetSummaryByTranscriptionId(int transcriptId) {
  std::string query = "SELECT summary FROM summaries WHERE transcript_id = " +
                      std::to_string(transcriptId) + ";";

  sqlite3_stmt *stmt;
  if (sqlite3_prepare_v2(db_, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    throw std::runtime_error("Failed to prepare statement");
  }

  std::string summary;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const unsigned char *text = sqlite3_column_text(stmt, 0);
    if (text) {
      summary = std::string(reinterpret_cast<const char *>(text));
    }
  }

  sqlite3_finalize(stmt);
  return summary;
}

bool DBHelper::DeleteTranscriptionById(int id) {
  std::string query =
      "DELETE FROM transcriptions WHERE id = " + std::to_string(id) + ";";

  std::string summaryDeleteQuery =
      "DELETE FROM summaries WHERE transcript_id = " + std::to_string(id) + ";";

  try {
    return execute(query) && execute(summaryDeleteQuery);
  } catch (const std::runtime_error &e) {
    throw std::runtime_error("Failed to delete transcription: " +
                             std::string(e.what()));
  }
}
