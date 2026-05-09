#include "../include/SessionManager.h"
#include "../include/util/myLog.h"
#include <iomanip>
#include <sstream>



namespace ai_chat_sdk {
    // 生成会话ID
    std::string SessionManager::generateSessionId() {
        _sessionCounter.fetch_add(1);
        std::time_t time = std::time(nullptr);
        std::ostringstream os;
        os<<"session_"<<time<<"_"<<std::setw(8)<<std::setfill('0')<<_sessionCounter;
        return os.str();
    }
    // 生成消息ID
    std::string SessionManager::generateMessageId(size_t messageCounter) {
        messageCounter++;
        std::time_t time = std::time(nullptr);
        std::ostringstream os;
        os<<"message_"<<time<<"_"<<std::setw(8)<<std::setfill('0')<<_sessionCounter;
        return os.str();
    }
    // 创建会话
    std::string SessionManager::createSession(const std::string &modelName) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::string sessionId = generateSessionId();
        auto session = std::make_shared<Session>(modelName);
        session->_sessionId = sessionId;
        _sessions[sessionId] = session;
        return sessionId;
    }
    // 获取会话信息
    std::shared_ptr<Session> SessionManager::getSessionInfo(const std::string &sessionId) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end()) {
            return nullptr;
        }
        return it->second;
    }
    // 向某个会话中添加信息
    bool SessionManager::addMessage(const std::string &sessionId, const Message &message) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end()) {
            return false;
        }
        Message msg(message._role, message._content);
        msg._messageId = generateMessageId(it->second->_messages.size());
        it->second->_messages.push_back(msg);
        it->second->_updatedAt = std::time(nullptr);
        INFO("SessionManager::addMessage: sessionId {}, message.content {}", sessionId, msg._content);
        return true;
    }
    // 获取某个会话的所有历史消息
    std::vector<Message> SessionManager::getMessages(const std::string &sessionId) const {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end()) {
            return {};
        }
        return it->second->_messages;
    }
    // 更新会话时间戳
    void SessionManager::updateSessionTimestamp(const std::string &sessionId) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end()) {
            return;
        }
        it->second->_updatedAt = std::time(nullptr);
        INFO("SessionManager::updateSessionTimestamp: sessionId {}", sessionId);
    }
    // 获取会话列表
    std::vector<std::string> SessionManager::getSessionList() const {
        std::lock_guard<std::mutex> lock(_mutex);
        // 按时间戳排序会话列表
        std::vector<std::pair<std::time_t, std::shared_ptr<Session>>> sessionTempList;
        for (auto &it : _sessions) {
            sessionTempList.push_back({it.second->_updatedAt, it.second});
        }
        std::sort(sessionTempList.begin(), sessionTempList.end(), [](const auto &a, const auto &b) {
            return a.first > b.first;
        });
        std::vector<std::string> sessionIds;
        sessionIds.reserve(_sessions.size());
        for (auto &it : sessionTempList) {
            sessionIds.push_back(it.second->_sessionId);
        }
        return sessionIds;
    }
    // 删除会话
    bool SessionManager::deleteSession(const std::string &sessionId) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end()) {
            return false;
        }
        _sessions.erase(it);
        return true;
    }
    // 清空所有会话
    void SessionManager::clearAllSessions() {
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions.clear();
    }
    // 获取会话总数
    size_t SessionManager::getSessionCount() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _sessions.size();
    }
}
