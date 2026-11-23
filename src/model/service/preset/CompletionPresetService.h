#ifndef COMPLETIONPRESETSERVICE_H
#define COMPLETIONPRESETSERVICE_H

#include <QObject>
#include <QList>
#include "CompletionPresetDao.h"
#include "CompletionPresetEntity.h"
#include "PromptDao.h"
#include "PromptEntity.h"

class CompletionPresetService : public QObject
{
    Q_OBJECT
public:
    static CompletionPresetService& instance();
    ~CompletionPresetService() override;

    // 测试/组合根可注入 DAO；生产仍使用 instance()。
    explicit CompletionPresetService(
            CompletionPresetDao* presetDao,PromptDao* promptDao,QObject* parent = nullptr);

    CompletionPresetService(const CompletionPresetService&) = delete;
    CompletionPresetService& operator=(const CompletionPresetService&) = delete;
    CompletionPresetService(CompletionPresetService&&) = delete;
    CompletionPresetService& operator=(CompletionPresetService&&) = delete;

    // 由 ApplicationBootstrap 在数据库初始化后调用；构造函数不再写库。
    void ensureSeedData();

    //----------预设----------
    // 查询
    QList<CompletionPresetEntity> getAllPresets();
    CompletionPresetEntity getPresetById(const QString& id);
    CompletionPresetEntity getActivePreset()const;
    // 增删改
    QString createPreset(const CompletionPresetEntity& preset);
    bool updatePreset(const CompletionPresetEntity& preset);
    bool deletePreset(const QString& id);
    QString duplicatePreset(const QString& sourceId);
    bool setActivePreset(const QString& id);
    bool renamePreset(const QString& id, const QString& newName);

    //----------提示词----------
    QList<PromptEntity> getPromptsByPresetId(const QString& id);
    PromptEntity getPromptById(const QString& id);
    bool createPrompts(const QList<PromptEntity>& prompts);
    bool createPrompt(const PromptEntity& prompt);
    bool updatePrompt(const PromptEntity& prompt);
    bool addPrompt(const QString& presetId, const PromptEntity& prompt);
    bool insertPromptBefore(const QString& presetId, const QString& beforeIdentifier,
                            const PromptEntity& prompt);
    bool deletePrompt(const QString& presetId, const QString& promptId);
    // 拖动换位置：visibleOrder 是编辑界面里"可见行"的新顺序（identifier 序列）。
    // promptOrder 里可能还留着没有实体的 identifier（导入的 ST 预设里常见），
    // 那些条目原地不动，只重填可见行占的槽位。顺序没变化时不写库。
    bool reorderPrompts(const QString& presetId, const QStringList& visibleOrder);

    void CreateDefaultPromptEntries(const QString& presetId,QStringList & promptOrderList);//创建默认提示词条目


    // 导入导出（兼容 SillyTavern 原生格式）
    int importFromSillyTavern(const QString& filePath,const QString& fileName);
    // 从已解析的 JSON 导入。同名预设存在时**覆盖更新**（保留 id / 激活状态 / 创建时间），
    // 否则新建。返回 1 表示成功（新建或更新）。
    int importFromSillyTavernJson(const QJsonObject& stJson, const QString& fileName);
    bool exportToSillyTavern(const QString& presetId, const QString& savePath);
    bool exportAllToSillyTavern(const QString& savePath);

signals:
    // void presetListChanged();
    void presetUpdated(const QString& presetId);
    void presetDeleted(const QString& presetId);
    void exportSuccess();
    void operationError(const QString& errorMsg);

private:
    explicit CompletionPresetService(QObject* parent = nullptr);
    void initBuiltinPresets();
    QStringList parsePromptOrder(const QJsonObject& cpJson,QHash<QString, bool>& h_isActive);
    bool validatePreset(const CompletionPresetEntity& preset, QString& outError);//验证预设(用处不大)
    QStringList normalizePromptOrder(const QStringList& order) const;
    bool writePromptChange(const CompletionPresetEntity& preset, const PromptEntity& prompt,
                           const QStringList& promptOrder, bool insertPrompt,
                           const QString& deletePromptId);

    // SillyTavern 格式互转
    CompletionPresetEntity parsePresetDataToEntity(const QJsonObject& stJson,const QString& fileName,QList<PromptEntity>& promptList);//json转结构体
    QJsonObject toSillyTavernJson(const CompletionPresetEntity& preset);

    CompletionPresetDao* m_completionPresetDao = nullptr;
    PromptDao* m_promptDao = nullptr;
};

#endif