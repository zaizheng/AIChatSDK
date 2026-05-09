#pragma once

#include <unordered_map>
#include <mutex>
#include <atomic>
#include <memory>
#include "common.h"



namespace ai_chat_sdk {
    class SessionManager {
    private:
        // key:会话ID, value:会话信息
        std::unordered_map<std::string, std::shared_ptr<Session>> _sessions;
        mutable std::mutex _mutex;
        std::atomic<int64_t> _sessionCounter = {0};

    private:
        // 生成会话ID
        std::string generateSessionId();
        // 生成消息ID
        std::string generateMessageId(size_t messageCounter);
        
    public:
        // 创建会话
        std::string createSession(const std::string &modelName);
        // 获取会话信息
        std::shared_ptr<Session>getSessionInfo(const std::string &sessionId) const;
        // 向某个会话中添加信息
        bool addMessage(const std::string &sessionId, const Message &message);
        // 获取某个会话的所有历史消息
        std::vector<Message> getMessages(const std::string &sessionId) const;
        // 更新会话时间戳
        void updateSessionTimestamp(const std::string &sessionId);
        // 获取会话列表
        std::vector<std::string> getSessionList() const;
        // 删除会话
        bool deleteSession(const std::string &sessionId);
        // 清空所有会话
        void clearAllSessions();
        // 获取会话总数
        size_t getSessionCount() const;
    };
}
