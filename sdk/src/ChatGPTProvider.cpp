#include "../include/ChatGPTProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <sstream>

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

    std::string ChatGPTProvider::getModelDesc() const { return "OpenAI 推出的轻量级、高性价比模型，核心能力解决 GPT-4 Turbo，但更经济"; }

     // 全量返回
    std::string ChatGPTProvider::sendMessage(
        const std::vector<Message> &messages,
        const std::map<std::string, std::string> &requestParams) {
        // 1、模型是否可用
        if(!isAvailable()){
            ERR("ChatGPTProvider sendMessage model not available");
            return "";
        }
        // 2、构造请求参数
        double temperature = 0.7;
        int maxOutputTokens = 2048;
        if (requestParams.find("temperature") != requestParams.end()) {
            temperature = std::stod(requestParams.at("temperature"));
        }
        if (requestParams.find("max_output_tokens") != requestParams.end()) {
            maxOutputTokens = std::stoi(requestParams.at("max_output_tokens"));
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
        requestBody["input"] = messageArry;
        requestBody["temperature"] = temperature;
        requestBody["max_output_tokens"] = maxOutputTokens;
        // 4、序列化
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(builder, requestBody);
        INFO("ChatGPTProvider sendMessage requestBody: {}", requestBodyStr);
        // 5、使用cpp-httplib库构建客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(60, 0);
        client.set_proxy("127.0.0.1", 7890);
        // 设置请求头
        httplib::Headers headers = {
            {"Authorization", "Bearer " + _apiKey},
            // {"Content-Type", "application/json"}
        };
        // 6、发送POST请求
        auto resp = client.Post("/v1/responses", headers, requestBodyStr, "application/json");
        if (!resp) {
            auto err = resp.error();
            ERR("ChatGPTProvider sendMessage POST request failed, error: {}, error_message: {}", 
                static_cast<int>(err), httplib::to_string(err));
            return "";
        }
        // 检测响应是否成功
        if (resp->status != 200) {
            INFO("ChatGPTProvider sendMessage POST request failed, status : {}",resp->status);
            return "";
        }
        INFO("ChatGPTProvider sendMessage POST request body : {}", resp->body);
        // 7、解析响应体
        Json::Value responseBody;
        Json::CharReaderBuilder readerBuilder;
        std::string parseError;
        std::istringstream responseStream(resp->body);
        if (!Json::parseFromStream(readerBuilder, responseStream, &responseBody, &parseError)) {
            ERR("ChatGPTProvider sendMessage json parse failed, error : {}", parseError);
            return "";
        }
        if (responseBody.isMember("output") && responseBody["output"].isArray() && !responseBody["output"].empty()) {
            auto output = responseBody["output"][0];
            if (output.isMember("content") && output["content"].isArray() && !output["content"].empty() && output["content"][0].isMember("text")) {
                std::string replyContent = output["content"][0]["text"].asString();
                INFO("ChatGPTProvider response txt : {}", replyContent);
                return replyContent;
            }
        }
        ERR("ChatGPTProvider sendMessage parse response body failed, parseError : {}", parseError);
        return "";
    }

    // 流式返回
    std::string ChatGPTProvider::sendMessageStream(
        const std::vector<Message> &messages,
        const std::map<std::string, std::string> &requestParams,
        std::function<void(const std::string &, bool)> callback) {
        // 1、模型是否可用
        if(!isAvailable()){
            ERR("ChatGPTProvider sendMessageStream model not available");
            return "";
        }
        // 2、构造请求参数
        double temperature = 0.7;
        int maxTokens = 2048;
        if (requestParams.find("temperature") != requestParams.end()) {
            temperature = std::stod(requestParams.at("temperature"));
        }
        if (requestParams.find("max_output_tokens") != requestParams.end()) {
            maxTokens = std::stoi(requestParams.at("max_output_tokens"));
        }
        // 3、构造历史消息
        Json::Value messageArry(Json::arrayValue);
        for (const auto &msg : messages) {
            Json::Value messageObject;
            messageObject["role"] = msg._role;
            messageObject["content"] = msg._content;
            messageArry.append(messageObject);
        }
        // 4、构造请求体
        Json::Value requestBody;
        requestBody["model"] = getModelName();
        requestBody["input"] = messageArry;
        requestBody["temperature"] = temperature;
        requestBody["max_output_tokens"] = maxTokens;
        requestBody["stream"] = true;
        // 5、序列化
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(builder, requestBody);
        INFO("ChatGPTProvider sendMessageStream requestBody: {}", requestBodyStr);
        // 6、使用cpp-httplib库构建客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(300, 0);
        client.set_proxy("127.0.0.1", 7890);
        // 设置请求头
        httplib::Headers headers = {{"Authorization", "Bearer " + _apiKey},
                                    {"Content-Type", "application/json"},
                                    {"Accept", "txt/event-stream"}};
        // 流式处理变量
        std::string buffer;
        bool getError = false;
        std::string errorMsg;
        int statusCode = 0;
        bool streamEnd = false;
        std::string fullResponse;

        httplib::Request req;
        req.method = "POST";
        req.path = "/v1/chat/completions";
        req.headers = headers;
        req.body = requestBodyStr;
        // 设置响应处理函数
        req.response_handler = [&](const httplib::Response &resp) {
          if (resp.status != 200) {
            getError = true;
            errorMsg = "Http status code : " + std::to_string(resp.status);
            return false;
          }
          return true;
        };
        req.content_receiver = [&](const char *data, size_t len, size_t offset,
                                   size_t totalLength) {
            if (getError) {
                return false;
            }
            buffer.append(data, len);
            INFO("ChatGPTProvider sendMessageStream buffer: {}", buffer);
            size_t pos;
            while ((pos = buffer.find("\n\n")) != std::string::npos) {
                std::string event = buffer.substr(0, pos);
                buffer.erase(0, pos + 2);
                // 解析事件类型和具体数据位置
                std::istringstream eventStream(event);
                std::string line;
                std::string eventType;
                std::string eventData;
                while (std::getline(eventStream, line)) {
                    if (line.empty()) {
                        continue;
                    }
                    if (line.compare(0, 6, "event: ") == 0) {
                        eventType = line.substr(7);
                    } else if (line.compare(0, 5, "data: ") == 0) {
                        eventData = line.substr(6);
                    }
                }
                INFO("ChatGPTProvider sendMessageStream eventType: {}, eventData: {}", eventType, eventData);
                // 反序列化数据
                Json::Value chunk;
                Json::CharReaderBuilder builder;
                std::string err;
                std::istringstream eventDataStream(eventData);
                if (!Json::parseFromStream(builder, eventDataStream, &chunk, &err)) {
                    ERR("ChatGPTProvider sendMessageStream parse event data failed, err : {}", err);
                    continue;
                }
                if (eventType == "response.output_text.delta") {
                    if (chunk.isMember("delta") && chunk["delta"].isString()) {
                        std::string delta = chunk["delta"].asString();
                        callback(delta, false);
                    } else if (eventType == "response.output_item.done") {
                        if(chunk.isMember("item") && chunk["item"].isObject()){
                            Json::Value item = chunk["item"];
                            if (item.isMember("content") &&
                                item["content"].isArray() &&
                                !item["content"].empty() &&
                                item["content"][0].isMember("text") &&
                                item["content"][0]["text"].isString()) {
                                fullResponse += item["content"][0]["text"].asString();
                            }
                        }
                    } else if (eventType == "response.completed") {
                        streamEnd = true;
                        callback("", true);
                        return true;
                    }
                }
            }
            return true;
        };
        auto result = client.send(req);
        if(!result){
            ERR("ChatGPTProvider sendMessageStream POST request failed, error: {}", to_string(result.error()));
            return "";
        }
        if (!streamEnd) {
            WARN("stream ended without [DONE] marker");
            callback("", true);
            return "";
        }
        
        return fullResponse;
    }
}