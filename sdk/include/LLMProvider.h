#pragma once
#include <map>
#include <string>
#include <vector>
#include <functional>
#include "common.h"

namespace ai_chat_sdk {
    class LLMProvider {
    protected:
        bool _isAvailable = false;
        std::string _apiKey;
        std::string _endpoint;
    
    public:
        virtual bool initModel(const std::map<std::string, std::string>& modelConfig) = 0;
        virtual bool isAvailable() const = 0;
        virtual std::string getModelName() const = 0;
        virtual std::string getModelDes() const = 0;
        // 全量返回
        virtual std::string sendMessage(const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams) = 0;
        // 流式返回
        virtual std::string sendMessageStream(const std::vector<Message> &messages,
                                       const std::map<std::string, std::string> &requestParams,
                                       std::function<void(const std::string&, bool)> callback) = 0; // callback：对模型返回的流式结果进行处理的回调函数
   };
}  // namespace ai_chat_sdk