#include "../sdk/include/DeepSeekProvider.h"
#include <gtest/gtest.h>
#include <spdlog/common.h>
#include "../sdk/include/util/myLog.h"

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
    std::string response = provider->sendMessage(messages, requestParams);
    ASSERT_TRUE(!response.empty());
}

int main(int argc, char **argv) {
    LogUtil::Logger::initLogger("testlog", "stdout", spdlog::level::debug);
  
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
