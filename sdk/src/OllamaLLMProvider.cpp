#include "../include/OllamaLLMProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <httplib.h>

namespace ai_chat_sdk {
bool OllamaLLMProvider::initModel(
    const std::map<std::string, std::string> &modelConfig) {
        // 初始化模型名称
        auto it = modelConfig.find("model_name");
        if (it == modelConfig.end()) {
            ERR("OllamaLLMProvider initModel model_name not found");
            return false;
        } else {
            _modelName = it->second;
        }
        // 初始化模型描述
        it = modelConfig.find("model_desc");
        if (it == modelConfig.end()) {
            ERR("OllamaLLMProvider initModel model_desc not found");
            return false;
        } else {
            _modelDesc = it->second;
        }
        // 初始化endpoint
        it = modelConfig.find("endpoint");
        if (it == modelConfig.end()) {
            ERR("OllamaLLMProvider initModel endpoint not found");
            return false;
        } else {
            _endpoint = it->second;
        }
        _isAvailable = true;
        return true;
    }
    bool OllamaLLMProvider::isAvailable() const { return _isAvailable; }
    std::string OllamaLLMProvider::getModelName() const { return _modelName; }
    std::string OllamaLLMProvider::getModelDesc() const { return _modelDesc; }
    // 全量返回
    std::string OllamaLLMProvider::sendMessage(
        const std::vector<Message> &messages,
        const std::map<std::string, std::string> &requestParams) {
        // 模型是否可用
        if (!_isAvailable) {
            ERR("OllamaLLMProvider sendMessage model not available");
            return "";
        }
        // 构建请求参数
        double temperature = 0.7;
        int maxTokens = 2048;
        if (requestParams.find("temperature") != requestParams.end()) {
            temperature = std::stod(requestParams.at("temperature"));
        }
        if (requestParams.find("max_tokens") != requestParams.end()) {
            maxTokens = std::stoi(requestParams.at("max_tokens"));
        }
        // 构建历史消息
        Json::Value messageArray(Json::arrayValue);
        for (const auto &msg : messages) {
            Json::Value message(Json::objectValue);
            message["role"] = msg._role;
            message["content"] = msg._content;
            messageArray.append(message);
        }
        // 构建请求体
        Json::Value options(Json::objectValue);
        options["temperature"] = temperature;
        options["num_ctx"] = maxTokens;
        Json::Value requestBodyJson(Json::objectValue);
        requestBodyJson["model"] = _modelName;
        requestBodyJson["messages"] = messageArray;
        requestBodyJson["options"] = options;
        requestBodyJson["stream"] = false;
        // 序列化请求体
        Json::StreamWriterBuilder writerBuilder;
        std::string requestBodyStr =
            Json::writeString(writerBuilder, requestBodyJson);
        // 创建http客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(60, 0);
        // 设置请求头
        httplib::Headers headers = {
            {"Content-Type", "application/json"},
        };
        // 发送POST请求
        auto response = client.Post("/api/chat", headers, requestBodyStr, "application/json");
        if (!response) {
            ERR("OllamaLLMProvider sendMessage response is null, error : {}", to_string(response.error()));
            return "";
        }
        INFO("OllamaLLMProvider sendMessage response : {}", response->body);
        INFO("OllamaLLMProvider sendMessage response status : {}", response->status);
        if (response->status != 200) {
            ERR("OllamaLLMProvider sendMessage response status not 200, status : {}", response->status);
            return "";
        }
        // 反序列化
        Json::Value responseBody;
        Json::CharReaderBuilder reader;
        std::string errors;
        std::istringstream responseStream(response->body);
        if(!Json::parseFromStream(reader, responseStream, &responseBody, &errors)){
            ERR("OllamaLLMProvider sendMessage failed to parse response body, errors: {}", errors);
            return "";
        }
        // 提取生成文本
        std::string modelResponse = "";
        if (responseBody.isMember("message") && responseBody["message"].isObject() && responseBody["message"].isMember("content")) {
            modelResponse = responseBody["message"]["content"].asString();
            INFO("OllamaLLMProvider sendMessage modelResponse : {}", modelResponse);
            return modelResponse;
        }
        // 处理其他情况
        ERR("OllamaLLMProvider sendMessage invalid response format");
        return "";
    }
    // 流式返回
    std::string OllamaLLMProvider::sendMessageStream(
        const std::vector<Message> &messages,
        const std::map<std::string, std::string> &requestParams,
        std::function<void(const std::string &, bool)> callback) {
        return "";
    }
}
