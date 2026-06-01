#include "../include/DeepSeekProvider.h"
#include "../include/util/myLog.h"
#include <jsoncpp/json/json.h>
#include <httplib.h>
#include <jsoncpp/json/reader.h>

namespace ai_chat_sdk {
    bool DeepSeekProvider::initModel(const std::map<std::string, std::string>& modelConfig) {
        auto it = modelConfig.find("api_key");
        if (it == modelConfig.end()) {
            ERR("DeepSeek初始化失败: api_key未找到");
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
        INFO("DeepSeek初始化成功, endpoint: {}",_endpoint);
        return true;
    }

    bool DeepSeekProvider::isAvailable() const { return _isAvailable; }

    std::string DeepSeekProvider::getModelName() const { return "deepseek-chat"; }

    std::string DeepSeekProvider::getModelDesc() const { return "深度求索"; }

    // 全量返回
    std::string DeepSeekProvider::sendMessage(
        const std::vector<Message> &messages,
        const std::map<std::string, std::string> &requestParams) {
        // 1、模型是否可用
        if(!isAvailable()){
            ERR("DeepSeek发送消息失败: 模型不可用");
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
        INFO("DeepSeek发送消息请求体: {}", requestBodyStr);
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
            auto err = resp.error();
            ERR("DeepSeek发送消息失败: POST请求失败, error: {}, error_message: {}", 
                static_cast<int>(err), httplib::to_string(err));
            return "";
        }
        // 检测响应是否成功
        if (resp->status != 200) {
            ERR("DeepSeek发送消息失败: POST请求失败, status : {}",resp->status);
            return "";
        }
        INFO("DeepSeek发送消息响应体 : {}", resp->body);
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
                    INFO("DeepSeek响应内容 : {}", replyContent);
                    return replyContent;
                }
            }
        }
        // 8、json解析失败
        ERR("DeepSeek发送消息失败: JSON解析失败, error : {}", parseError);
        return "DeepSeek响应JSON解析失败";
    }
    // 流式返回
    std::string DeepSeekProvider::sendMessageStream(
        const std::vector<Message> &messages,
        const std::map<std::string, std::string> &requestParams,
        std::function<void(const std::string &, bool)> callback) {
        // 1、模型是否可用
        if(!isAvailable()){
            ERR("DeepSeek流式发送失败: 模型不可用");
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
        requestBody["messages"] = messageArry;
        requestBody["temperature"] = temperature;
        requestBody["max_tokens"] = maxTokens;
        requestBody["stream"] = true;
        // 5、序列化
        Json::StreamWriterBuilder builder;
        builder["indentation"] = "";
        std::string requestBodyStr = Json::writeString(builder, requestBody);
        INFO("DeepSeek流式发送请求体: {}", requestBodyStr);
        // 6、使用cpp-httplib库构建客户端
        httplib::Client client(_endpoint.c_str());
        client.set_connection_timeout(30, 0);
        client.set_read_timeout(300, 0);
        // 设置请求头
        httplib::Headers headers = {{"Authorization", "Bearer " + _apiKey},
                                    {"Content-Type", "application/json"},
                                    {"Accept", "text/event-stream"}};

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
        // 设置数据接收处理函数
        req.content_receiver = [&](const char *data, size_t len, size_t offset,
                                   size_t totalLength) {
            if (getError) {
                return false;
            }
            buffer.append(data, len);
            INFO("DeepSeek流式发送缓冲区: {}", buffer);
            size_t pos;
            while ((pos = buffer.find("\n\n")) != std::string::npos) {
                std::string chunk = buffer.substr(0, pos);
                buffer.erase(0, pos + 2);
                // 判断是否为空或者注释
                if (chunk.empty() || chunk[0] == ':') {
                    continue;
                }
                // 获取模型返回的有效值
                if (chunk.compare(0, 6, "data: ") == 0) {
                    std::string modelData = chunk.substr(6);
                    // 标记是否结束
                    if (modelData == "[DONE]") {
                        callback("", true);
                        streamEnd = true;
                        return true;
                    }
                    // 反序列化
                    Json::Value modelDataJson;
                    Json::CharReaderBuilder reader;
                    std::string errors;
                    std::istringstream modelDataStream(modelData);
                    if (Json::parseFromStream(reader, modelDataStream,
                                               &modelDataJson, &errors)) {
                      if (modelDataJson.isMember("choices") &&
                          modelDataJson["choices"].isArray() &&
                          !modelDataJson["choices"].empty() &&
                          modelDataJson["choices"][0].isMember("delta") &&
                          modelDataJson["choices"][0]["delta"].isMember("content")) {
                            std::string content = modelDataJson["choices"][0]["delta"]["content"].asString();
                            fullResponse += content;
                            callback(content, false);
                        }
                    } else {
                        WARN("DeepSeek流式发送解析错误: {}", errors);
                    }
                }
            }
            return true;
        };
        auto result = client.send(req);
        if(!result){
            ERR("DeepSeek流式发送失败: POST请求失败, error: {}", to_string(result.error()));
            return "";
        }
        if (!streamEnd) {
            WARN("DeepSeek流式发送警告: 流未正常结束(缺少[DONE]标记)");
            callback("", true);
        }
        
        return fullResponse;
    }
} // namespace ai_chat_sdk