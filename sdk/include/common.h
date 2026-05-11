#pragma once
#include <ctime>
#include <string>
#include <vector>

namespace ai_chat_sdk {
    struct Message {
        std::string _messageId;  // 消息ID
        std::string _content;    // 消息内容
        std::string _role;       // 消息角色
        std::time_t _timestamp;  // 消息时间戳

        Message(const std::string& role = "", const std::string& content = "")
            : _role(role), _content(content), _timestamp(0) {}
    };
    
    struct Config {
        std::string _modelName;  // 模型名称
        double _temperature = 0.7;     // 温度参数，用于控制生成文本的随机性
        int _maxTokens = 2048;         // 最大生成令牌数，用于控制生成文本的长度

        virtual ~Config() = default;
    };

    struct APIConfig : public Config {
        std::string _apiKey;  // API密钥
    };

    struct OllamaConfig : public Config {
        std::string _modelName;
        std::string _modelDesc;
        std::string _endpoint;
    };

    // LLM信息
    struct ModelInfo {
        std::string _modelName;     // 模型名称
        std::string _modelDesc;      // 模型描述
        std::string _provider;      // 模型提供方
        std::string _endpoint;      // base url
        bool _isAvailable = false;  // 是否可用

        ModelInfo(const std::string& modelName = "", const std::string& modelDesc = "", const std::string& provider = "", const std::string& endpoint = "")
            : _modelName(modelName), _modelDesc(modelDesc), _provider(provider), _endpoint(endpoint) {}
    };

    // 会话信息
    struct Session {
        std::string _sessionId;          // 会话ID
        std::string _modelName;          // 模型名称
        std::vector<Message> _messages;  // 消息队列，用于存储会话中的消息
        std::time_t _createdAt;          // 创建时间
        std::time_t _updatedAt;          // 更新时间

        Session(const std::string& modelName = "") : _modelName(modelName) {}
    };
}  // namespace ai_chat_sdk
