#include "ChatServer.h"
#include <ai_chat_sdk/util/myLog.h>
#include <httplib.h>
#include <jsoncpp/json/forwards.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>

namespace ai_chat_server {
    ChatServer::ChatServer(const ServerConfig &config) {
        _chatSDK = std::make_shared<ai_chat_sdk::ChatSDK>();
        // deepseek-chat
        auto deepseekConfig = std::make_shared<ai_chat_sdk::APIConfig>();
        deepseekConfig->_modelName = "deepseek-chat";
        deepseekConfig->_apiKey = std::getenv("deepseek_apikey");
        deepseekConfig->_temperature = config.temperature;
        deepseekConfig->_maxTokens = config.maxTokens;

        // gpt-4o-mini
        auto chatGPTConfig = std::make_shared<ai_chat_sdk::APIConfig>();
        chatGPTConfig->_modelName = "gpt-4o-mini";
        chatGPTConfig->_apiKey = std::getenv("chatgpt_apikey");
        chatGPTConfig->_temperature = config.temperature;
        chatGPTConfig->_maxTokens = config.maxTokens;

        // Ollama本地接入deepseek-r1:1.5b
        auto ollamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
        ollamaConfig->_modelName = config.ollamaModelName;
        ollamaConfig->_modelDesc = config.ollamaModelDesc;
        ollamaConfig->_endpoint = config.ollamaEndpoint;
        ollamaConfig->_temperature = config.temperature;
        ollamaConfig->_maxTokens = config.maxTokens;

        std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
                deepseekConfig, chatGPTConfig, ollamaConfig
        };
        INFO("start init ChatSDK model...");
        if (!_chatSDK->initModels(modelConfigs)) {
            ERR("ChatSDK model init failed!!!");
            return;
        }
        INFO("ChatSDK model init success");
        _chatServer = std::unique_ptr<httplib::Server>(new httplib::Server());
        if (!_chatServer) {
            ERR("ChatServer init failed!!!");
            return;
        }
    }
    bool ChatServer::start() {
        if (_isRunning.load()) {
            ERR("ChatServer is already running!!!");
            return false;
        }
        setHttpRoutes();
        _chatServer->set_mount_point("/", "./www");
        // 为了不卡主线程，将服务器放到独立的线程中运行
        std::thread serverThread([this]() {
            _chatServer->listen(_config.host, _config.port);
            INFO("ChatServer start on {} :{}", _config.host, _config.port);
        });
        serverThread.detach();
        _isRunning.store(true);
        INFO("ChatServer start success");
        return true;
    }
    void ChatServer::stop() {
        if (!_isRunning.load()) {
            ERR("ChatServer is not running!!!");
            return;
        }
        if (_chatServer) {
            _chatServer->stop();  
        }
        _isRunning.store(false);
        INFO("ChatServer stop success");
    }
    bool ChatServer::isRunning() const {
        return _isRunning.load();
    }
    // 构建响应
    std::string ChatServer::buildResponse(const std::string& message, bool success) {
        Json::Value responseJson;
        responseJson["success"] = success;
        responseJson["message"] = message;
        Json::StreamWriterBuilder builder;
        return Json::writeString(builder, responseJson);
    }
    // 处理创建会话请求
    void ChatServer::handleCreateSessionRequest(const httplib::Request &request,
                                                httplib::Response &response) {
        // 解析请求体
        Json::Value requestJson;
        Json::Reader reader;
        if (!reader.parse(request.body, requestJson)) {
            response.body = buildResponse("parse request body failed, json format error", false);
            response.status = 400;
            response.set_content(response.body, "application/json");
            return;
        }
        std::string modelName = requestJson.get("model", "deepseek-chat").asString();
        std::string sessionId = _chatSDK->createSession(modelName);
        if (sessionId.empty()) {
            response.body = buildResponse("create session failed", false);
            response.status = 500;
            response.set_content(response.body, "application/json");
            return;
        }
        // 构建响应体
        Json::Value dataJson;
        dataJson["session_id"] = sessionId;
        dataJson["model"] = modelName;
        
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "create session success";
        responseJson["data"] = dataJson;
        Json::StreamWriterBuilder builder;
        std::string responseJsonStr = Json::writeString(builder, responseJson);
        
        response.status = 200;
        response.set_content(responseJsonStr, "application/json");
    }
    // 处理获取会话列表请求
    void ChatServer::handleGetSessionListsRequest(const httplib::Request &request,
                                                  httplib::Response &response) {
        std::vector<std::string> sessionIDs = _chatSDK->getSessionsLists();
        Json::Value dataArray(Json::arrayValue);
        for (const auto& sessionID : sessionIDs) {
            auto session = _chatSDK->getSession(sessionID);
            if (session) {
                Json::Value sessionJson;
                sessionJson["id"] = session->_sessionId;
                sessionJson["model"] = session->_modelName;
                sessionJson["created_at"] = static_cast<int64_t>(session->_createdAt);
                sessionJson["updated_at"] = static_cast<int64_t>(session->_updatedAt);
                sessionJson["message_count"] = session->_messages.size();
                if (!session->_messages.empty()) {
                    sessionJson["first_user_message"] = session->_messages.front()._content;
                }
                dataArray.append(sessionJson);
            }
        }
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get session lists success";
        responseJson["data"] = dataArray;
        Json::StreamWriterBuilder builder;
        std::string responseJsonStr = Json::writeString(builder, responseJson);
        
        response.status = 200;
        response.set_content(responseJsonStr, "application/json");
    }
    // 处理获取模型列表请求
    void ChatServer::handleGetModelListsRequest(const httplib::Request &request,
                                                httplib::Response &response) {
        auto modelLists = _chatSDK->getAvailableModels();
        Json::Value dataArray(Json::arrayValue);
        for (const auto &model : modelLists) {
            Json::Value modelJson;
            modelJson["name"] = model._modelName;
            modelJson["desc"] = model._modelDesc;
            dataArray.append(modelJson);
        }
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get model lists success";
        responseJson["data"] = dataArray;
        Json::StreamWriterBuilder builder;
        std::string responseJsonStr = Json::writeString(builder, responseJson);
        
        response.status = 200;
        response.set_content(responseJsonStr, "application/json");
    }
    // 处理删除会话请求
    void ChatServer::handleDeleteSessionRequest(const httplib::Request &request,
                                                httplib::Response &response) {
        std::string sessionID = request.matches[1];
        auto ret = _chatSDK->deleteSession(sessionID);
        if (ret) {
            std::string errJsonStr = buildResponse("delete session success", true);
            response.status = 200;
            response.set_content(errJsonStr, "application/json");
        } else {
            std::string errJsonStr = buildResponse("delete session failed", false);
            response.status = 404;
            response.set_content(errJsonStr, "application/json");
        }
    }
    // 处理获取历史消息请求
    void ChatServer::handleGetHistoryMessagesRequest(const httplib::Request &request,
                                                httplib::Response &response) {
        std::string sessionID = request.matches[1];
        auto session = _chatSDK->getSession(sessionID);
        if (!session) {
            std::string errJsonStr = buildResponse("session not found", false);
            response.status = 404;
            response.set_content(errJsonStr, "application/json");
            return;
        }
        Json::Value dataArray(Json::arrayValue);
        for (const auto &message : session->_messages) {
            Json::Value messageJson;
            messageJson["id"] = message._messageId;
            messageJson["role"] = message._role;
            messageJson["content"] = message._content;
            messageJson["timestamp"] = static_cast<int64_t>(message._timestamp);
            dataArray.append(messageJson);
        }
        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get history messages success";
        responseJson["data"] = dataArray;
        Json::StreamWriterBuilder builder;
        std::string responseJsonStr = Json::writeString(builder, responseJson);
        
        response.status = 200;
        response.set_content(responseJsonStr, "application/json");
    }
    // 处理发送消息请求-全量返回
    void ChatServer::handleSendMessageRequest(const httplib::Request &request,
                                              httplib::Response &response) {
        Json::Value requestJson;
        Json::Reader reader;
        if (!reader.parse(request.body, requestJson)) {
            std::string errJsonStr = buildResponse("parse request body failed, json format error", false);
            response.status = 400;
            response.set_content(errJsonStr, "application/json");
            return;
        }
        
        std::string sessionID = requestJson["session_id"].asString();
        std::string message = requestJson["message"].asString();
        if (sessionID.empty() || message.empty()) {
            std::string errJsonStr = buildResponse("session_id or message is empty", false);
            response.status = 400;
            response.set_content(errJsonStr, "application/json");
            return;
        }

        std::string assistantMessage = _chatSDK->sendMessage(sessionID, message);
        if (assistantMessage.empty()) {
            std::string errJsonStr = buildResponse("send message failed", false);
            response.status = 500;
            response.set_content(errJsonStr, "application/json");
            return;
        }
        
        Json::Value dataJson;
        dataJson["session_id"] = sessionID;
        dataJson["response"] = assistantMessage;
        dataJson["data"]["assistant_message"] = assistantMessage;

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "send message success";
        responseJson["data"] = dataJson;
        Json::StreamWriterBuilder builder;
        std::string responseJsonStr = Json::writeString(builder, responseJson);
        
        response.status = 200;
        response.set_content(responseJsonStr, "application/json");
    }
    // 处理发送消息请求-流式返回
    void ChatServer::handleSendMessageStreamRequest(const httplib::Request &request,
                                                    httplib::Response &response) {
        Json::Value requestJson;
        Json::Reader reader;
        if (!reader.parse(request.body, requestJson)) {
            std::string errJsonStr = buildResponse("parse request body failed, json format error", false);
            response.status = 400;
            response.set_content(errJsonStr, "application/json");
            return;
        }
        
        std::string sessionID = requestJson["session_id"].asString();
        std::string message = requestJson["message"].asString();
        if (sessionID.empty() || message.empty()) {
            std::string errJsonStr = buildResponse("session_id or message is empty", false);
            response.status = 400;
            response.set_content(errJsonStr, "application/json");
            return;
        }
        response.status = 200;
        response.set_header("Cache-Control", "no-cache");              // 不使用缓存，服务器立即将数据发送到网络
        response.set_header("Connection", "keep-alive");               // 保持连接，服务器不会关闭连接
        response.set_header("Access-Control-Allow-Origin", "*");       // 允许跨域请求
        response.set_header("Access-Control-Allow-Headers", "*"); // 允许所有请求头

        
        response.set_chunked_content_provider("text/event-stream", [this, sessionID, message](size_t offset, httplib::DataSink& dataSink)->bool {
            auto writeChunk = [&](const std::string &chunk, bool last) {
                std::string sseData = "data: " + Json::valueToQuotedString(chunk.c_str()) + "\n\n";
                dataSink.write(sseData.c_str(), sseData.size());
                if (last) {
                    std::string doneData = "data: [DONE]\n\n";
                    dataSink.write(doneData.c_str(), doneData.size());
                    dataSink.done();
                    return false;
                }
                return true;
            };
            if (!writeChunk("", false)) {
                return false;
            }
            _chatSDK->sendMessageStream(sessionID, message, writeChunk);
            
            return false;
        });
    }
    // 设置HTTP路由规则
    void ChatServer::setHttpRoutes() {
        // 处理创建会话请求
        _chatServer->Post("/api/session", [this](const httplib::Request& request, httplib::Response& response){
            handleCreateSessionRequest(request, response);
        });

        // 处理获取会话列表请求
        _chatServer->Get("/api/sessions", [this](const httplib::Request& request, httplib::Response& response){
            handleGetSessionListsRequest(request, response);
        }); 

        // 处理获取模型列表请求
        _chatServer->Get("/api/models", [this](const httplib::Request& request, httplib::Response& response){
            handleGetModelListsRequest(request, response);
        });

        // 处理删除会话请求
        _chatServer->Delete("/api/session/(.*)", [this](const httplib::Request& request, httplib::Response& response){
            handleDeleteSessionRequest(request, response);
        });

        // 处理获取历史消息请求
        _chatServer->Get("/api/session/(.*)/history", [this](const httplib::Request& request, httplib::Response& response){
            handleGetHistoryMessagesRequest(request, response);
        });

        // 处理发送消息请求-全量返回
        _chatServer->Post("/api/message", [this](const httplib::Request& request, httplib::Response& response){
            handleSendMessageRequest(request, response);
        });

        // 处理发送消息请求-增量返回
        _chatServer->Post("/api/message/async", [this](const httplib::Request& request, httplib::Response& response){
            handleSendMessageStreamRequest(request, response);
        });
    }
}
