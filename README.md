# AIChatSDK

<div align="center">

一个基于 C++ 的 AI 聊天软件开发工具包，提供对多种大语言模型的统一接口访问，支持 DeepSeek、ChatGPT 和 Ollama 等多种后端服务。

[![C++](https://img.shields.io/badge/C++-17-blue.svg)](https://isocpp.org/)[![Platform](https://img.shields.io/badge/platform-Linux-orange.svg)](https://www.linux.org/)

</div>

---

## 📁 项目结构

```
AIChatSDK/
|-- ChatServer/
|   |-- ChatServer.cpp
|   |-- ChatServer.h
|   |-- main.cpp
|   |-- require.txt
|   `-- www/
|       |-- index.html
|       |-- script.js
|       `-- styles.css
`-- sdk/
    |-- include/
    |   |-- ChatGPTProvider.h
    |   |-- ChatSDK.h
    |   |-- common.h
    |   |-- DataManager.h
    |   |-- DeepSeekProvider.h
    |   |-- LLMManager.h
    |   |-- LLMProvider.h
    |   |-- OllamaLLMProvider.h
    |   |-- SessionManager.h
    |   `-- util/
    `-- src/
        |-- ChatGPTProvider.cpp
        |-- ChatSDK.cpp
        |-- DataManager.cpp
        |-- DeepSeekProvider.cpp
        |-- LLMManager.cpp
        |-- OllamaLLMProvider.cpp
        |-- SessionManager.cpp
        `-- util/
```

---

## ✨ 功能特性

### 后端 SDK

- 🔥 **多模型支持**：同时支持 DeepSeek、ChatGPT 和 Ollama 本地模型
- 💬 **会话管理**：完整的会话创建、查询和删除功能
- 📡 **消息发送**：支持全量返回和流式返回两种模式
- 🌐 **RESTful API**：提供 HTTP 服务接口，便于集成到其他应用
- ⚙️ **配置灵活**：支持命令行参数和配置文件两种方式

### 前端界面

- 🎨 **现代化 UI**：渐变背景、毛玻璃效果、响应式设计
- 📝 **Markdown 渲染**：支持标题、列表、表格、引用等完整语法
- 💻 **代码高亮**：自动语言检测，一键复制代码块
- 🔄 **流式响应**：实时显示 AI 回复，提升交互体验
- 📱 **移动端适配**：完美支持各种屏幕尺寸

---

## 🚀 快速开始

### 安装依赖（Ubuntu Linux）

```bash
# 安装基础编译工具
sudo apt install -y build-essential cmake git

# 安装 httplib（头文件库）
git clone https://github.com/yhirose/cpp-httplib.git
cd cpp-httplib
sudo cp -r include/httplib /usr/local/include/
cd .. && rm -rf cpp-httplib

# 安装其他依赖
sudo apt-get install libgflags-dev
sudo apt-get install libspdlog-dev
sudo apt-get install fmt
sudo apt-get install libjsoncpp-dev
sudo apt-get install libssl-dev
```

### 编译构建

```bash
# 编译 SDK 静态库（在 sdk 目录下执行）
cd sdk
mkdir build && cd build
cmake ..
make && make install

# 编译服务器（在 ChatServer 目录下执行）
cd ChatServer
mkdir build && cd build
cmake ..
make
```

### 运行服务器

```bash
# 设置环境变量（至少设置一个 API Key）
export deepseek_apikey=your_deepseek_api_key
export chatgpt_apikey=your_chatgpt_api_key

# 启动服务器（默认端口 8080）
./AIChatServer

# 指定端口启动
./AIChatServer --port=9000

# 使用配置文件
./AIChatServer --config_file=config.conf
```

### 访问前端界面

服务器启动后，在浏览器中访问：

```
http://localhost:8080
```

---

## 📖 使用指南

### 前端界面操作

1. **创建新对话**：点击「新建对话」按钮，选择 AI 模型
2. **发送消息**：在输入框输入内容，按 Enter 或点击「发送」
3. **查看历史**：左侧会话列表显示所有历史对话
4. **删除会话**：点击会话卡片右侧的删除按钮

### 配置参数

| 参数                    | 类型     | 默认值     | 说明            |
| --------------------- | ------ | ------- | ------------- |
| `--host`              | string | 0.0.0.0 | 服务器绑定地址       |
| `--port`              | int    | 8080    | 服务器端口         |
| `--log_level`         | string | INFO    | 日志级别          |
| `--temperature`       | double | 0.7     | 生成温度          |
| `--max_tokens`        | int    | 2048    | 最大 token 数    |
| `--ollama_model_name` | string | -       | Ollama 模型名称   |
| `--ollama_model_desc` | string | -       | Ollama 模型描述   |
| `--ollama_endpoint`   | string | -       | Ollama API 地址 |

### 环境变量

- `deepseek_apikey`: DeepSeek API 密钥
- `chatgpt_apikey`: ChatGPT API 密钥

---

## 🔌 API 接口

### 创建会话

```bash
POST /api/session
Content-Type: application/json

{
  "model_name": "deepseek-chat"
}
```

### 获取会话列表

```bash
GET /api/sessions
```

### 获取模型列表

```bash
GET /api/models
```

### 删除会话

```bash
DELETE /api/session/{session_id}
```

### 获取历史消息

```bash
GET /api/session/{session_id}/history
```

### 发送消息（全量返回）

```bash
POST /api/message
Content-Type: application/json

{
  "session_id": "xxx",
  "message": "你好"
}
```

### 发送消息（流式返回）

```bash
POST /api/message/async
Content-Type: application/json

{
  "session_id": "xxx",
  "message": "你好"
}
```

---

## 🛠️ 技术栈

### 后端

| 技术 | 说明 |
|------|------|
| C++17 | 核心开发语言 |
| cpp-httplib | HTTP 服务器框架 |
| spdlog | 日志库 |
| jsoncpp | JSON 解析 |
| gflags | 命令行参数解析 |

### 前端

| 技术 | 说明 |
|------|------|
| HTML5 | 页面结构 |
| CSS3 | 样式设计（渐变、毛玻璃、响应式） |
| JavaScript | 交互逻辑 |
| Marked.js | Markdown 渲染 |
| Highlight.js | 代码语法高亮 |
| Font Awesome | 图标库 |

---

## 🤖 支持的模型

| 模型名称             | 类型     | 说明           |
| ---------------- | ------ | ------------ |
| deepseek-chat    | API    | DeepSeek 云服务 |
| gpt-4o-mini      | API    | OpenAI 云服务   |
| deepseek-r1:1.5b | Ollama | 本地部署模型       |

---

## 💻 SDK 使用示例

```cpp
#include <iostream>
#include <memory>
#include <ChatSDK.h>
#include <util/myLog.h>

int main() {
    // 初始化日志系统（必须）
    LogUtil::Logger::initLogger("ai_sdk_test", "stdout");
    
    // 创建 SDK 实例
    auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();
    
    // 配置模型
    auto config = std::make_shared<ai_chat_sdk::APIConfig>();
    config->_modelName = "deepseek-chat";
    config->_apiKey = "your_api_key";
    config->_temperature = 0.7;
    config->_maxTokens = 2048;
    
    // 初始化模型
    sdk->initModels({config});
    
    // 创建会话
    std::string sessionId = sdk->createSession("deepseek-chat");
    
    // 发送消息（全量返回）
    std::string response = sdk->sendMessage(sessionId, "你好，请介绍一下自己");
    std::cout << "AI 回复: " << response << std::endl;
    
    // 发送消息（流式返回）
    std::cout << "流式响应: ";
    sdk->sendMessageStream(sessionId, "用几句话解释一下什么是人工智能", 
        [](const std::string& chunk, bool last) {
            std::cout << chunk << std::flush;
            if (last) std::cout << std::endl;
        });
    
    // 会话管理
    auto sessions = sdk->getSessionsLists();                    // 获取所有会话
    auto session = sdk->getSession(sessionId);                  // 获取指定会话
    sdk->deleteSession(sessionId);                              // 删除会话
    
    // 多模型配置
    auto chatgptConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    chatgptConfig->_modelName = "gpt-4o-mini";
    chatgptConfig->_apiKey = "your_chatgpt_api_key";
    
    auto ollamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
    ollamaConfig->_modelName = "deepseek-r1:1.5b";
    ollamaConfig->_endpoint = "http://localhost:11434";
    
    sdk->initModels({config, chatgptConfig, ollamaConfig});
    auto models = sdk->getAvailableModels();                    // 获取可用模型列表
    
    return 0;
}
```

### 编译运行

```bash
# 编译 SDK（如果尚未编译）
cd sdk
mkdir -p build && cd build
cmake .. && make && make install
cd ../..

# 编译示例程序
g++ -std=c++17 -o ai_sdk_test main.cpp \
    -I./sdk/include \
    -L./sdk/build \
    -lai_chat_sdk -ljsoncpp -lfmt -lspdlog -lsqlite3 -lssl -lcrypto -lpthread

# 运行
./ai_sdk_test
```

---

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

### 贡献指南

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 提交 Pull Request


---

## 📮 联系方式

如有问题或建议，欢迎通过以下方式联系：

- 提交 [Issue](../../issues)
- 发送 Pull Request

---

<div align="center">

**⭐ 如果这个项目对你有帮助，请给一个 Star 支持一下！⭐**

</div>