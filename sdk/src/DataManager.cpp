#include "../include/DataManager.h"
#include "../include/util/myLog.h"


namespace ai_chat_sdk {
    DataManager::DataManager(const std::string &dbName)
        : _dbName(dbName), _db(nullptr) {
        // 创建并打开数据库
        int rc = sqlite3_open(dbName.c_str(), &_db);
        if (rc != SQLITE_OK) {
            ERR("DataManager::DataManager: open database failed: {}", sqlite3_errmsg(_db));
        }
        INFO("DataManager::DataManager: open database success: {}", dbName);
        // 初始化数据库
        if (!initDatabase()) {
            sqlite3_close(_db);
            _db = nullptr;
            ERR("DataManager::DataManager: initDatabase failed");
        }
    }
    DataManager::~DataManager() {
        if (_db) {
            sqlite3_close(_db);
            _db = nullptr;
        }
    }
    // 初始化数据库
    bool DataManager::initDatabase() {
        // 创建会话表
        std::string createSessionTableSql = R"(
            CREATE TABLE IF NOT EXISTS sessions (
                session_id TEXT PRIMARY KEY,
                model_name TEXT NOT NULL,
                create_time INTEGER NOT NULL,
                update_time INTEGER NOT NULL
            );
        )";
        if (!executeSQL(createSessionTableSql)) {
            return false;
        }
        // 创建信息表
        std::string createMessageTableSql = R"(
            CREATE TABLE IF NOT EXISTS messages (
                message_id TEXT PRIMARY KEY,
                session_id TEXT NOT NULL,
                role TEXT NOT NULL,
                content TEXT NOT NULL,
                timestamp INTEGER NOT NULL,
                FOREIGN KEY (session_id) REFERENCES sessions (session_id) ON DELETE CASCADE
            );
        )";
        if (!executeSQL(createMessageTableSql)) {
            return false;
        }
        return true;
    }
    // 执行SQL语句
    bool DataManager::executeSQL(const std::string &sql) {
        if (!_db) {
            ERR("DataManager::executeSQL: database is null");
            return false;
        }
        int rc = sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::executeSQL: execute SQL failed: {}", sqlite3_errmsg(_db));
            return false;
        }
        return true;
    }
    // Session相关操作
    // 插入新会话
    bool DataManager::insertSession(const Session &session) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string insertSessionSql = R"(
            INSERT INTO sessions (session_id, model_name, create_time, update_time)
            VALUES (?, ?, ?, ?);
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, insertSessionSql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::insertSession: prepare statement failed: {}", sqlite3_errmsg(_db));
            return false;
        }
        // bind
        sqlite3_bind_text(stmt, 1, session._sessionId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, session._modelName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(session._createdAt));
        sqlite3_bind_int64(stmt, 4, static_cast<int64_t>(session._updatedAt));
        // execute
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            ERR("DataManager::insertSession: step statement failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        INFO("DataManager::insertSession: insert session success: {}", session._sessionId);
        return true;
    }
    // 获取会话信息
    std::shared_ptr<Session> DataManager::getSession(const std::string &sessionId) const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string selectSessionSql = R"(
            SELECT model_name, create_time, update_time FROM sessions WHERE session_id = ?;
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, selectSessionSql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::getSessionInfo: prepare statement failed: {}", sqlite3_errmsg(_db));
            return nullptr;
        }
        // bind
        sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
        // execute
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            ERR("DataManager::getSessionInfo: step statement failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return nullptr;
        }
        // fetch
        std::string modelName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        int64_t createdAt = sqlite3_column_int64(stmt, 1);
        int64_t updatedAt = sqlite3_column_int64(stmt, 2);

        auto session = std::make_shared<Session>(modelName);
        session->_createdAt = static_cast<std::time_t>(createdAt);
        session->_updatedAt = static_cast<std::time_t>(updatedAt);
        session->_sessionId = sessionId;
        
        sqlite3_finalize(stmt);
        session->_messages = getSessionMessages(sessionId);
        INFO("DataManager::getSessionInfo: get session success: {}", sessionId);
        return session;
    }
    // 更新会话时间戳
    bool DataManager::updateSessionTimestamp(const std::string &sessionId, std::time_t timestamp) {    
        std::lock_guard<std::mutex> lock(_mutex);
        std::string updateSessionSql = R"(
            UPDATE sessions SET update_time = ? WHERE session_id = ?;
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, updateSessionSql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::updateSessionTimestamp: prepare statement failed: {}", sqlite3_errmsg(_db));
            return false;
        }
        // bind
        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(timestamp));
        sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);
        // execute
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            ERR("DataManager::updateSessionTimestamp: step statement failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        INFO("DataManager::updateSessionTimestamp: update session timestamp success: {}", sessionId);
        return true;
    }
    // 删除会话
    bool DataManager::deleteSession(const std::string &sessionId) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string deleteSessionSql = R"(
            DELETE FROM sessions WHERE session_id = ?;
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, deleteSessionSql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::deleteSession: prepare statement failed: {}", sqlite3_errmsg(_db));
            return false;
        }
        // bind
        sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
        // execute
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            ERR("DataManager::deleteSession: step statement failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        INFO("DataManager::deleteSession: delete session success: {}", sessionId);
        return true;
    }
    // 获取所有会话ID
    std::vector<std::string> DataManager::getAllSessionIds() const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string selectSessionIdsSql = R"(
            SELECT session_id FROM sessions ORDER BY update_time DESC;
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, selectSessionIdsSql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::getAllSessionIds: prepare statement failed: {}", sqlite3_errmsg(_db));
            return {};
        }
        std::vector<std::string> sessionIds;
        rc = sqlite3_step(stmt);
        while (rc == SQLITE_ROW) {
            sessionIds.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
        sqlite3_finalize(stmt);
        INFO("DataManager::getAllSessionIds: get all session ids success, total {}", sessionIds.size());
        return sessionIds;
    }
    // 获取所有会话信息, 按更新时间降序排序
    std::vector<std::shared_ptr<Session>> DataManager::getAllSessions() const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string selectSQL = R"(
            SELECT session_id, model_name, create_time, update_time FROM sessions ORDER BY update_time DESC;
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::getAllSessions: prepare statement failed: {}", sqlite3_errmsg(_db));
            return {};
        }
        std::vector<std::shared_ptr<Session>> sessions;
        rc = sqlite3_step(stmt);
        while (rc == SQLITE_ROW) {
            std::string sessionId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            std::string modelName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int64_t createdAt = sqlite3_column_int64(stmt, 2);
            int64_t updatedAt = sqlite3_column_int64(stmt, 3);
            auto session = std::make_shared<Session>(modelName);
            session->_sessionId = sessionId;
            session->_createdAt = static_cast<std::time_t>(createdAt);
            session->_updatedAt = static_cast<std::time_t>(updatedAt);
            sessions.push_back(session);
        }
        sqlite3_finalize(stmt);
        INFO("DataManager::getAllSessions: get all sessions success, total {}", sessions.size());
        return sessions;
    }
    // 获取会话总数
    size_t DataManager::getSessionCount() const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string selectSQL = R"(
            SELECT COUNT(*) FROM sessions;
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::getSessionCount: prepare statement failed: {}", sqlite3_errmsg(_db));
            return 0;
        }
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_ROW) {
            ERR("DataManager::getSessionCount: step statement failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return 0;
        }
        size_t count = sqlite3_column_int64(stmt, 0);
        sqlite3_finalize(stmt);
        INFO("DataManager::getSessionCount: get session count success: {}", count);
        return count;
    }
    // 删除所有会话
    bool DataManager::deleteAllSessions() {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string deleteAllSessionsSql = R"(
            DELETE FROM sessions;
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, deleteAllSessionsSql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::deleteAllSessions: prepare statement failed: {}", sqlite3_errmsg(_db));
            return false;
        }
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            ERR("DataManager::deleteAllSessions: step statement failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        INFO("DataManager::deleteAllSessions: delete all sessions success");
        return true;
    }
    //////////////////////////////////////////////////////////////////////////////////////
    // Message相关操作
    // 插入新消息， 并更新会话时间戳
    bool DataManager::insertMessage(const std::string &sessionId, const Message &message) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string insertMessageSql = R"(
            INSERT INTO messages (message_id, session_id, role, content, timestamp)
            VALUES (?, ?, ?, ?, ?);
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, insertMessageSql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::insertMessage: prepare statement failed: {}", sqlite3_errmsg(_db));
            return false;
        }
        sqlite3_bind_text(stmt, 1, message._messageId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, message._role.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, message._content.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 5, static_cast<int64_t>(message._timestamp));

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            ERR("DataManager::insertMessage: step statement failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        // 同时更新session的update_time
        std::string updateSessionSql = R"(
            UPDATE sessions SET updatedAt = ? WHERE sessionId = ?
        )";
        sqlite3_stmt *updateStmt;
        rc = sqlite3_prepare_v2(_db, updateSessionSql.c_str(), -1, &updateStmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::insertMessage: prepare statement failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_bind_int64(updateStmt, 1, static_cast<int64_t>(message._timestamp));
        sqlite3_bind_text(updateStmt, 2, sessionId.c_str(), -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(updateStmt);
        if (rc != SQLITE_DONE) {
            ERR("DataManager::insertMessage: step statement failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        sqlite3_finalize(updateStmt);
        INFO("DataManager::insertMessage: insert message success, messageId {}", message._messageId);
        return true;
    }
    // 获取会话中所有消息
    std::vector<Message> DataManager::getSessionMessages(const std::string &sessionId) const {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string selectSQL = R"(
            SELECT message_id, role, content, timestamp FROM messages WHERE session_id = ?;
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, selectSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::getSessionMessages: prepare statement failed: {}", sqlite3_errmsg(_db));
            return {};
        }
        sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
        std::vector<Message> messages;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
          Message msg;
          msg._messageId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
          msg._role = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
          msg._content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
          msg._timestamp = static_cast<std::time_t>(sqlite3_column_int64(stmt, 3));
          messages.push_back(msg);
        }
        if (rc != SQLITE_DONE) {
            ERR("DataManager::getSessionMessages: step statement failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return {};
        }
        sqlite3_finalize(stmt);
        INFO("DataManager::getSessionMessages: get session messages success, sessionId {}", sessionId);
        return messages;
    }
    // 删除制定会话的历史消息
    bool DataManager::deleteSessionMessages(const std::string& sessionId) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string deleteSQL = R"(
            DELETE FROM messages WHERE session_id = ?;
        )";
        sqlite3_stmt *stmt;
        int rc = sqlite3_prepare_v2(_db, deleteSQL.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            ERR("DataManager::deleteSessionMessages: prepare statement failed: {}", sqlite3_errmsg(_db));
            return false;
        }
        sqlite3_bind_text(stmt, 1, sessionId.c_str(), -1, SQLITE_TRANSIENT);
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE) {
            ERR("DataManager::deleteSessionMessages: step statement failed: {}", sqlite3_errmsg(_db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        INFO("DataManager::deleteSessionMessages: delete session messages success, sessionId {}", sessionId);
        return true;
    }
} // namespace ai_chat_sdk