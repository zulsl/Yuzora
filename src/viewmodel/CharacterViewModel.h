#ifndef CHARACTERVIEWMODEL_H
#define CHARACTERVIEWMODEL_H

#include <QObject>
#include <QVariantMap>
#include <QStringList>
#include <QUrl>
#include <QtQml/qqmlregistration.h>
#include <QQmlEngine>
#include "CharacterListModel.h"
#include "CharacterEntity.h"

class QJSEngine;
class CharacterService;
class ChatService;
class WorldInfoService;
class CharacterDraft;

/**
 * @brief 角色管理视图模型
 * 职责：视图状态持有、列表模型维护、实体与QVariantMap互转、UI事件转发
 * 本类以 QML 单例形式暴露给 QML（类型名 CharacterViewModel）。
 */
class CharacterViewModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    // QML 单例工厂（供 QML 使用）：没有 create() 时单例会被标记为 isCreatable=false。
    static CharacterViewModel* create(QQmlEngine*, QJSEngine*)
    {
        static CharacterViewModel instance;
        // 生命周期归 C++：静态实例必须由我们销毁，否则退出时被销毁两次导致卡死
        QQmlEngine::setObjectOwnership(&instance, QQmlEngine::CppOwnership);
        return &instance;
    }
    //列表model
    Q_PROPERTY(CharacterListModel* characterListModel READ characterListModel CONSTANT)
    //当前选中项索引（由当前角色 id 派生，只读）
    Q_PROPERTY(int currentIndex READ currentIndex NOTIFY currentIndexChanged)
    //当前角色（只读展示快照）
    Q_PROPERTY(CharacterEntity currentCharacter READ currentCharacter NOTIFY currentCharacterChanged)
    Q_PROPERTY(QString currentAvatarUrl READ currentAvatarUrl NOTIFY currentCharacterChanged)
    // 当前绑定的世界书 id（只读，读的是编辑草稿；空串 = 无绑定）。
    // 单本语义：即使将来底层支持多本，QML 也只通过这两个只读属性看结果，
    // 不直接操作 worldBookIds 列表，避免 "first()" 的规则散落到界面层。
    Q_PROPERTY(QString characterBookId READ characterBookId NOTIFY characterBookIdChanged FINAL)
    // 绑定文集的名字；无绑定或文集已被删除时为空串，由界面决定占位文案
    Q_PROPERTY(QString characterBookName READ getCharacterBookName NOTIFY characterBookNameChanged FINAL)
    // 编辑草稿：强类型 QObject，QML 直接读写；ViewModel 负责 dirty 和保存
    Q_PROPERTY(CharacterDraft* characterDraft READ characterDraft NOTIFY characterDraftChanged)
    Q_PROPERTY(bool characterDirty READ characterDirty NOTIFY characterDirtyChanged)

    //搜索关键词
    Q_PROPERTY(QString searchKeyword READ searchKeyword WRITE setSearchKeyword NOTIFY searchKeywordChanged)
    //有选择
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY currentCharacterChanged)


public:
    ~CharacterViewModel() override;


    //索引
    int currentIndex() const;
    Q_INVOKABLE bool requestSelectCharacter(const QString& charId);
    Q_INVOKABLE void confirmPendingCharacterSelection();
    Q_INVOKABLE void cancelPendingCharacterSelection();
    // 属性读取
    CharacterListModel* characterListModel() const;
    CharacterEntity currentCharacter() const;
    QString currentAvatarUrl() const;
    CharacterDraft* characterDraft() const;
    bool characterDirty() const;
    QString searchKeyword() const;
    void setSearchKeyword(const QString& keyword);
    bool hasSelection() const;

// ========== UI 可调用函数 ==========
    Q_INVOKABLE void refreshList();//刷新列表
    Q_INVOKABLE void createNewCharacter();//新建角色
    Q_INVOKABLE void setCharacterDraftField(const QString& field, const QVariant& value);
    Q_INVOKABLE bool saveCharacterDraft();
    Q_INVOKABLE void discardCharacterDraft();
    Q_INVOKABLE void updateAvatar(const QUrl& fileUrl);//更新角色头像
    Q_INVOKABLE void deleteCurrentCharacter();//删除角色
    Q_INVOKABLE void duplicateCurrentCharacter();//复制角色
    Q_INVOKABLE void importCharacter(const QUrl& fileUrl);//导入角色
    Q_INVOKABLE void exportCurrentCharacter(const QUrl& fileUrl);//导出当前角色
    Q_INVOKABLE void selectCharacter(const QString& charId);//选中角色
    Q_INVOKABLE void clearSelection();//清除选择
    Q_INVOKABLE void openSelectedCharChat();//打开角色会话

    // ========== 世界书绑定（单本语义）==========
    // 两个入口都只写编辑草稿，由「保存」统一提交；不直接改库。
    Q_INVOKABLE void bindCharacterBook(const QString& bookId);//绑定一本世界书（整个绑定替换为这一本）
    Q_INVOKABLE void unbindCharacterBook();//解绑当前世界书（清空绑定）
    // 强制让界面重新求值绑定展示（打开绑定向导时调用）
    // 存在的意义：QML 的只读属性只在收到 NOTIFY 时重算，一旦某条数据变更路径漏发信号，
    // 界面就会一直显示删除/改名前的旧值。这条兜底让"打开对话框"必定看到最新状态。
    Q_INVOKABLE void refreshCharacterBookBinding();

    QString characterBookId() const;
    QString getCharacterBookName() const;

signals:
    void currentIndexChanged();//当前索引变更
    void currentCharacterChanged();//角色表单变更
    void characterDraftChanged();
    void characterDirtyChanged();
    void unsavedChanges();
    void searchKeywordChanged();
    void operationError(const QString& errorMsg);//错误消息
    void operationTip(const QString& tipMsg);//操作提示(未完成)
    void importSuccess(int count);//导入成功
    void exportSuccess();//导出成功
    void navigateToChatPage(); // 通知QML切换页面

    void characterBookIdChanged();
    void characterBookNameChanged();

private slots:
    void onCharacterListChanged();
    void onCharacterUpdated(const QString& charId);
    void onOperationError(const QString& errorMsg);

private:
    // 内部转换方法
    void updateListModel(const QList<CharacterEntity>& entities);//更新列表model
    CharacterItemViewModel* createItemFromEntity(const CharacterEntity& entity);//制造列表项

    void refreshForm();
    void resetCharacterDraft(const CharacterEntity& entity);
    CharacterEntity draftToEntity() const;

    int m_currentIndex = -1;//当前列表索引
    CharacterListModel* m_characterListModel = nullptr;//列表数据源
    CharacterService* m_characterService = nullptr;
    ChatService* m_chatService = nullptr;
    WorldInfoService* m_worldInfoService = nullptr;
    CharacterEntity m_currentCharacter;//当前已保存快照
    CharacterDraft* m_characterDraft = nullptr;//当前编辑草稿
    CharacterEntity m_characterBaseline;//草稿基线，用于 dirty 判断
    bool m_characterDirty = false;
    QString m_pendingCharacterId;//等待用户确认丢弃草稿后要切换到的角色
    QString m_searchKeyword;

    // 构造函数必须 private：QML 单例只有在"无法默认构造"时才会调用 create()。
    // 若保持 public，引擎会自己 new 一个实例，C++ 侧的静态实例就与 QML 拿到的
    // 不是同一个对象（会出现"数据写进 A、界面读的是 B"这类极难定位的问题）。
    explicit CharacterViewModel(QObject* parent = nullptr);
};

#endif