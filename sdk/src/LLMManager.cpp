#include "../include/LLMManager.h"
#include "../include/util/myLog.h"
#include "../include/common.h"
namespace ai_chat_sdk {
    // 注册LLM提供者
    bool LLMManager::registerProvider(const std::string& modelName, std::unique_ptr<LLMProvider> provider) {
        if (!provider) {
            ERR("cannot register nullptr provider, modelName = {}", modelName);
            return false;
        }
        _providers[modelName] = std::move(provider);
        _modelInfos[modelName] = ModelInfo(modelName);
        INFO("register provider success, modelName = {}", modelName);
        return true;
    }
    // 初始化指定模型
    bool LLMManager::initModel(const std::string &modelName,
                               const std::map<std::string, std::string> &modelParam) {
        auto it = _providers.find(modelName);
        if (it == _providers.end()) {
            return false;
        }
        bool isSuccess = it->second->initModel(modelParam);
        if (!isSuccess) {
            ERR("init model failed, modelName = {}", modelName);
        } else {
            INFO("init model success, modelName = {}", modelName);
            _modelInfos[modelName]._modelDesc = it->second->getModelDesc();
            _modelInfos[modelName]._isAvailable = true;
        }
        return isSuccess;
    }
    // 获取可用模型
    std::vector<ModelInfo> LLMManager::getAvailableModels() const {
        std::vector<ModelInfo> availableModels;
        for (const auto &model : _modelInfos) {
            if (model.second._isAvailable) {
                availableModels.push_back(model.second);
            }
        }
        return availableModels;
    }
    // 检测模型是否可以使用
    bool LLMManager::isModelAvailable(const std::string &modelName) const {
        auto it = _modelInfos.find(modelName);
        if (it == _modelInfos.end()) {
            return false;
        }
        return it->second._isAvailable;
    }
    // 发送消息给指定模型
    std::string LLMManager::sendMessage(
        const std::string &modelName, const std::vector<Message> &messages,
        const std::map<std::string, std::string> &requestParam) {
        auto it = _providers.find(modelName);
        if (it == _providers.end()) {
            ERR("model not found, modelName = {}", modelName);
            return "";
        }
        if (!it->second->isAvailable()) {
            ERR("model not available, modelName = {}", modelName);
            return "";
        }
        return it->second->sendMessage(messages, requestParam);
    }
    // 发送流式消息给指定模型
    std::string LLMManager::sendMessageStream(
        const std::string &modelName, const std::vector<Message> &messages,
        const std::map<std::string, std::string> &requestParam,
        std::function<void(const std::string &, bool)> callback) {
        auto it = _providers.find(modelName);
        if (it == _providers.end()) {
            ERR("model not found, modelName = {}", modelName);
            return "";
        }
        if (!it->second->isAvailable()) {
            ERR("model not available, modelName = {}", modelName);
            return "";
        }
        return it->second->sendMessageStream(messages, requestParam, callback);
    }
}
