#include "../include/ChatSDK.h"
#include "../include/DeepSeekProvider.h"
#include "../include/ChatGPTProvider.h"
#include "../include/util/myLog.h"
#include "../include/OllamaLLMProvider.h"
#include <memory>
#include <unordered_set>


namespace ai_chat_sdk {
    // 初始化模型
    bool ChatSDK::initModels(const std::vector<std::shared_ptr<Config>>& configs) {
        registerAllProvider(configs);
        initProviders(configs);
        _initialized = true;
        return true;
    }
        
    void ChatSDK::registerAllProvider(const std::vector<std::shared_ptr<Config>> &configs) {
        // deepseek
        if (!_llmManager.isModelAvailable("deepseek-chat")) {
            auto deepseekProvider = std::make_unique<DeepSeekProvider>();
            _llmManager.registerProvider("deepseek-chat",
                                         std::move(deepseekProvider));
            INFO("DeepSeek Provider注册成功");
        }
        // chatgpt
        if (!_llmManager.isModelAvailable("gpt-4o-mini")) {
            auto chatgptProvider = std::make_unique<ChatGPTProvider>();
            _llmManager.registerProvider("gpt-4o-mini",
                                         std::move(chatgptProvider));
            INFO("ChatGPT Provider注册成功");
        }
        // ollama
        std::unordered_set<std::string> modelNames;
        for (const auto &config : configs) {
            auto ollamaConfig = std::dynamic_pointer_cast<OllamaConfig>(config);
            if (ollamaConfig) {
                auto modelName = ollamaConfig->_modelName;
                if (modelNames.find(modelName) == modelNames.end()) {
                    modelNames.insert(modelName);
                    if (!_llmManager.isModelAvailable(modelName)) {
                        _llmManager.registerProvider(modelName, std::make_unique<OllamaLLMProvider>());
                        INFO("Ollama Provider注册成功");
                    }
                }
            }
        }
    }
    // 初始化所支持的模型
    void ChatSDK::initProviders(const std::vector<std::shared_ptr<Config>>& configs) {
        for (const auto &config : configs) {
            if (auto apiconfig = std::dynamic_pointer_cast<APIConfig>(config)) {
                if (apiconfig->_modelName == "deepseek-chat" ||
                  apiconfig->_modelName == "gpt-4o-mini") {
                    initAPIModelProvider(apiconfig->_modelName, apiconfig);
                } else {
                    ERR("ChatSDK初始化Provider失败: 模型不支持: {}", apiconfig->_modelName);
                }
            } else if (auto ollamaConfig = std::dynamic_pointer_cast<OllamaConfig>(config)) {
                initOllamaModelProvider(ollamaConfig->_modelName, ollamaConfig);
            } else {
                ERR("ChatSDK初始化Provider失败: 配置不支持: {}", config->_modelName);
            }
        }
    }
    // API初始化
    bool ChatSDK::initAPIModelProvider(const std::string& modelName, const std::shared_ptr<APIConfig>& apiConfig) {
        if (modelName.empty()) {
            ERR("初始化API模型失败: modelName为空");
            return false;
        }
        if (!apiConfig || apiConfig->_apiKey.empty()) {
            ERR("初始化API模型失败: apiConfig为空或API Key为空");
            return false;
        }
        if (_llmManager.isModelAvailable(modelName)) {
            INFO("API模型已可用: {}", modelName);
            return true;
        }
        std::map<std::string, std::string> modelParams;
        modelParams["api_key"] = apiConfig->_apiKey;
        if (!_llmManager.initModel(modelName, modelParams)) {
            ERR("初始化API模型失败: {}", modelName);
            return false;
        }
        _modelConfigs[modelName] = apiConfig;
        INFO("API模型初始化成功: {}", modelName);
        return true;
    }
    // Ollama初始化
    bool ChatSDK::initOllamaModelProvider(const std::string& modelName, const std::shared_ptr<OllamaConfig>& ollamaConfig) {
        if (modelName.empty()) {
            ERR("初始化Ollama模型失败: modelName为空");
            return false;
        }
        if (!ollamaConfig || ollamaConfig->_endpoint.empty()) {
            ERR("初始化Ollama模型失败: ollamaConfig为空或endpoint为空");
            return false;
        }
        if (_llmManager.isModelAvailable(modelName)) {
            INFO("Ollama模型已可用: {}", modelName);
            return true;
        }
        std::map<std::string, std::string> modelParams;
        modelParams["model_name"] = ollamaConfig->_modelName;
        modelParams["model_desc"] = ollamaConfig->_modelDesc;
        modelParams["endpoint"] = ollamaConfig->_endpoint;
        if (!_llmManager.initModel(modelName, modelParams)) {
            ERR("初始化Ollama模型失败: {}", modelName);
            return false;
        }
        _modelConfigs[modelName] = ollamaConfig;
        INFO("Ollama模型初始化成功: {}", modelName);
        return true;
    }
    // 创建会话
    std::string ChatSDK::createSession(const std::string &modelName) {
        if (!_initialized) {
            ERR("创建会话失败: ChatSDK未初始化");
            return "";
        }
        auto sessionId = _sessionManager.createSession(modelName);
        if (sessionId.empty()) {
            ERR("创建会话失败: {}", modelName);
            return "";
        }
        INFO("创建会话成功: {}", sessionId);
        return sessionId;
    }
    // 获取指定会话
    std::shared_ptr<Session> ChatSDK::getSession(const std::string& sessionId) {
        if (!_initialized) {
            ERR("获取会话失败: ChatSDK未初始化");
            return nullptr;
        }
        auto session = _sessionManager.getSession(sessionId);
        if (!session) {
            ERR("获取会话失败: 会话不存在: {}", sessionId);
            return nullptr;
        }
        INFO("获取会话成功: {}", sessionId);
        return session;
    }
    // 获取所有会话列表
    std::vector<std::string> ChatSDK::getSessionsLists() const {
        if (!_initialized) {
            ERR("获取会话列表失败: ChatSDK未初始化");
            return {};
        }
        return _sessionManager.getSessionList();
    }
    // 删除会话
    bool ChatSDK::deleteSession(const std::string& sessionId) {
        if (!_initialized) {
            ERR("删除会话失败: ChatSDK未初始化");
            return false;
        }
        if (!_sessionManager.deleteSession(sessionId)) {
            ERR("删除会话失败: 会话不存在: {}", sessionId);
            return false;
        }
        INFO("删除会话成功: {}", sessionId);
        return true;
    }
    // 获取可用的模型信息
    std::vector<ModelInfo> ChatSDK::getAvailableModels() const {
        return _llmManager.getAvailableModels();
    }
    // 发送消息给指定模型
    std::string ChatSDK::sendMessage(const std::string &sessionId, const std::string& message) {
        if (!_initialized) {
            ERR("发送消息失败: ChatSDK未初始化");
            return "";
        }
        auto session = getSession(sessionId);
        if (!session) {
            ERR("发送消息失败: 会话不存在: {}", sessionId);
            return "";
        }
        Message userMessage("user", message);
        _sessionManager.addMessage(sessionId, userMessage);
        auto historyMessages = _sessionManager.getHistoryMessages(sessionId);

        auto it = _modelConfigs.find(session->_modelName);
        if (it == _modelConfigs.end()) {
            ERR("发送消息失败: 模型配置未找到: {}", session->_modelName);
            return "";
        }
        std::map<std::string, std::string> requestParams;
        requestParams["temperature"] = std::to_string(it->second->_temperature);
        requestParams["max_tokens"] = std::to_string(it->second->_maxTokens);

        auto response = _llmManager.sendMessage(it->second->_modelName,
                                                historyMessages, requestParams);
        if (response.empty()) {
            ERR("发送消息失败: 模型响应为空: {}", session->_modelName);
            return "";
        }

        Message assistantMessage("assistant", response);
        _sessionManager.addMessage(sessionId, assistantMessage);
        _sessionManager.updateSessionTimestamp(sessionId);
        INFO("发送消息成功: {}", session->_modelName);
        return response;
    }
    // 发送流式消息给指定模型
    std::string ChatSDK::sendMessageStream(const std::string &sessionId, const std::string& message, std::function<void(const std::string&, bool)> callback) {
        if (!_initialized) {
            ERR("发送流式消息失败: ChatSDK未初始化");
            return "";
        }
        auto session = getSession(sessionId);
        if (!session) {
            ERR("发送流式消息失败: 会话不存在: {}", sessionId);
            return "";
        }
        Message userMessage("user", message);
        _sessionManager.addMessage(sessionId, userMessage);
        auto historyMessages = _sessionManager.getHistoryMessages(sessionId);
        
        auto it = _modelConfigs.find(session->_modelName);
        if (it == _modelConfigs.end()) {
            ERR("发送流式消息失败: 模型配置未找到: {}", session->_modelName);
            return "";
        }
        std::map<std::string, std::string> requestParams;
        requestParams["temperature"] = std::to_string(it->second->_temperature);
        requestParams["max_tokens"] = std::to_string(it->second->_maxTokens);
        
        auto response = _llmManager.sendMessageStream(it->second->_modelName,
                                                historyMessages, requestParams, callback);
        if (response.empty()) {
            ERR("发送流式消息失败: 模型响应为空: {}", session->_modelName);
            return "";
        }
        Message assistantMessage("assistant", response);
        _sessionManager.addMessage(sessionId, assistantMessage);
        _sessionManager.updateSessionTimestamp(sessionId);
        INFO("发送流式消息成功: {}", session->_modelName);
        return response;
    }
}
