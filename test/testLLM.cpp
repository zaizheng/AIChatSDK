#include "../sdk/include/DeepSeekProvider.h"
#include "../sdk/include/ChatGPTProvider.h"
#include "../sdk/include/OllamaLLMProvider.h"
#include "../sdk/include/ChatSDK.h"
#include <gtest/gtest.h>
#include <memory>
#include <spdlog/common.h>
#include "../sdk/include/util/myLog.h"

#if 0
TEST(DeepSeekProviderTest, sendMessage) {
    auto provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
    ASSERT_TRUE(provider != nullptr);
    const char* api_key = std::getenv("deepseek_apikey");
    if (api_key == nullptr) {
        FAIL() << "Environment variable deepseek_apikey not set";
    }
    std::map<std::string, std::string> modelParam;
    modelParam["api_key"] = api_key;
    modelParam["endpoint"] = "https://api.deepseek.com";

    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    std::map<std::string, std::string> requestParams = {{"temperature", "0.7"},
                                                        {"max_tokens", "2048"}};

    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back(ai_chat_sdk::Message("user", "你是谁"));
    // std::string response = provider->sendMessage(messages, requestParams);
    // 全量响应 auto writechunk = [&](const std::string& chunk, bool last){
        INFO("chunk : {}", chunk);
        if(last){
            INFO("[DONE]");
        }
    };
    std::string response = provider->sendMessageStream(messages,
    requestParams, writechunk); // 流式响应 ASSERT_FALSE(!response.empty());
}

TEST(ChatGPTProviderTest, sendMessage) {
    auto provider = std::make_shared<ai_chat_sdk::ChatGPTProvider>();
    ASSERT_TRUE(provider != nullptr);
    const char* api_key = std::getenv("chatgpt_apikey");
    if (api_key == nullptr) {
        FAIL() << "Environment variable chatgpt_apikey not set";
    }
    std::map<std::string, std::string> modelParam;
    modelParam["api_key"] = api_key;
    modelParam["endpoint"] = "https://api.openai.com";

    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    std::map<std::string, std::string> requestParams = {{"temperature", "0.7"},
                                                        {"max_output_tokens", "2048"}};

    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back(ai_chat_sdk::Message("user", "你是谁"));
    // std::string response = provider->sendMessage(messages, requestParams); // 全量响应
    auto writeChunk = [&](const std::string& chunk, bool last){ 
        INFO("chunk : {}", chunk);
        if(last){
            INFO("[DONE]"); 
        } 
    };
    std::string fullData = provider->sendMessageStream(messages, requestParams, writeChunk);
    ASSERT_FALSE(fullData.empty());
    INFO("response : {}", fullData);
}

TEST(OllamaLLMProviderTest, sendMessage){
    auto provider = std::make_shared<ai_chat_sdk::OllamaLLMProvider>();
    ASSERT_TRUE(provider != nullptr);

    std::map<std::string, std::string> modelParam;
    modelParam["model_name"] = "deepseek-r1:1.5b";
    modelParam["model_desc"] = "本地部署deepseek-r1:1.5b模型，采用专家混合架构，专注于深度理解与推理";
    modelParam["endpoint"] = "http://localhost:11434";

    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}
    };
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "你是谁？"});

    // 实例化DeepSeekProvider的对象
    // 调用sendMessage方法
    // std::string fullData = provider->sendMessage(messages, requestParam);
    auto writeChunk = [&](const std::string& chunk, bool last){ 
        INFO("chunk : {}", chunk);
        if(last){
            INFO("[DONE]"); 
        } 
    };
    std::string fullData = provider->sendMessageStream(messages, requestParam, writeChunk);
    ASSERT_FALSE(fullData.empty());
    INFO("response : {}", fullData);
}
#endif
// 测试ChatSDK
TEST(ChatSDKTest, sendMessage){
    auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();
    ASSERT_TRUE(sdk != nullptr);

    // 配置支持的模型参数：云模型-deepseek-chat gpt-4o-mini gemini-2.0-flash   Ollama本地接入deepseek-r1:1.5b
    // deepseek-chat
    auto deepseekConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    ASSERT_TRUE(deepseekConfig != nullptr);
    deepseekConfig->_modelName = "deepseek-chat";
    deepseekConfig->_apiKey = std::getenv("deepseek_apikey");
    ASSERT_FALSE(deepseekConfig->_apiKey.empty());
    deepseekConfig->_temperature = 0.7;
    deepseekConfig->_maxTokens = 2048;

    // gpt-4o-mini
    auto chatGPTConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    ASSERT_TRUE(chatGPTConfig != nullptr);
    chatGPTConfig->_modelName = "gpt-4o-mini";
    chatGPTConfig->_apiKey = std::getenv("chatgpt_apikey");
    ASSERT_FALSE(chatGPTConfig->_apiKey.empty());
    chatGPTConfig->_temperature = 0.7;
    chatGPTConfig->_maxTokens = 2048;

    // Ollama本地接入deepseek-r1:1.5b
    auto ollamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
    ASSERT_TRUE(ollamaConfig != nullptr);
    ollamaConfig->_modelName = "deepseek-r1:1.5b";
    ollamaConfig->_modelDesc = "本地部署deepseek-r1:1.5b模型，采用专家混合架构，专注于深度理解与推理";
    ollamaConfig->_endpoint = "http://localhost:11434";
    ollamaConfig->_temperature = 0.7;
    ollamaConfig->_maxTokens = 2048;

    std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
        deepseekConfig, chatGPTConfig, ollamaConfig
    };

    sdk->initModels(modelConfigs);

    // 创建会话
    auto sessionId = sdk->createSession(deepseekConfig->_modelName);
    ASSERT_FALSE(sessionId.empty());

    std::string message;
    std::cout<<">>> ";
    std::getline(std::cin, message);
    auto response = sdk->sendMessage(sessionId, message);
    ASSERT_FALSE(response.empty());

    std::cout<<">>> ";
    std::getline(std::cin, message);
     sdk->sendMessage(sessionId, message);
    ASSERT_FALSE(response.empty());

    // 获取会话历史消息
    auto messages = sdk->_sessionManager.getHistoryMessages(sessionId);
    for(const auto& msg : messages){
        std::cout<<msg._role<<": "<<msg._content<<std::endl;
    }
    ASSERT_FALSE(messages.empty());
}

int main(int argc, char **argv) {
    LogUtil::Logger::initLogger("testlog", "stdout", spdlog::level::debug);
  
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
