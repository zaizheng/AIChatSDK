#include "../include/LLMManager.h"
#include "../include/util/myLog.h"

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
        if (_providers.find(modelName) == _providers.end()) {
            return false;
        }
        _modelInfos[modelName] = _providers[modelName].get()->initModel(modelParam);
        return _modelInfos[modelName]._isAvailable;
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
        return _modelInfos.find(modelName) != _modelInfos.end() && _modelInfos[modelName]._isAvailable;
    }
}
