#ifndef WORLDINFOENTRYENTITY_H
#define WORLDINFOENTRYENTITY_H

#include <QString>
#include <QStringList>
#include <QVariantMap>
#include "GlobalConstants.h"

//世界书条目数据实体
enum InsertPosition
{
    RuleRole_Normal = 0,
    RuleRole_Admin = 1
};

struct WorldInfoEntryEntity {
    QString id;
    QString bookId;         //所属世界书id
    QString name;
    bool isEnabled = true;//是否启用

    // 关键词
    QStringList keywords;   //主要关键字
    QStringList secondaryKeywords;//次要关键字

    // 核心内容
    QString content;            //内容

    // 触发控制
    // int triggerControl = 1;     //触发策略(自定义)--0:蓝灯永久1:绿灯关键词2:向量触发
    bool constant = false;      //触发策略--恒定的(永久/关键词)
    bool vectorized = false;    //向量化(是/否)


    //=====插入控制=====
    enum Position {//位置枚举
        BeforeChar = 0,
        AfterChar = 1,
        ANTop = 2,
        ANBottom = 3,
        AtDepth = 4,
        EMTop = 5,
        EMBottom = 6,
        Outlet = 7
    };
    //插入位置(自定义)
    Position position = BeforeChar;
        /*--
         * 0:before:拼在角色描述前; 1:after:角色定义之后;
         * 4:atDepth:按depth插到聊天历史中;role在深度(根据角色role判断在什么角色插入);
         * 5:EMTop:示例消息之前;6:EMBottom:示例消息之后(同角色);
         * 2:ANTop:作者注释之前;3:ANBottom:作者注释之后(同角色);
         * 7:outlet:通过命名 outlet 输出（由扩展消费）*/

    //在什么角色(的深度)插入
    GlobalConstants::MessageType insertRole = GlobalConstants::MessageType::System;
        //--null/0/1/2   null:无 0:系统在深度 1:用户在深度  2:ai在深度
    //插入深度
    int insertDepth = 4;
        /*--当position(插入位置) = 4 时生效;
        注入深度，数值越小，content 越靠近系统提示头部（优先级越高），初始值为4*/
    //插入顺序
    int sortOrder = 100;
        //--匹配检索优先级，数值越小，ST 越早检索这条规则；order=1 > order=100优先匹配


    //其他功能
    bool useProbability = false;//启用概率触发
    int probability = 100;//触发概率(%)
    bool selective = true;//选择性逻辑开关(默认开启)

    enum SelectiveLogic {
        AND_ANY = 0,
        AND_ALL = 1,
        NOT_ANY = 2,
        NOT_ALL = 3
    };
    SelectiveLogic selectiveLogic = AND_ANY;//选择性逻辑(默认0)
        /*--
         * 0:AND_ANY:次关键词任意一个匹配; 1:AND_ALL:次关键词全部匹配;
         * 2:NOT_ANY:次关键词都不匹配;   3:NOT_ALL:次关键词不全部匹配*/

    int scanDepth = -1;    //扫描深度(默认为空,使用全局设置,-1)
        //扫描聊天历史的深度（消息条数）


    //未定义字段
    bool caseSensitive = false;     //区分大小写(默认为空,使用全局设置)
    bool matchWholeWords = false;   //完整单词--全词匹配(默认为空,使用全局设置)
    bool useGroupScoring = false;   //启用组评分
    //--null 使用全局;true 启用分组计分择优，同组只选高分条目；false 满足条件同组全注入
    QString automationId;          //自动化ID
    //--绑定 ST 自动化插件 ID，由自动化脚本控制触发；空 = 不绑定(默认为空)

    bool excludeRecursion = false;// 不可递归
    bool preventRecursion = false; //防止进一步递归
    bool delayUntilRecursion = false;//延迟到递归
    bool ignoreBudget = false;//无视回复限额


    //额外匹配来源(去哪找关键词)
    bool matchCharacterDescription = false;     //角色描述
    bool matchCharacterPersonality = false;     //角色性格
    bool matchScenario = false;                 //情景--匹配场景设定
    bool matchPersonaDescription = false;       //用户设定描述--匹配人设描述
    bool matchCharacterDepthPrompt = false;     //角色备注--匹配角色深度提示词
    bool matchCreatorNotes = false;             //创作者的注释--匹配创作者备注


    //分组配置
    QString group;          //分组名称--分组 ID 字符串，同编号 = 同一个分组；空 = 无分组
    bool groupOverride = false;     //确认优先级--覆盖分组设置
    int groupWeight = 0;        //分组权重

    int uid = 0;                //唯一标识--条目全局唯一 ID
    int displayIndex = 0;       //显示索引--世界书编辑界面列表上下排序序号，拖动条目自动修改
    int sticky = 0;             //粘性--持久标记
    int cooldown = 0;           //冷却
    int delay = 0;              //延迟
    //characterFilter       //json--绑定到角色和标签--角色卡过滤器
    //QStringList triggers;   //筛选生成触发器[],触发器列表
    //outletName;           //输出通道名称

    //addMemo添加备忘录
    //disable是否禁用--彻底禁用


    GlobalConstants::WorldInfoTriggerType triggerType
        = GlobalConstants::WorldInfoTriggerType::ExactMatch;    //世界书触发模式

    // 扩展
    QVariantMap extensions;
};

#endif // WORLDINFOENTRYENTITY_H