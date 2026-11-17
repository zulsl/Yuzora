#include "ChatMessage.h"

ChatMessage ChatMessage::makeText(GlobalConstants::MessageType role, const QString& text)
{
    ChatMessage msg;
    msg.role = role;
    ContentPart part;
    part.type = ContentPart::Text;
    part.text = text;
    msg.parts.append(part);
    return msg;
}

QString ChatMessage::textContent() const
{
    QStringList texts;
    for (const ContentPart& part : parts) {
        if (part.type == ContentPart::Type::Text) {
            texts.append(part.text);
        }
    }
    return texts.join("\n");
}

QJsonObject ChatMessage::toJson() const {
    QJsonObject obj;
    obj["role"] = messageTypeToString(role);

    // content 的序列化：只有一个文本块时，优先输出纯字符串以兼容旧接口
    if (parts.size() == 1 && parts.first().type == ContentPart::Text) {
        obj["content"] = parts.first().text;
    } else if (!parts.isEmpty()) {
        QJsonArray arr;
        for (const auto &part : parts)
            arr.append(part.toJson());
        obj["content"] = arr;
    }

    // 说话人名字：ST 默认只有示例消息带 name（getChat() 里 message.name 有值才发）。
    if (includeName && !senderName.isEmpty()) {
        obj["name"] = senderName;
    }

    if (!toolCalls.isEmpty()) {
        QJsonArray calls;
        for (const auto &call : toolCalls)
            calls.append(call.toJson());
        obj["tool_calls"] = calls;
    }

    if (!toolCallId.isEmpty())
        obj["tool_call_id"] = toolCallId;

    return obj;
}

ChatMessage ChatMessage::fromJson(const QJsonObject &obj) {
    //未完成
    // ChatMessage msg;
    // msg.role = obj["role"].toString();
    // msg.name = obj["name"].toString();
    // msg.toolCallId = obj["tool_call_id"].toString();

    // QJsonValue contentVal = obj["content"];
    // if (contentVal.isString()) {
    //     msg.addText(contentVal.toString());
    // } else if (contentVal.isArray()) {
    //     for (const auto &v : contentVal.toArray())
    //         msg.contentParts.append(ContentPart::fromJson(v.toObject()));
    // }

    // if (obj.contains("tool_calls")) {
    //     for (const auto &v : obj["tool_calls"].toArray())
    //         msg.toolCalls.append(ToolCall::fromJson(v.toObject()));
    // }

    // return msg;
    return ChatMessage{};
}

void ChatMessage::appendText(const QString& text)
{
    ContentPart part;
    part.type = ContentPart::Text;
    part.text = text;
    parts.append(part);
}

void ChatMessage::addImage(const QString &url, const QString &detail)
{
    ContentPart p;
    p.type = ContentPart::ImageUrl;
    p.imageUrl = url;
    p.detail = detail;
    parts.append(p);
}
QJsonObject ContentPart::toJson() const {
    QJsonObject obj;
    if (type == Text) {
        obj["type"] = "text";
        obj["text"] = text;
    } else {
        obj["type"] = "image_url";
        QJsonObject img;
        img["url"] = imageUrl;
        if (!detail.isEmpty())
            img["detail"] = detail;
        obj["image_url"] = img;
    }
    return obj;
}

ContentPart ContentPart::fromJson(const QJsonObject &obj) {
    ContentPart part;
    if (obj["type"].toString() == "text") {
        part.type = Text;
        part.text = obj["text"].toString();
    } else if (obj["type"].toString() == "image_url") {
        part.type = ImageUrl;
        QJsonObject img = obj["image_url"].toObject();
        part.imageUrl = img["url"].toString();
        part.detail = img["detail"].toString("auto");
    }
    return part;
}

QJsonObject ToolCall::toJson() const {
    QJsonObject func;
    func["name"] = functionName;
    func["arguments"] = arguments;
    QJsonObject obj;
    obj["id"] = id;
    obj["type"] = type;
    obj["function"] = func;
    return obj;
}

ToolCall ToolCall::fromJson(const QJsonObject &obj) {
    ToolCall call;
    call.id = obj["id"].toString();
    call.type = obj["type"].toString("function");
    QJsonObject func = obj["function"].toObject();
    call.functionName = func["name"].toString();
    call.arguments = func["arguments"].toString();
    return call;
}
