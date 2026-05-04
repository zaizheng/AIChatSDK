#include "LLMProvider.h"

#include <map>
#include <string>
#include <vector>
#include <functional>
#include "common.h"

namespace ai_chat_sdk {
    class DeepSeekProvider : public LLMProvider {
    public:
        virtual bool initModel(const std::map<std::string, std::string>& modelConfig);
        virtual bool isAvailable() const;
        virtual std::string getModelName() const;
        virtual std::string getModelDes() const;
        // 全量返回
        virtual std::string sendMessage(const std::vector<Message>& messages, const std::map<std::string, std::string>& requestParams);
        // 流式返回
        virtual std::string sendMessageStream(const std::vector<Message> &messages,
                                       const std::map<std::string, std::string> &requestParams,
                                       std::function<void(const std::string&, bool)> callback); // callback：对模型返回的流式结果进行处理的回调函数
   };
}  // namespace ai_chat_sdk
