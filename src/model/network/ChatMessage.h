#ifndef CHATMESSAGE_H
#define CHATMESSAGE_H

#include <QString>
#include <QList>
#include <QVariantMap>
#include "GlobalConstants.h"

/**
 * @brief 消息内容片段
 * 支持多模态扩展：文本、图片URL、图片Base64
 */
struct ChatMessagePart
{
    enum class Type {
        Text,
        ImageUrl,
        ImageBase64
    };

    Type type = Type::Text;
    QString data;   //实际内容,文本内容 / 图片URL / base64数据
};

/**
 * @brief 标准聊天消息结构体
 * 业务层(PromptBuilder) → 网络层(ApiClient) 的统一输入契约
 * 与数据库实体 ChatMessageEntity 解耦，专用于API请求构建
 */
struct ChatMessage
{

    GlobalConstants::MessageType role = GlobalConstants::MessageType::User;  //发送者角色
    QString senderName;                 // 发送者名称（工具调用/角色名）(无用,保留字段)

    QList<ChatMessagePart> parts;       // 多模态内容片段

    QString testContent;

    QString toolCallId;                 // 工具调用ID
    QVariantMap metadata;               // 通用扩展元数据

    // ========== 便捷构造与读取方法 ==========
    static ChatMessage makeText(GlobalConstants::MessageType role, const QString& text);

    // 快速获取纯文本内容（兼容纯文本场景）
    QString textContent() const;

    // 追加文本片段
    void appendText(const QString& text);
};

#endif // CHATMESSAGE_H