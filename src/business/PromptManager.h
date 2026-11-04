#ifndef PROMPTMANAGER_H
#define PROMPTMANAGER_H

#include <QObject>
#include <QJsonArray>
#include <QVariantMap>

#include "GlobalConstants.h"
#include "CharacterManager.h"
#include "ChatRecordManager.h"
#include "WorldInfoManager.h"
#include "MemoryManager.h"

//提示词管理核心业务

class PromptManager : public QObject {
    Q_OBJECT
public:
    static PromptManager& instance();
    ~PromptManager() override = default;

// ======================================
// 构建完整对话消息数组
// ======================================
    QJsonArray buildChatMessages(
        const QString& charId,//角色id
        const QString& sessionId,//会话id
        const QString& userInput,//用户输入
        GlobalConstants::PromptTemplateType templateType//提示词模板类型
        );

// ======================================
// 自定义模板配置
// ======================================
    //设置自定义提示词模板
    void setCustomPromptTemplate(const QString& templateStr);
    //获取提示词模板
    QString getPromptTemplate(GlobalConstants::PromptTemplateType type);
    //获取提示词变量
    QVariantMap getPromptVariables(const CharacterInfo& character);

// ======================================
// 模块独立构建
// ======================================
    //构建系统提示词
    QString buildSystemPrompt(const CharacterInfo& character);
    //构建世界书部分
    QString buildWorldInfoSection(const QString& bookId, const QList<ChatMessage>& context);
    //构建记忆部分
    QString buildMemorySection(const QString& charId, const QString& sessionId, const QString& query);

private:
    PromptManager(QObject* parent = nullptr);

    //加载默认模板
    void loadDefaultTemplates();

    QMap<GlobalConstants::PromptTemplateType, QString> m_templates;//模板
    QString m_customTemplate;//自定义模板
};

#endif // PROMPTMANAGER_H
