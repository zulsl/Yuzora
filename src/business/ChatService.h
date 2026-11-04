// #ifndef CHATSERVICE_H
// #define CHATSERVICE_H

// #include <QObject>

// #include "GlobalConstants.h"
// #include "ChatRecordManager.h"
// #include "PromptManager.h"
// #include "BaseApiClient.h"

// //聊天核心业务

// class ChatService : public QObject {
//     Q_OBJECT
// public:
//     static ChatService& instance();
//     ~ChatService() override = default;

// // ======================================
// // 核心聊天操作
// // ======================================
//     //发送用户消息
//     void sendUserMessage(const QString& charId, const QString& sessionId, const QString& content);
//     //重新生成最后一条消息
//     void regenerateLastMessage(const QString& charId, const QString& sessionId);
//     //继续生成
//     void continueGeneration(const QString& charId, const QString& sessionId);
//     //取消生成
//     void cancelGeneration();
//     //编辑消息
//     void editMessage(const QString& charId, const QString& sessionId, const QString& messageId, const QString& newContent);

// // ======================================
// // 状态查询
// // ======================================
//     //正在生成
//     bool isGenerating() const { return m_isGenerating; }
//     //获取当前生成会话
//     QString getCurrentGeneratingSession() const { return m_currentSessionId; }

// signals:
//     //生成已开始
//     void generationStarted(const QString& charId, const QString& sessionId);
//     //生成块已接收
//     void generationChunkReceived(const QString& charId, const QString& sessionId, const QString& chunk);
//     //生成已完成
//     void generationFinished(const QString& charId, const QString& sessionId, const QString& fullText);
//     //生成失败
//     void generationFailed(const QString& charId, const QString& sessionId, const QString& error);

// private slots:
//     //当接收到生成块
//     void onStreamChunkReceived(const QString& chunk);
//     //当完成时
//     void onCompletionFinished(const QString& fullText);
//     //当请求失败时
//     void onRequestFailed(const QString& error);

// private:
//     ChatService(QObject* parent = nullptr);

//     //开始生成
//     void startGeneration(const QString& charId, const QString& sessionId, const QJsonArray& messages);

//     BaseApiClient* m_currentClient = nullptr;//当前客户端
//     bool m_isGenerating = false;//是否正在生成
//     QString m_currentCharId;//当前字符id
//     QString m_currentSessionId;//当前会话id
//     QString m_currentMessageId;//当前消息id
//     ModelConfig m_modelConfig;//当前模型配置
// };

// #endif // CHATSERVICE_H
