// #include "WorldInfoEntryItemModel.h"

// WorldInfoEntryItemModel::WorldInfoEntryItemModel(const WorldInfoEntryEntity& entity, QObject* parent)
//     : QObject(parent)
//     , m_id(entity.id)
//     , m_name(entity.name)
//     , m_isEnabled(entity.isEnabled)
//     , m_priority(entity.priority)
// {
//     // 拼接关键词预览，最多显示前3个，多余用省略号
//     QStringList previewList = entity.keywords.mid(0, 3);
//     if (entity.keywords.size() > 3)
//         previewList << "...";
//     m_keywordPreview = previewList.join("、");
// }

// QString WorldInfoEntryItemModel::id() const
// {
//     return m_id;
// }

// QString WorldInfoEntryItemModel::name() const
// {
//     return m_name;
// }

// QString WorldInfoEntryItemModel::keywordPreview() const
// {
//     return m_keywordPreview;
// }

// bool WorldInfoEntryItemModel::isEnabled() const
// {
//     return m_isEnabled;
// }

// int WorldInfoEntryItemModel::priority() const
// {
//     return m_priority;
// }