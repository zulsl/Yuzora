#ifndef CHATMESSAGEENTITY_H
#define CHATMESSAGEENTITY_H

#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include "GlobalConstants.h"

//聊天记录数据实体

struct ChatMessageEntity {
    QString id;
    QString sessionId;                                      //所属会话id
    GlobalConstants::MessageType type = GlobalConstants::MessageType::System; //聊天消息类型
    GlobalConstants::MessageGenerationStatus status
        = GlobalConstants::MessageGenerationStatus::Normal; //消息生成状态
    QString content;                                        //内容
    QString senderName;                                     //发送者名字,role(保留字段,便于测试识别)
    QDateTime timestamp;                                    //时间戳
    QVariantMap metadata;                                   //元数据
    bool isEdited = false;                                  //已编辑
    bool isHidden = false;                                  //已隐藏
};

#endif // CHATMESSAGEENTITY_H