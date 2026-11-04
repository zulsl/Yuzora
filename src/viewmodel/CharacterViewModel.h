#ifndef CHARACTERVIEWMODEL_H
#define CHARACTERVIEWMODEL_H

#include <QObject>
#include <QVariantMap>
#include <QStringList>
#include "CharacterListModel.h"
#include "CharacterEntity.h"

/**
 * @brief 角色管理视图模型
 * 职责：视图状态持有、列表模型维护、实体与QVariantMap互转、UI事件转发
 */
class CharacterViewModel : public QObject
{
    Q_OBJECT
    //列表model
    Q_PROPERTY(CharacterListModel* characterListModel READ characterListModel CONSTANT)
    //当前选中项索引
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    //当前角色
    Q_PROPERTY(CharacterEntity currentCharacter READ currentCharacter NOTIFY currentCharacterChanged)
    Q_PROPERTY(QString characterBookName READ getCharacterBookName NOTIFY characterBookNameChanged FINAL)

    //搜索关键词
    Q_PROPERTY(QString searchKeyword READ searchKeyword WRITE setSearchKeyword NOTIFY searchKeywordChanged)
    //有选择
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY currentCharacterChanged)


public:
    explicit CharacterViewModel(QObject* parent = nullptr);
    ~CharacterViewModel() override;


    //索引
    int currentIndex() const;
    Q_INVOKABLE void setCurrentIndex(int index);
    // 属性读取
    CharacterListModel* characterListModel() const;
    CharacterEntity currentCharacter() const;
    QString searchKeyword() const;
    void setSearchKeyword(const QString& keyword);
    bool hasSelection() const;

// ========== UI 可调用函数 ==========
    Q_INVOKABLE void refreshList();//刷新列表
    Q_INVOKABLE void searchCharacters();//搜索
    Q_INVOKABLE void createNewCharacter();//新建角色
    Q_INVOKABLE void saveCurrentCharacter(const CharacterEntity& entity);//保存当前角色
    Q_INVOKABLE void deleteCurrentCharacter();//删除角色
    Q_INVOKABLE void duplicateCurrentCharacter();//复制角色
    Q_INVOKABLE void importCharacter(const QUrl& fileUrl);//导入角色
    Q_INVOKABLE void exportCurrentCharacter(const QString& savePath);//导出当前角色
    Q_INVOKABLE void toggleFavorite(const QString& charId);//切换收藏
    Q_INVOKABLE void selectCharacter(const QString& charId);//选中角色
    Q_INVOKABLE void clearSelection();//清除选择
    Q_INVOKABLE void openSelectedCharChat();//打开角色会话

    QString getCharacterBookName() const;
    void setCharacterBookName(const QString &newCharacterBookName);

signals:
    void currentIndexChanged();//当前索引变更
    void currentCharacterChanged();//角色表单变更
    void searchKeywordChanged();
    void operationError(const QString& errorMsg);//错误消息
    void operationTip(const QString& tipMsg);//操作提示(未完成)
    void importSuccess(int count);//导入成功
    void exportSuccess();//导出成功
    void navigateToChatPage(); // 通知QML切换页面

    void characterBookNameChanged();

private slots:
    void onCharacterListChanged();
    void onOperationError(const QString& errorMsg);

private:
    // 内部转换方法
    void updateListModel(const QList<CharacterEntity>& entities);//更新列表model
    CharacterItemViewModel* createItemFromEntity(const CharacterEntity& entity);//制造列表项

    void refreshForm();

    int m_currentIndex = -1;//当前列表索引
    CharacterListModel* m_characterListModel = nullptr;//列表数据源
    CharacterEntity m_currentCharacter;//当前表单数据
    QString m_searchKeyword;
};

#endif