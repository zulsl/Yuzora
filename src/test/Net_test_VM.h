#ifndef NET_TEST_VM_H
#define NET_TEST_VM_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "GlobalConstants.h"
#include "ModelFactory.h"
#include "BaseApiClient.h"

/*
 * 网络模块测试 ViewModel
 * 遵循 MVVM：仅处理业务逻辑、状态转发，不操作UI
 */
class Net_test_VM : public QObject
{
    Q_OBJECT
    /* 注册requestLog属性,READ指定读属性的成员函数,NOTIFY属性变化通知信号
    WRITE qml可写
    Q_PROPERTY(类型 变量名 READ 读函数 [WRITE 写函数] [NOTIFY 信号名] [CONSTANT] [FINAL])
    QML可绑定属性当日志内容更新后,手动 emit requestLogChanged();
    Qt 收到信号，所有绑定该属性的 QML 控件会自动刷新界面。*/
    Q_PROPERTY(QString requestLog READ requestLog NOTIFY requestLogChanged)
    Q_PROPERTY(GlobalConstants::ModelType currentModelType READ currentModelType WRITE setCurrentModelType NOTIFY currentModelTypeChanged)
    Q_PROPERTY(QStringList modelList READ modelList NOTIFY modelListChanged)
    Q_PROPERTY(QString receiveContent READ receiveContent NOTIFY receiveContentChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)

public:
    explicit Net_test_VM(QObject *parent = nullptr);

    // 外部读写接口
    GlobalConstants::ModelType currentModelType() const;//当前大模型
    void setCurrentModelType(GlobalConstants::ModelType type);//设置当前大模型

    QStringList modelList() const;//子模型列表
    QString requestLog() const;//日志
    QString receiveContent() const;//ai返回内容
    bool isLoading() const;//是否正在加载

public slots:
//==============================
// QML 可调用槽函数（业务操作入口）
//==============================
    void loadModelList();                  // 拉取远端模型列表
    void sendChatRequest(const QString& userInput, const QString& selectModel); // 发送对话
    void cancelAllRequest();               // 取消网络请求

signals:
    // 状态变更信号，驱动QML刷新
    void currentModelTypeChanged();//当前模型类型已变更
    void modelListChanged();//子模型列表已变更
    void requestLogChanged();//日志已变更
    void receiveContentChanged();//接收内容已更改

    void isLoadingChanged();//加载状态已更改

private slots:
    // 内部信号接收槽
    void onModelListReady(GlobalConstants::ModelType type,const QStringList& models);//模型列表就绪
    void onStreamChunk(const QString& chunk);//流式数据块就绪
    void onRequestFinish(const QString& fullText);//请求完成
    void onRequestError(const QString& err);//网络请求错误

    // void onModelListFailed(const QString& err);//模型列表加载失败
    // //拉取子模型槽
    // //模型列表已获取
    // void onModelListFetched(GlobalConstants::ModelType type, const QStringList& models);
    // //模型列表获取失败
    // void onModelListFetchError(GlobalConstants::ModelType type, const QString& error);


private:
    // 成员变量
    GlobalConstants::ModelType m_curModelType;//当前大模型
    QStringList m_curModelList;//当前子模型列表
    QString m_requestLog;//输出日志
    QString m_receiveContent;//ai返回内容
    bool m_isLoading;//加载中

    BaseApiClient* m_tempClient = nullptr;//当前客户端

    QMap<GlobalConstants::ModelType,QStringList> m_modelList;//子模型列表

    //保存已创建客户端的列表缓存,可使用QSharedPointer<BaseApiClient>替代
    // QMap<GlobalConstants::ModelType,BaseApiClient*> m_modelTypeList;
    // BaseApiClient* getClient(GlobalConstants::ModelType ModelType);
};

#endif