#include "Utils.h"

#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include <cmath>
#include <QString>
#include <QImageReader>

#include "DeviceHelper.h"
#include "ExceptionHandler.h"

// ---------- 字符串与提示词模板处理 ----------
QString Utils::replacePromptVariables(const QString& templateStr, const QVariantMap& vars)
{
    // result 保存最终输出副本
    QString result = templateStr;

    // 正则：匹配 {{xxx}}，捕获组1取出中间变量名 xxx
    // \\{\\{ 匹配 {{；(\\w+)} 捕获变量名；\\}\\} 匹配 }}
    QRegularExpression re("\\{\\{(\\w+)\\}\\}");

    // globalMatch：在原始 templateStr 上找出全部 {{xxx}} 匹配，生成迭代器
    QRegularExpressionMatchIterator it = re.globalMatch(templateStr);

    // offset：偏移修正量，记录前面替换后字符串长度发生的变化
    int offset = 0;
    while (it.hasNext()) {
        // 取出下一个匹配结果，注意：match里面的位置全部是【原始templateStr】的下标
        QRegularExpressionMatch match = it.next();
        QString key = match.captured(1);// 获取变量名（捕获组1）
        if (vars.contains(key)) {
            QString value = vars[key].toString();
            /*
             match.capturedStart(0)：占位符在原始字符串中的起始下标
             + offset：加上前面替换造成的长度偏移，得到在 result 当前字符串里真实起始位置
             match.capturedLength(0)：原始占位符 {{xxx}} 的字符长度
             在 result 副本中做原地替换
            */
            result.replace(match.capturedStart(0) + offset, match.capturedLength(0), value);
            // 更新偏移：新值长度 − 旧占位符长度
            // 如果value比占位符长，offset变大；短则offset变小，用于修正后面所有匹配的位置
            offset += value.length() - match.capturedLength(0);
        }
    }
    return result;
}

QString Utils::escapeMarkdown(const QString& text)
{
    QString escaped = text;
    // 转义 Markdown 特殊字符
    QList<QChar> specials = {'\\', '`', '*', '_', '{', '}', '[', ']', '(', ')', '#', '+', '-', '.', '!', '|'};
    for (QChar ch : specials) {
        escaped.replace(ch, QString("\\") + ch);
    }
    return escaped;
}

QString Utils::unescapeMarkdown(const QString& text)
{
    QString unescaped = text;
    QRegularExpression re("\\\\([\\\\`*_{}\\[\\]()#+\\-.!|])");
    unescaped.replace(re, "\\1");
    return unescaped;
}

QString Utils::truncateTextByToken(const QString& text, int maxToken)
{
    // 简单近似：按字符数估算，中文约1字符≈1 token，英文约1 token≈4字符
    // 实际应使用模型 tokenizer，此处仅作示例
    if (maxToken <= 0) return QString();
    int estimatedTokens = text.length() / 2; // 粗略估算
    if (estimatedTokens <= maxToken)
        return text;
    // 保守截断
    int charLimit = maxToken * 2;
    return text.left(charLimit);
}

QList<QString> Utils::splitTextBySentence(const QString& text)
{
    QList<QString> sentences;
    // 按句号、问号、感叹号、换行分割
    QRegularExpression re("([^。!?\\n]+[。!?\\n])");
    QRegularExpressionMatchIterator it = re.globalMatch(text);
    while (it.hasNext()) {
        sentences.append(it.next().captured(1).trimmed());
    }
    if (sentences.isEmpty() && !text.isEmpty()) {
        sentences.append(text);
    }
    return sentences;
}

QString Utils::stringListToJson(const QStringList &stringList)
{
    QJsonArray array;
    for(const QString& item : stringList)
    {
        array.append(item);
    }
    return QJsonDocument(array).toJson(QJsonDocument::Compact);
}

QStringList Utils::jsonToStringList(const QVariant &fieldValue)
{
    QStringList result;
    QByteArray jsonData = fieldValue.toByteArray();

    // 空字段直接返回空列表
    if (jsonData.isEmpty()) {
        // 兼容字段是 QString 类型的情况
        QString str = fieldValue.toString();
        if (str.isEmpty()) return result;
        jsonData = str.toUtf8();
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
    if (error.error != QJsonParseError::NoError || !doc.isArray()) {
        return result;
    }

    QJsonArray array = doc.array();
    for (const QJsonValue& val : array) {
        if (val.isString()) {
            result.append(val.toString());
        }
    }
    return result;
}

QString Utils::variantMapToJson(const QVariantMap &map)
{
    QJsonObject obj = QJsonObject::fromVariantMap(map);
    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

QVariantMap Utils::jsonToVariantMap(const QVariant &fieldValue)
{
    QVariantMap result;
    QByteArray jsonData = fieldValue.toByteArray();

    // 兼容字段为 QString 类型的情况
    if (jsonData.isEmpty()) {
        QString str = fieldValue.toString();
        if (str.isEmpty()) return result;
        jsonData = str.toUtf8();
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(jsonData, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return result;
    }

    return doc.object().toVariantMap();
}

// ---------- 角色卡 PNG 元数据处理 ----------
QJsonObject Utils::extractPngMetadata(const QString& imagePath)
{
    //加载图片
    QImage image;
    if (!image.load(imagePath)) {
        qWarning() << "图像加载失败:" << imagePath;
        return QJsonObject();
    }

    qDebug()<<"当前图片文本键为:"<<image.textKeys();

    // 读取 "chara" 键对应的 tEXt 块(chara)数据
    QString meta64 = image.text(GlobalConstants::CHARACTER_CARD_META_KEY);
    if (meta64.isEmpty()) {
        qWarning() << "在PNG中找不到chara元数据：" << imagePath;
        return QJsonObject();
    }

    //解码base64
    QByteArray metaBA = QByteArray::fromBase64(meta64.toUtf8());
    // 解析 JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(metaBA, &parseError);
    //容错校验
    if (parseError.error != QJsonParseError::NoError) {
        qWarning() << "解析字符元数据JSON失败:" << parseError.errorString();
        return QJsonObject();
    }
    if (!doc.isObject()) {
        qWarning() << "元数据不是JSON对象";
        return QJsonObject();
    }
    return doc.object();
}

bool Utils::injectPngMetadata(const QString& imagePath, const QJsonObject& metadata)
{
    QImage image;
    if (!image.load(imagePath)) {
        qWarning() << "Failed to load image for metadata injection:" << imagePath;
        return false;
    }

    // 保留原有所有文本元数据（避免覆盖其他自定义 tEXt 块）
    QStringList existingKeys = image.textKeys();
    for (const QString& key : existingKeys) {
        // 仅当不是 "chara" 时才保留，因为我们即将替换 chara 块
        if (key != GlobalConstants::CHARACTER_CARD_META_KEY) {
            image.setText(key, image.text(key));
        }
    }

    // 写入新的 chara 元数据
    QJsonDocument doc(metadata);
    QString metaStr = QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
    image.setText(GlobalConstants::CHARACTER_CARD_META_KEY, metaStr);

    // 保存图片（PNG 格式，保留原有图像质量）
    if (!image.save(imagePath, "PNG")) {
        qWarning() << "Failed to save image with injected metadata:" << imagePath;
        return false;
    }

    return true;
}

QJsonObject Utils::convertCharacterV1ToV2(const QJsonObject& v1Data)
{
    QJsonObject v2;
    // V1 典型结构: { "name": "...", "description": "...", "personality": "...", "first_mes": "...", "mes_example": "...", "creatorcomment": "...", "data": {...} }
    // V2 结构: { "data": { "name": "...", "description": "...", "personality": "...", "first_mes": "...", "mes_example": "...", "creator_notes": "...", "extensions": {...} }, "spec": "chara_card_v2", "spec_version": "2.0" }
    v2["spec"] = "chara_card_v2";
    v2["spec_version"] = "2.0";

    QJsonObject data;
    data["name"] = v1Data["name"];
    data["description"] = v1Data["description"];
    data["personality"] = v1Data["personality"];
    data["first_mes"] = v1Data["first_mes"];
    data["mes_example"] = v1Data["mes_example"];
    data["creator_notes"] = v1Data["creatorcomment"];

    QJsonObject extensions;
    if (v1Data.contains("data") && v1Data["data"].isObject()) {
        QJsonObject oldData = v1Data["data"].toObject();
        // 保留旧扩展字段
        for (auto it = oldData.begin(); it != oldData.end(); ++it) {
            extensions[it.key()] = it.value();
        }
    }
    data["extensions"] = extensions;
    v2["data"] = data;
    return v2;
}

QString Utils::chatUrlTextSuffix(const QString &urlStr)
{
    // 确保 URL 以 /chat/completions 结尾
    if (!urlStr.endsWith("/chat/completions")) {
        QString path = urlStr;
        if (!path.endsWith('/')) {
            path += '/';
        }
        path += "chat/completions";
        return path;
    }
    return urlStr;
}

QString Utils::UrlTextPrefix(const QString &urlStr)
{
    // 确保 URL 以 https:// 开头
    if (!urlStr.startsWith("https://")) {
        QString path = "https://";
        path += urlStr;
        return path;
    }
    return urlStr;
}

// ---------- JSON 序列化工具 ----------
bool Utils::writeJsonFile(const QString& filePath, const QJsonDocument& doc)
{
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "无法打开文件进行写入:" << filePath;
        return false;
    }
    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        qWarning() << "提交JSON文件失败:" << filePath;
        return false;
    }
    return true;
}

QJsonDocument Utils::readJsonFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开文件进行读取:" << filePath;
        return QJsonDocument();
    }
    QByteArray data = file.readAll();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qWarning() << "文件中的JSON解析错误:" << filePath << error.errorString();
        return QJsonDocument();
    }
    return doc;
}

// QString Utils::PromptOrderToJsonArr(const QList<PromptOrderEntry> & promptOrder)
// {
//     QJsonArray arr;
//     for(const auto& p : promptOrder)
//     {
//         QJsonObject obj;
//         obj["identifier"] = p.identifier;
//         obj["enabled"] = p.enabled;

//         arr.append(obj);
//     }
//     QJsonDocument doc(arr);
//     return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));;
// }

QVariantMap Utils::jsonObjectToVariantMap(const QJsonObject &obj)
{
    return QVariantMap();
}

QJsonObject Utils::variantMapToJsonObject(const QVariantMap &map)
{
    return QJsonObject();
}

QString Utils::jsonToPrettyString(const QJsonObject& obj)
{
    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

// ---------- 双端适配文件路径 ----------
QString Utils::getAppDataPath()
{
    QString path=DeviceHelper::instance().getAppDataPath();
    ensureDirExists(path);
    return path;
}

QString Utils::getAvatarDirPath()
{
    QString dirPath = getAppDataPath() + "/" + GlobalConstants::AVATARS_DIR;
    ensureDirExists(dirPath);
    return dirPath;
}


bool Utils::ensureDirExists(const QString& dirPath)
{
    QDir dir(dirPath);
    if (dir.exists())//判断是否存在
        return true;
    if (dir.mkpath(".")) {//自动创建目录
        qDebug()<<"创建:"<<dir<<"目录";
        return true;
    } else {
        qWarning() << "无法创建目录:" << dirPath;
        return false;
    }
}

// ---------- Token 计算（适配多模型）----------
int Utils::calculateToken(const QString& text, GlobalConstants::ModelType modelType)
{
    // 简单估算：不同模型 tokenizer 差异较大，此处提供通用近似算法
    // 实际应集成 tiktoken (cl100k_base) 或调用模型 API 的计数接口
    Q_UNUSED(modelType);
    // 粗略计数：中文/日文/韩文字符按1个token，英文单词按1个token，标点按0.5平均
    int tokenCount = 0;
    bool inWord = false;
    for (QChar ch : text) {
        if (ch.isLetterOrNumber()) {
            if (!inWord) {
                inWord = true;
                tokenCount++;
            }
        } else {
            inWord = false;
            if (ch.isPunct() || ch.isSpace()) {
                tokenCount += 0.5; // 标点和空格算半个token
            } else {
                tokenCount++;
            }
        }
    }
    return static_cast<int>(std::ceil(tokenCount));
}

int Utils::calculateMessagesToken(const QJsonArray& messages, GlobalConstants::ModelType modelType)
{
    int total = 0;
    for (const QJsonValue& val : messages) {
        if (!val.isObject()) continue;
        QJsonObject obj = val.toObject();
        // 每个消息的 role 和 content 都有开销
        total += 4; // 基础开销
        if (obj.contains("content") && obj["content"].isString()) {
            total += calculateToken(obj["content"].toString(), modelType);
        }
        if (obj.contains("name") && obj["name"].isString()) {
            total += calculateToken(obj["name"].toString(), modelType);
        }
    }
    return total;
}

QString Utils::avatarPathToUrl(const QString & avatarPath)
{
    // qDebug()<<"调用头像"<<m_avatarRelativePath;
    // return "qrc:/default/res/user-circle.png";


    // 空路径返回默认头像资源
    if (avatarPath.isEmpty()) {
        return "qrc:/default/res/default/user-hexagon.svg";
    }
    // 相对路径拼接应用头像目录，生成绝对路径供QML直接加载
    QString avatarDir = Utils::getAvatarDirPath();
    QString avatarUrl("file:///"+QDir(avatarDir).filePath(avatarPath));
    return avatarUrl;
}

QString Utils::formatTime(const QDateTime& time)
{
    return time.toString("yyyy-MM-dd hh:mm:ss");
}


void Utils::errorOccurred(const QString &message)
{
    emit ExceptionHandler::instance().errorOccurred(message);
}

void Utils::writeLog(const QString& level, const QString& module,const QString& message)
{
    //获取当前时间
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    //日志行
    QString logLine = QString("[%1] [%2] [%3] %4").arg(timestamp, level, module, message);
    // 简单输出到调试输出，实际可写入文件
    qDebug().noquote() << logLine;

    //写入日志文件
    static QString logFilePath =
        getAppDataPath() + "/logs/" + QDate::currentDate().toString("yyyy-MM-dd") + ".log";
    static bool logDirCreated = ensureDirExists(getAppDataPath() + "/logs");
    if (logDirCreated) {
        QFile file(logFilePath);
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            file.write((logLine + "\n").toUtf8());
            file.close();
        }
    }
}
