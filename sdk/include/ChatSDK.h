#pragma once
#include "LLMManager.h"
#include "SessionManager.h"
#include "common.h"

namespace ai_chat_sdk {
    class ChatSDK {
        private:
            bool _initialized = false; // 是否初始化
            std::unordered_map<std::string, std::shared_ptr<Config>> _modelConfigs;
            LLMManager _llmManager;
            SessionManager _sessionManager;

        public:
            // 初始化模型
            bool initModels(const std::vector<std::shared_ptr<Config>>& configs);
            // 创建会话
            std::string createSession(const std::string &modelName);
            // 获取指定会话
            std::shared_ptr<Session> getSession(const std::string& sessionId);
            // 获取所有会话列表
            std::vector<std::string> getSessionsLists() const;
            // 删除指定会话
            bool deleteSession(const std::string& sessionId);
            // 获取可用的模型信息
            std::vector<ModelInfo> getAvailableModels() const;
            // 发送消息给指定模型
            std::string sendMessage(const std::string &sessionId, const std::string& message);
            // 发送流式消息给指定模型
            std::string sendMessageStream(const std::string &sessionId, const std::string& message, std::function<void(const std::string&, bool)>& callback);

        private:
            // 注册支持的模型提供器
            void registerAllProvider(const std::vector<std::shared_ptr<Config>>& configs);
            // 初始化所支持的模型
            void initProviders(const std::vector<std::shared_ptr<Config>>& configs);
            // API初始化
            bool initAPIModelProvider(const std::string& modelName, const std::shared_ptr<APIConfig>& apiConfig);
            // Ollama初始化
            bool initOllamaModelProvider(const std::string& modelName, const std::shared_ptr<OllamaConfig>& ollamaConfig);
    };
}
