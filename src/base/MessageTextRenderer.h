#ifndef MESSAGETEXTRENDERER_H
#define MESSAGETEXTRENDERER_H

#include <QString>

/**
 * @brief 聊天消息的显示文本投影
 *
 * 聊天记录保存的是角色卡/用户输入的原始文本，`{{user}}`、`{{char}}`
 * 这类身份宏只在展示时替换。不要用渲染结果回写数据库：
 * 否则改名后旧消息无法跟着变，编辑时也会把模板弄丢。
 */
class MessageTextRenderer
{
public:
    /// 仅替换身份宏（{{user}}、{{char}} 及旧式 <USER>/<BOT>），
    /// 不执行 {{getvar}} / {{random}} 等有状态宏。
    static QString render(const QString& raw,
                          const QString& userName,
                          const QString& charName);
};

#endif // MESSAGETEXTRENDERER_H
