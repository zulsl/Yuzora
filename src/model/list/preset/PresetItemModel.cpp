// #include "PresetItemModel.h"
// #include "GlobalConstants.h"

// PresetItemModel::PresetItemModel(const CompletionPresetEntity& entity, QObject* parent)
//     : QObject(parent)
//     , m_id(entity.id)
//     , m_name(entity.name)
//     , m_isDefault(entity.isDefault)
// {
//     // // 初始化子提示词列表模型
//     //     m_presetPromptModel = new PresetPromptListModel(this);

//     //     // 把实体中的提示词转为列表项并加载
//     //     QList<PresetPromptItemModel*> promptItems;
//     //     promptItems.reserve(entity.promptEntitys.size());
//     //     for (const auto& promptEntity : entity.promptEntitys) {
//     //         promptItems.append(new PresetPromptItemModel(promptEntity));
//     //     }
//     //     m_presetPromptModel->loadList(promptItems);
// }


// QString PresetItemModel::id() const { return m_id; }
// QString PresetItemModel::name() const { return m_name; }
// int PresetItemModel::modelType() const { return m_modelType; }
// QString PresetItemModel::modelName() const { return m_modelName; }
// bool PresetItemModel::isDefault() const { return m_isDefault; }