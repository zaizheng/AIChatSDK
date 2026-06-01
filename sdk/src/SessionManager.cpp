#include "../include/SessionManager.h"
#include "../include/DataManager.h"
#include "../include/util/myLog.h"
#include <iomanip>
#include <sstream>



namespace ai_chat_sdk {
    SessionManager::SessionManager(const std::string &dbName) : _dataManager(dbName) {
        auto sess = _dataManager.getAllSessions();
        for (auto &session : sess) {
            _sessions[session->_sessionId] = session;
        }
    }
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
        os<<"message_"<<time<<"_"<<std::setw(8)<<std::setfill('0')<<messageCounter;
        return os.str();
    }
    // 创建会话
    std::string SessionManager::createSession(const std::string &modelName) {
        _mutex.lock();
        std::string sessionId = generateSessionId();
        auto session = std::make_shared<Session>(modelName);
        session->_sessionId = sessionId;
        session->_createdAt = std::time(nullptr);
        session->_updatedAt = session->_createdAt;
        _sessions[sessionId] = session;
        _mutex.unlock();
        _dataManager.insertSession(*session);
        return sessionId;
    }
    // 获取会话信息
    std::shared_ptr<Session> SessionManager::getSession(const std::string &sessionId) {
        _mutex.lock();
        // 内存中查找
        auto it = _sessions.find(sessionId);
        if (it != _sessions.end()) {
            _mutex.unlock();
            it->second->_messages = _dataManager.getSessionMessages(sessionId);
            return it->second;
        }
        _mutex.unlock();
        // 数据库中查找
        auto session = _dataManager.getSession(sessionId);
        if (session) {
            _mutex.lock();
            auto it = _sessions.find(sessionId);
            if (it == _sessions.end()) {
                _sessions[sessionId] = session;
            }
            _mutex.unlock();
            session->_messages = _dataManager.getSessionMessages(sessionId);
            return session;
        }
        WARN("会话未找到: sessionId {}", sessionId);
        return nullptr;
    }
    // 向某个会话中添加信息
    bool SessionManager::addMessage(const std::string &sessionId, const Message &message) {
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end()) {
            _mutex.unlock();
            return false;
        }
        Message msg(message._role, message._content);
        msg._messageId = generateMessageId(it->second->_messages.size());
        msg._timestamp = std::time(nullptr);
        INFO("消息信息: content {}  timestamp {}", msg._content, msg._timestamp);
        it->second->_messages.push_back(msg);
        it->second->_updatedAt = std::time(nullptr);
        INFO("添加消息成功: sessionId {}, message.content {}", sessionId, msg._content);
        _mutex.unlock();
        _dataManager.insertMessage(sessionId, msg);
        return true;
    }
    // 获取某个会话的所有历史消息
    std::vector<Message> SessionManager::getHistoryMessages(const std::string &sessionId) const {
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if (it != _sessions.end()) {
            _mutex.unlock();
            return it->second->_messages;
        }
        _mutex.unlock();
        return _dataManager.getSessionMessages(sessionId);
    }
    // 更新会话时间戳
    void SessionManager::updateSessionTimestamp(const std::string &sessionId) {
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if (it != _sessions.end()) {
            it->second->_updatedAt = std::time(nullptr);
        }
        _mutex.unlock();
        _dataManager.updateSessionTimestamp(sessionId, std::time(nullptr));
        INFO("更新会话时间戳: sessionId {}", sessionId);
    }
    // 获取会话列表
    std::vector<std::string> SessionManager::getSessionList() const {
        auto session = _dataManager.getAllSessions();
        std::lock_guard<std::mutex> lock(_mutex);
        // 按时间戳排序会话列表
        std::vector<std::pair<std::time_t, std::shared_ptr<Session>>> temp;
        temp.reserve(session.size());
        for (const auto &it : _sessions) {
            temp.emplace_back(it.second->_updatedAt, it.second);
        }
        for (const auto &it : session) {
            if (_sessions.find(it->_sessionId) == _sessions.end()) {
                temp.emplace_back(it->_updatedAt, it);
            }
        }
        std::sort(temp.begin(), temp.end(), [](const auto &a, const auto &b) {
            return a.first > b.first;
        });
        std::vector<std::string> sessionIds;
        sessionIds.reserve(_sessions.size());
        for (const auto &it : temp) {
            sessionIds.push_back(it.second->_sessionId);
        }
        return sessionIds;
    }
    // 删除会话
    bool SessionManager::deleteSession(const std::string &sessionId) {
        _mutex.lock();
        auto it = _sessions.find(sessionId);
        if (it == _sessions.end()) {
            _mutex.unlock();
            return false;
        }
        _sessions.erase(it);
        _mutex.unlock();
        _dataManager.deleteSession(sessionId);
        return true;
    }
    // 清空所有会话
    void SessionManager::clearAllSessions() {
        _mutex.lock();
        _sessions.clear();
        _mutex.unlock();
        _dataManager.deleteAllSessions();
    }
    // 获取会话总数
    size_t SessionManager::getSessionCount() const {
        std::lock_guard<std::mutex> lock(_mutex);
        return _sessions.size();
    }
}
