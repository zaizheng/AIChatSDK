#include "ChatServer.h"
#include <ai_chat_sdk/util/myLog.h>
#include <httplib.h>
#include <jsoncpp/json/forwards.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/writer.h>


namespace ai_chat_server{

    ChatServer::ChatServer(const ServerConfig& config){
        _chatSDK = std::make_shared<ai_chat_sdk::ChatSDK>();

        auto deepseekConfig = std::make_shared<ai_chat_sdk::APIConfig>();
        deepseekConfig->_modelName = "deepseek-chat";
        deepseekConfig->_apiKey = config.deepseekAPIKey;
        deepseekConfig->_temperature = config.temperature;
        deepseekConfig->_maxTokens = config.maxTokens;

        auto chatGPTConfig = std::make_shared<ai_chat_sdk::APIConfig>();
        chatGPTConfig->_modelName = "gpt-4o-mini";
        chatGPTConfig->_apiKey = config.chatGptAPIKey;
        chatGPTConfig->_temperature = config.temperature;
        chatGPTConfig->_maxTokens = config.maxTokens;

        auto ollamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
        ollamaConfig->_modelName = config.ollamaModelName;
        ollamaConfig->_modelDesc = config.ollamaModelDesc;
        ollamaConfig->_endpoint = config.ollamaEndpoint;
        ollamaConfig->_temperature = config.temperature;
        ollamaConfig->_maxTokens = config.maxTokens;

        std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
            deepseekConfig, chatGPTConfig, ollamaConfig
        };

        INFO("开始初始化ChatSDK模型...");
        if(!_chatSDK->initModels(modelConfigs)){
            ERR("ChatSDK初始化失败!!!");
            return;
        }
        INFO("ChatSDK模型初始化成功!!!");

        _chatServer = std::make_unique<httplib::Server>();
        if(!_chatServer){
            ERR("ChatServer初始化失败!!!");
            return;
        }
    }

    bool ChatServer::start(){
        if(_isRunning.load()){
            ERR("ChatServer已在运行中!!!");
            return false;
        }

        setHttpRoutes();

        _chatServer->set_mount_point("/", "../www");

        std::thread serverThread([this](){
            _chatServer->listen(_config.host, _config.port);
            INFO("ChatServer启动于 {} :{}", _config.host, _config.port);
        });

        serverThread.detach();
        _isRunning.store(true);
        INFO("ChatServer启动成功!!!");
        return true;
    }

    void ChatServer::stop(){
        if(!_isRunning.load()){
            ERR("ChatServer未在运行!!!");
            return;
        }

        if(_chatServer){
            _chatServer->stop();
        }

        _isRunning.store(false);
        INFO("ChatServer停止成功!!!");
    }

    bool ChatServer::isRunning() const{
        return _isRunning.load();
    }

    std::string ChatServer::buildResponse(const std::string& message, bool success){
        Json::Value responseJson;
        responseJson["success"] = success;
        responseJson["message"] = message;

        Json::StreamWriterBuilder writerBuilder;
        return Json::writeString(writerBuilder, responseJson);
    }

    void ChatServer::handleCreateSessionRequest(const httplib::Request& request, httplib::Response& response)
    {
        Json::Value requestJson;
        Json::Reader reader;
        if(!reader.parse(request.body, requestJson)){
            std::string errorJsonStr = buildResponse("parse request body failed, json format error");
            response.status = 400;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        std::string modelName = requestJson.get("model", "deepseek-chat").asString();
        
        std::string sessionID = _chatSDK->createSession(modelName);
        if(sessionID.empty()){
            std::string errorJsonStr = buildResponse("create session failed");
            response.status = 500;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        Json::Value dataJson;
        dataJson["session_id"] = sessionID;
        dataJson["model"] = modelName;

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "create session success";
        responseJson["data"] = dataJson;

        Json::StreamWriterBuilder writerBuilder;
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);

        response.status = 200;
        response.set_content(responseJsonStr, "application/json");
    }


    void ChatServer::handleGetSessionListsRequest(const httplib::Request& request, httplib::Response& response)
    {
        std::vector<std::string> sessionIDs = _chatSDK->getSessionsLists();

        Json::Value dataArray(Json::arrayValue);
        for(const auto& sessionID : sessionIDs){
            auto session = _chatSDK->getSession(sessionID);
            if(session){
                Json::Value sessionJson;
                sessionJson["id"] = session->_sessionId;
                sessionJson["model"] = session->_modelName;
                sessionJson["created_at"] = static_cast<int64_t>(session->_createdAt);
                sessionJson["updated_at"] = static_cast<int64_t>(session->_updatedAt);
                sessionJson["message_count"] = session->_messages.size();
                if(!session->_messages.empty()){
                    sessionJson["first_user_message"] = session->_messages.front()._content;
                }

                dataArray.append(sessionJson);
            }
        }

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get session lists success";
        responseJson["data"] = dataArray;

        Json::StreamWriterBuilder writerBuilder;
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);

        response.status = 200;
        response.set_content(responseJsonStr, "application/json");
    }

    void ChatServer::handleGetModelListsRequest(const httplib::Request& request, httplib::Response& response)
    {
        auto modelLists = _chatSDK->getAvailableModels();

        Json::Value dataArray(Json::arrayValue);
        for(const auto& modelInfo : modelLists){
            Json::Value modelJson;
            modelJson["name"] = modelInfo._modelName;
            modelJson["desc"] = modelInfo._modelDesc;
            dataArray.append(modelJson);
        }

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "get model lists success";
        responseJson["data"] = dataArray;

        Json::StreamWriterBuilder writerBuilder;
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);

        response.status = 200;
        response.set_content(responseJsonStr, "application/json");
    }

    void ChatServer::handleDeleteSessionRequest(const httplib::Request& request, httplib::Response& response)
    {
        std::string sessionId = request.matches[1];
        
        bool ret = _chatSDK->deleteSession(sessionId);
        if(ret){
            std::string errorJsonStr = buildResponse("delete session success", true);
            response.status = 200; 
            response.set_content(errorJsonStr, "application/json");
        }else{
            std::string errorJsonStr = buildResponse("delete session failed, session not found");
            response.status = 404;
            response.set_content(errorJsonStr, "application/json");
        }

    }

    void ChatServer::handleGetHistoryMessagesRequest(const httplib::Request& request, httplib::Response& response)
    {
        std::string sessionId = request.matches[1];
        auto session = _chatSDK->getSession(sessionId);
        if(!session){
            std::string errorJsonStr = buildResponse("session not found");
            response.status = 404;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        Json::Value dataArray(Json::arrayValue);
        for(const auto& message : session->_messages){
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

        Json::StreamWriterBuilder writerBuilder;
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);

        response.status = 200;
        response.set_content(responseJsonStr, "application/json");
    }


    void ChatServer::handleSendMessageRequest(const httplib::Request& request, httplib::Response& response)
    {
        Json::Value requestJson;
        Json::Reader reader;
        if(!reader.parse(request.body, requestJson)){
            std::string errorJsonStr = buildResponse("parse request body failed, json format error");
            response.status = 400;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        std::string sessionId = requestJson["session_id"].asString();
        std::string message = requestJson["message"].asString();
        if(sessionId.empty() || message.empty()){
            std::string errorJsonStr = buildResponse("session_id or message is empty");
            response.status = 400;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        std::string assistantMessage = _chatSDK->sendMessage(sessionId, message);
        if(assistantMessage.empty()){
            std::string errorJsonStr = buildResponse("Failed to send AI response message");
            response.status = 500;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        Json::Value dataJson;
        dataJson["session_id"] = sessionId;
        dataJson["response"] = assistantMessage;
        dataJson["data"]["assistant_message"] = assistantMessage;

        Json::Value responseJson;
        responseJson["success"] = true;
        responseJson["message"] = "send message success";
        responseJson["data"] = dataJson;

        Json::StreamWriterBuilder writerBuilder;
        std::string responseJsonStr = Json::writeString(writerBuilder, responseJson);

        response.status = 200;
        response.set_content(responseJsonStr, "application/json");
    }


    void ChatServer::handleSendMessageStreamRequest(const httplib::Request& request, httplib::Response& response)
    {
        Json::Value requestJson;
        Json::Reader reader;
        if(!reader.parse(request.body, requestJson)){
            std::string errorJsonStr = buildResponse("parse request body failed, json format error");
            response.status = 400;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        std::string sessionId = requestJson["session_id"].asString();
        std::string message = requestJson["message"].asString();
        if(sessionId.empty() || message.empty()){
            std::string errorJsonStr = buildResponse("session_id or message is empty");
            response.status = 400;
            response.set_content(errorJsonStr, "application/json");
            return;
        }

        response.status = 200;
        response.set_header("Cache-Control", "no-cache");
        response.set_header("Connection", "keep-alive");
        response.set_header("Access-Control-Allow-Origin", "*");
        response.set_header("Access-Control-Allow-Headers", "*");
        
        response.set_chunked_content_provider("text/event-stream", [this, sessionId, message](size_t offset, httplib::DataSink& dataSink)->bool{

            auto writeChunk = [&](const std::string& chunk, bool last){ 
                std::string sseData = "data: " + Json::valueToQuotedString(chunk.c_str()) + "\n\n";
                dataSink.write(sseData.c_str(), sseData.size());

                if(last){
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
            
            _chatSDK->sendMessageStream(sessionId, message, writeChunk);

            return false;
        });
    }

    void ChatServer::setHttpRoutes(){
        _chatServer->Post("/api/session", [this](const httplib::Request& request, httplib::Response& response){
            handleCreateSessionRequest(request, response);
        });

        _chatServer->Get("/api/sessions", [this](const httplib::Request& request, httplib::Response& response){
            handleGetSessionListsRequest(request, response);
        }); 

        _chatServer->Get("/api/models", [this](const httplib::Request& request, httplib::Response& response){
            handleGetModelListsRequest(request, response);
        });

        _chatServer->Delete("/api/session/(.*)", [this](const httplib::Request& request, httplib::Response& response){
            handleDeleteSessionRequest(request, response);
        });

        _chatServer->Get("/api/session/(.*)/history", [this](const httplib::Request& request, httplib::Response& response){
            handleGetHistoryMessagesRequest(request, response);
        });

        _chatServer->Post("/api/message", [this](const httplib::Request& request, httplib::Response& response){
            handleSendMessageRequest(request, response);
        });

        _chatServer->Post("/api/message/async", [this](const httplib::Request& request, httplib::Response& response){
            handleSendMessageStreamRequest(request, response);
        });
    }


} // end ai_chat_server