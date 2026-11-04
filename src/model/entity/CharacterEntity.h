#ifndef CHARACTERENTITY_H
#define CHARACTERENTITY_H

#include <QString>
#include <QDateTime>
#include <QVariantMap>
#include <QtQml>


//角色数据实体

struct CharacterEntity {
    Q_GADGET//元对象开关
    QML_VALUE_TYPE(characterEntity)//注册为 QML 原生值类型,首字母小写

public:
    // 主键（UUID）
    QString id;

    // ========== 角色卡核心字段（兼容标准规范） ==========
    QString name;                       //姓名
    QString description;                //角色描述+
    QString firstMessage;               //第一条消息
    QStringList alternateGreetings;     //备选开场白列表
    QString creatorNotes;               //作者注释+
    QString creator;                    //角色作者名称

    QString personality;                //性格特征设定+
    QString scenario;                   //对话场景设定+
    QString exampleDialogue;            //对话示例+
    QString characterVersion;           //作者自身版本号
    QStringList tags;                   //作者标签分类列表


    //其他
    QString systemPrompt;               //角色专属系统提示词+(可开关,覆盖系统提示词)
        //角色独立系统提示词,单卡专属 <System> 全局系统提示，加载该角色时直接替换全局系统 提示，专门给这个角色用
    QString postHistoryInstructions;  //后续历史指令
        //对话追加提示,拼在全部聊天消息的末尾、发给 AI 的最后一段文本。


    //其他参数:persona用户提示词,depth_prompt.prompt角色备注
    //所有提示词都需要使用baseChatReplace替换

    // ========== 格式与资源字段 ==========
    int cardFormatVersion = 2;       // 角色卡格式版本
    QString avatarPath;              // 头像相对路径

    // ========== 业务关联字段 ==========
    QStringList worldBookIds;        // 绑定的世界书ID列表

    // ========== 元数据与管理字段 ==========
    QString lastSessionId ;                // 最后一次会话id
    bool isFavorite = false;         // 是否收藏(预留)

    // ========== 扩展与时间戳 ==========
    QVariantMap extensions;         //扩展
    QDateTime createdAt;            //创建时间
    QDateTime updatedAt;            //更新时间

    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString description MEMBER description)
    Q_PROPERTY(QString firstMessage MEMBER firstMessage)
    Q_PROPERTY(QStringList alternateGreetings MEMBER alternateGreetings)
    Q_PROPERTY(QString creatorNotes MEMBER creatorNotes)
    Q_PROPERTY(QString creator MEMBER creator)
    Q_PROPERTY(QString personality MEMBER personality)
    Q_PROPERTY(QString scenario MEMBER scenario)
    Q_PROPERTY(QString exampleDialogue MEMBER exampleDialogue)
    Q_PROPERTY(QString characterVersion MEMBER characterVersion)
    Q_PROPERTY(QStringList tags MEMBER tags)
    Q_PROPERTY(QString systemPrompt MEMBER systemPrompt)
    Q_PROPERTY(QString postHistoryInstructions MEMBER postHistoryInstructions)

};
Q_DECLARE_METATYPE(CharacterEntity)
#endif // CHARACTERENTITY_H