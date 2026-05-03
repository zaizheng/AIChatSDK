#include "../include/DeepSeekProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <httplib.h>

namespace ai_chat_sdk {
    bool DeepSeekProvider::initModel(const std::map<std::string, std::string>& modelConfig) {
        auto it = modelConfig.find("api_key");
        if (it == modelConfig.end()) {
            ERR("DeepSeekProvider initModel api_key not found");
            return false;
        } else {
            _apiKey = it->second;
        }
        it = modelConfig.find("endpoint");
        if(it == modelConfig.end()){
            _endpoint = "https://api.deepseek.com";
        }else{
            _endpoint = it->second;
        }
        
        _isAvailable = true;
        INFO("DeepSeekProvider initModel success, endpoint: {}",_endpoint);
        return true;
    }

    bool DeepSeekProvider::isAvailable() const { return _isAvailable; }

    std::string DeepSeekProvider::getModelName() const { return "deepseek-chat"; }

    std::string DeepSeekProvider::getModelDes() const { return "深度求索"; }

    // 全量返回
    std::string DeepSeekProvider::sendMessage(const std::vector<Message> &messages,
                                              const std::map<std::string, std::string> &requestParams) {
        // 1、模型是否可用
        if(!isAvailable()){
            ERR("DeepSeekProvider sendMessage model not available");
            return "";
        }
        // 2、构造请求参数
        double temperature = 0.7;
        int maxTokens = 2048;
        if (requestParams.find("temperature") != requestParams.end()) {
            temperature = std::stod(requestParams.at("temperature"));
        }
        if (requestParams.find("max_tokens") != requestParams.end()) {
            maxTokens = std::stoi(requestParams.at("max_tokens"));
        }
        // 构造历史消息
        Json::Value messageArry(Json::arrayValue);
        for (const auto &msg : messages) {
            Json::Value messageObject;
            messageObject["role"] = msg._role;
            messageObject["content"] = msg._content;
            messageArry.append(messageObject);
        }
        // 3、构造请求体
        Json::Value requestBody;
        requestBody["model"] = getModelName();
        requestBody["messages"] = messageArry;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = maxTokens;
        // 4、序列化
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(builder, requestBody);
        INFO("DeepSeekProvider sendMessage requestBody: {}", requestBodyStr);
        // 5、使用cpp-httplib库构建客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(60, 0);
        // 设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            {"Content-Type", "application/json"}
        };
        // 6、发送POST请求
        auto resp = client.Post("/v1/chat/completions", headers, requestBodyStr, "application/json");
        if (!resp) {
            ERR("DeepSeekProvider sendMessage POST request failed");
            return "";
        }
        INFO("DeepSeekProvider sendMessage POST request success, status : {}",resp->status);
        INFO("DeepSeekProvider sendMessage POST request body : {}", resp->body);
        // 检测响应是否成功
        if (resp->status != 200) {
            return "";
        }
        // 7、解析响应体
        Json::Value responseBody;
        Json::CharReaderBuilder readerBuilder;
        std::string parseError;
        std::istringstream responseStream(resp->body);
        if (Json::parseFromStream(readerBuilder, responseStream, &responseBody, &parseError)) {
            if (responseBody.isMember("choices") && responseBody["choices"].isArray() && !responseBody["choices"].empty()) {
                auto choice = responseBody["choices"][0];
                if (choice.isMember("message") && choice["message"].isMember("content")) {
                    std::string replyContent = choice["message"]["content"].asString();
                    INFO("DeepSeekProvider response txt : {}", replyContent);
                    return replyContent;
                }
            }
        }
        // 8、json解析失败
        ERR("DeepSeekProvider sendMessage json parse failed, error : {}", parseError);
        return "deepseek response json parse failed";
    }
    // 流式返回TODO
    std::string DeepSeekProvider::sendMessageStream(const std::vector<Message> &messages,
                      const std::map<std::string, std::string> &requestParams,
                      std::function<void(std::string &, bool)> callback) {
        return "";
    }
} // namespace ai_chat_sdk