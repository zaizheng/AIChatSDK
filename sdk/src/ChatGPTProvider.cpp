#include "../include/ChatGPTProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <jsoncpp/json/reader.h>

namespace ai_chat_sdk {
    bool ChatGPTProvider::initModel(const std::map<std::string, std::string>& modelConfig) {
        auto it = modelConfig.find("api_key");
        if (it == modelConfig.end()) {
            ERR("ChatGPTProvider initModel api_key not found");
            return false;
        } else {
            _apiKey = it->second;
        }
        it = modelConfig.find("endpoint");
        if(it == modelConfig.end()){
            _endpoint = "https://api.openai.com";
        }else{
            _endpoint = it->second;
        }
        
        _isAvailable = true;
        INFO("ChatGPTProvider initModel success, endpoint: {}",_endpoint);
        return true;
    }

    bool ChatGPTProvider::isAvailable() const { return _isAvailable; }

    std::string ChatGPTProvider::getModelName() const { return "gpt-4o-mini"; }

    std::string ChatGPTProvider::getModelDes() const { return "OpenAI 推出的轻量级、高性价比模型，核心能力解决 GPT-4 Turbo，但更经济"; }

     // 全量返回
    std::string ChatGPTProvider::sendMessage(const std::vector<Message> &messages, const std::map<std::string, std::string> &requestParams) {
        return "";
    }
    // 流式返回
    std::string ChatGPTProvider::sendMessageStream(const std::vector<Message> &messages,
                                                   const std::map<std::string, std::string> &requestParams,
                                                   std::function<void(const std::string &, bool)> callback) {
        return "";
    }
}