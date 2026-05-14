# AIChatSDK

一个基于 C++ 的 AI 聊天软件开发工具包，提供对多种大语言模型的统一接口访问，支持 DeepSeek、ChatGPT 和 Ollama 等多种后端服务。

## 功能特性

- **多模型支持**：同时支持 DeepSeek、ChatGPT 和 Ollama 本地模型
- **会话管理**：完整的会话创建、查询和删除功能
- **消息发送**：支持全量返回和流式返回两种模式
- **RESTful API**：提供 HTTP 服务接口，便于集成到其他应用
- **配置灵活**：支持命令行参数和配置文件两种方式

## 项目结构

```
AIChatSDK/
├── ChatServer/              # 聊天服务器模块
│   ├── main.cpp             # 服务器主入口
│   ├── ChatServer.cpp       # 服务器实现
│   └── ChatServer.h         # 服务器头文件
├── sdk/                     # SDK 核心模块
│   ├── include/             # 头文件目录
│   │   ├── ChatSDK.h        # SDK 主接口
│   │   ├── LLMManager.h     # LLM 管理器
│   │   ├── SessionManager.h # 会话管理器
│   │   ├── LLMProvider.h    # 模型提供者基类
│   │   ├── ChatGPTProvider.h
│   │   ├── DeepSeekProvider.h
│   │   ├── OllamaLLMProvider.h
│   │   └── util/myLog.h
│   └── src/                 # 源文件目录
│       ├── ChatSDK.cpp
│       ├── LLMManager.cpp
│       ├── SessionManager.cpp
│       ├── ChatGPTProvider.cpp
│       ├── DeepSeekProvider.cpp
│       ├── OllamaLLMProvider.cpp
│       └── util/myLog.cpp
└── test/                    # 测试模块
    └── testLLM.cpp          # 单元测试

```

## 快速开始

### 安装依赖（Ubuntu Linux）

```bash
# 安装基础编译工具
sudo apt install -y build-essential cmake git

# 安装 httplib（头文件库，通过源码安装）
git clone https://github.com/yhirose/cpp-httplib.git
cd cpp-httplib
sudo cp -r include/httplib /usr/local/include/
cd .. && rm -rf cpp-httplib

# 安装 gflags
sudo apt-get install libgflags-dev

# 安装 spdlog
sudo apt-get install libspdlog-dev

# 安装 fmt
sudo apt-get install fmt

# 安装 jsoncpp
sudo apt-get install libjsoncpp-dev

# 安装 ssl
sudo apt-get install libssl-dev
```

### 编译构建

```bash
# 创建构建目录
mkdir build && cd build

# 生成 Makefile
cmake ..

# 编译
make -j4
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

## API 接口

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

## SDK 使用示例

```cpp
#include <ChatSDK.h>

// 创建 SDK 实例
auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();

// 配置模型
auto config = std::make_shared<ai_chat_sdk::APIConfig>();
config->_modelName = "deepseek-chat";
config->_apiKey = "your_api_key";
config->_temperature = 0.7;

// 初始化模型
sdk->initModels({config});

// 创建会话
std::string sessionId = sdk->createSession("deepseek-chat");

// 发送消息
std::string response = sdk->sendMessage(sessionId, "你好");

// 流式发送消息
sdk->sendMessageStream(sessionId, "你好", [](const std::string& chunk, bool last) {
    std::cout << chunk;
});
```

## 支持的模型

| 模型名称             | 类型     | 说明           |
| ---------------- | ------ | ------------ |
| deepseek-chat    | API    | DeepSeek 云服务 |
| gpt-4o-mini      | API    | OpenAI 云服务   |
| deepseek-r1:1.5b | Ollama | 本地部署模型       |

## 贡献

欢迎提交 Issue 和 Pull Request！
