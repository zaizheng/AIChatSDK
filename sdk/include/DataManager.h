#pragma once

#include "common.h"
#include <string>
#include <memory>
#include <sqlite3.h>
#include <mutex>

namespace ai_chat_sdk {
    class DataManager {
    private:
        sqlite3 *_db = nullptr;
        std::string _dbName;
        mutable std::mutex _mutex;

    private:
        // 初始化数据库
        bool initDatabase();
        // 执行SQL语句
        bool executeSQL(const std::string& sql);
      
    public:
        DataManager(const std::string& dbName);
        ~DataManager();
        // session相关操作
        // 插入新会话
        bool insertSession(const Session &session);
        // 获取会话信息
        std::shared_ptr<Session> getSessionInfo(const std::string &sessionId) const;
        // 更新会话时间戳
        bool updateSessionTimestamp(const std::string &sessionId);
        // 删除会话
        bool deleteSession(const std::string &sessionId);
        // 获取所有会话ID
        std::vector<std::string> getAllSessionIds() const;
        // 获取所有会话信息
        std::vector<std::shared_ptr<Session>> getAllSessions() const;
        // 获取会话总数
        size_t getSessionCount() const;
        // 删除所有会话
        bool deleteAllSessions();

        // Message相关操作
        // 插入新消息， 并更新会话时间戳
        bool insertMessage(const std::string &sessionId, const Message &message);
        // 获取会话中所有消息
        std::vector<Message> getSessionMessages(const std::string &sessionId) const;
        // 删除指定会话的所有消息
        bool deleteSessionMessages(const std::string &sessionId);
    };
}
