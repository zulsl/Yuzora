#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QString>
#include <QList>
#include <QVariantMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "GlobalConstants.h"

/**
 * @brief 消息内容片段
 * 支持多模态扩展：文本、图片URL
 */
struct ContentPart {
    enum Type { Text, ImageUrl };

    Type type = Text;
    QString text;          // type == Text 时有效
    QString imageUrl;      // type == ImageUrl 时有效，支持 URL 或 data URI (base64)
    QString detail = "auto"; // "auto" / "low" / "high"

    QJsonObject toJson() const;

    static ContentPart fromJson(const QJsonObject &obj);
};


// ---------- 函数调用 ----------
struct ToolCall {
    QString id;            // 如 "call_abc123"
    QString type = "function";
    QString functionName;
    QString arguments;     // JSON 字符串，由模型生成

    QJsonObject toJson() const;

    static ToolCall fromJson(const QJsonObject &obj);
};

/**
 * @brief 标准聊天消息结构体
 * 业务层(PromptBuilder) → 网络层(ApiClient) 的统一输入契约
 * 与数据库实体 ChatMessageEntity 解耦，专用于API请求构建
 */
struct ChatMessage
{
    GlobalConstants::MessageType role = GlobalConstants::User;  //发送者角色
    QString senderName;                 // 发送者名称（示例对话靠它区分说话人）
    bool includeName = false;//是否把 senderName 作为 name 字段发给接口。
    // ST 默认（names_behavior = DEFAULT）只有示例消息带 name，聊天历史不带；
    // 打开会改变请求体，业务侧默认不开。

    QList<ContentPart> parts;           // 统一承载文本和图片

    QList<ToolCall> toolCalls;          // assistant 消息中的函数调用
    QString toolCallId;                 // 工具调用ID

    QVariantMap metadata;               // 通用扩展元数据

    // 追加文本片段
    void appendText(const QString& text);

    // 快速添加图片
    void addImage(const QString &url, const QString &detail = "auto");

    // 快速构造纯文本消息
    static ChatMessage makeText(GlobalConstants::MessageType role, const QString& text);

    // 快速获取纯文本内容（兼容纯文本场景）
    QString textContent() const;

    //快速转json
    QJsonObject toJson() const;

    static ChatMessage fromJson(const QJsonObject &obj);
};

#endif // CHATMESSAGE_H