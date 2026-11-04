#ifndef UTILS_H
#define UTILS_H

#include <QString>
#include <QByteArray>
#include <QImage>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUuid>

#include "GlobalConstants.h"
#include "PromptEntity.h"
#include "CompletionPresetEntity.h"

class Utils
{
public:
// ======================================
// 双端适配文件路径
// ======================================
    //创建目录并确保目录存在
    static bool ensureDirExists(const QString& dirPath);
    //获取app数据路径
    static QString getAppDataPath();
    //获取头像目录路径
    static QString getAvatarDirPath();
    //获取配置文件路径(弃用)
    static QString getConfigFilePath(const QString& fileName);

// ======================================
// 角色卡专用处理
// ======================================
    //提取元数据
    static QJsonObject extractPngMetadata(const QString& imagePath);
    //注入元数据(未完成)
    static bool injectPngMetadata(const QString& imagePath, const QJsonObject& metadata);
    //转换v2和v3(未完成)
    static QJsonObject convertCharacterV1ToV2(const QJsonObject& v1Data);

// ======================================
// 字符串与提示词模板处理
// ======================================
    //判断chaturl前后缀(已废弃)
    static QString chatUrlTextSuffix(const QString& urlStr);
    static QString UrlTextPrefix(const QString& urlStr);
    //替换提示词变量
    static QString replacePromptVariables(const QString& templateStr, const QVariantMap& vars);
    //转义Markdown
    static QString escapeMarkdown(const QString& text);
    static QString unescapeMarkdown(const QString& text);
    //按token截断文本
    static QString truncateTextByToken(const QString& text, int maxToken);
    //按句子拆分文本
    static QList<QString> splitTextBySentence(const QString& text);


// ======================================
// JSON序列化工具
// ======================================
    //QStringList与JSON互转(再转QString)
    static QString stringListToJson(const QStringList& stringList);
    static QStringList jsonToStringList(const QVariant& fieldValue);
    // QVariantMap与JSON互转
    static QString variantMapToJson(const QVariantMap& map);
    static QVariantMap jsonToVariantMap(const QVariant& fieldValue);

    //写入json文件
    static bool writeJsonFile(const QString& filePath, const QJsonDocument& doc);
    //读取json文件
    static QJsonDocument readJsonFile(const QString& filePath);


    //PromptEntitys转换(弃用,保留函数)
    // static QString PromptOrderToJsonArr(const QList<PromptOrderEntry> & promptOrder);

    //json对象转变体map
    static QVariantMap jsonObjectToVariantMap(const QJsonObject& obj);
    //变体map转json对象
    static QJsonObject variantMapToJsonObject(const QVariantMap& map);
    //json转格式化
    static QString jsonToPrettyString(const QJsonObject& obj);



// ======================================
// Token计算适配多模型
// ======================================
    //计算token
    static int calculateToken(const QString& text, GlobalConstants::ModelType modelType);
    //计算消息token
    static int calculateMessagesToken(const QJsonArray& messages, GlobalConstants::ModelType modelType);

// ======================================
// 通用工具
// ======================================
    //路径转url
    static QString avatarPathToUrl(const QString& avatarPath);
    //生成uuid
    static QString generateUuid() { return QUuid::createUuid().toString(QUuid::WithoutBraces); }
    //格式时间
    static QString formatTime(const QDateTime& time);
    //发生错误
    static void errorOccurred(const QString& message);
    //写入日志
    //Utils::writeLog("ERROR","ChatService","失败");
    //qDebug()<<QString("测试消息");
    static void writeLog(const QString& level,const QString& module = "", const QString& message="");
};

#endif
