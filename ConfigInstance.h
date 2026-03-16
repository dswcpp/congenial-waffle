/*
* Copyright (c) 2016.08，南京华乘电气科技有限公司
* All rights reserved.
*
* Config.h
*
* 初始版本：1.0
* 作者：邵震宇
* 创建日期：2016年11月29日
* 摘要：配置接口
* 当前版本：1.0
*/
/*
 * 配置模块说明
 * 1、支持启动时合法性校验，并给出校验结果，非法时，自动使用默认参数
 * 2、可全部或部分恢复设置参数为默认参数
 * 4、利用键值和组号来实现对参数的访问
 * 5、支持Array模式（用于同类别，但是数目不定的场景）
 *   Array模式场景：1）wifi的账号密码，wifi组数不定
 *                2）在线监测中，设备的信息，设备的数组不定
 */
#ifndef CONFIGINSTANCE_H
#define CONFIGINSTANCE_H
#include "xmlsetting/XmlSettings.h"
#include "ConfigInfo.h"
#include "service_global.h"
class SERVICESHARED_EXPORT ConfigInstance
{
public:
    /************************************************
     * 功能：构造函数
     * 输入参数：
     *      pInfo -- 配置
     *      eMode -- 模式
     *      iLoopCount -- Loop模式时的个数
     ************************************************/
    ConfigInstance( Config::ConfigInfo* pInfo, Config::Mode eMode = Config::SINGLE, int iLoopCount = 5 );

    /************************************************
     * 功能：析构函数
     ************************************************/
    ~ConfigInstance();

    /************************************************
     * 功能：载入配置文件
     * 输入参数：
     *      strFile -- 文件名
     * 返回：
     *      ERROR_NONE -- 成功
     *      其它 -- 失败
     ************************************************/
    Config::ErrorCode load( const QString& strFile );

    /************************************************
     * 功能：将部分参数恢复默认
     * 输入参数：
     *      vecGroupKeys -- 所要恢复的组键
     * 返回：
     *      ERROR_NONE -- 成功
     *      其它 -- 失败
     ************************************************/
    Config::ErrorCode restoreDefault( const QVector<Config::GroupKey>& vecGroupKeys );

    /************************************************
     * 功能：将部分参数恢复默认
     * 输入参数：
     *      eGroup -- 所要恢复的组键
     * 返回：
     *      ERROR_NONE -- 成功
     *      其它 -- 失败
     ************************************************/
    Config::ErrorCode restoreDefault( Config::GroupCode eGroup );

    /************************************************
     * 功能：将所有参数恢复默认
     * 返回：
     *      ERROR_NONE -- 成功
     *      其它 -- 失败
     ************************************************/
    Config::ErrorCode restoreDefault();

    /*********/
    //常规模式
    /************************************************
     * 功能：进入普通组
     * 输入参数：
     *      eGroup -- 组号
     * 返回：
     *      ERROR_NONE -- 成功
     *      其它 -- 失败
     ************************************************/
    Config::ErrorCode beginGroup( Config::GroupCode eGroup );

    /************************************************
     * 功能：退出进入的普通组
     ************************************************/
    void endGroup();

    /************************************************
     * 功能：设置值
     * 输入参数：
     *      dbValue, strValue -- 值
     *      eKey -- 键值
     *      eGroup -- 组号
     ************************************************/
    void setValue( double dbValue, Config::KeyCode eKey, Config::GroupCode eGroup = Config::CURRENT_GROUP );
    void setValue( const QString& strValue, Config::KeyCode eKey, Config::GroupCode eGroup = Config::CURRENT_GROUP );

    /************************************************
     * 功能：设置默认值
     * 输入参数：
     *      dDefaultValue, strDefaultValue -- 默认值
     *      eKey -- 键值
     *      eGroup -- 组号
     ************************************************/
    void setDefaultValue(double dDefaultValue, Config::KeyCode eKey, Config::GroupCode eGroup = Config::CURRENT_GROUP);
    void setDefaultValue(const QString& strDefaultValue, Config::KeyCode eKey, Config::GroupCode eGroup = Config::CURRENT_GROUP);

    /************************************************
     * 功能：获取值
     * 输入参数：
     *      eKey -- 键值
     *      eGroup -- 组号
     * 返回：值
     ************************************************/
    QString value( Config::KeyCode eKey, Config::GroupCode eGroup = Config::CURRENT_GROUP );

    /*************************************************
    函数名： removeGroup
    输入参数： eGroup -- 组号
    输出参数： NULL
    返回值： NULL
    功能： 删除group
    *************************************************************/
    void removeGroup(Config::GroupCode eGroup = Config::CURRENT_GROUP);
    //常规模式
    /*********/

    /*********/
    //Array模式
    /************************************************
     * 功能：进入Array模式组
     * 输入参数：
     *      eGroup -- 组号
     * 返回：
     *      array中的成员个数
     ************************************************/
    int beginArray( Config::GroupCode eGroup );

    /************************************************
     * 功能：开始一个条目
     ************************************************/
    void beginItem( int iIndex );

    /************************************************
     * 功能：结束一个条目
     ************************************************/
    void endItem();

    /************************************************
     * 功能：退出进入的Array组
     ************************************************/
    void endArray();

    /************************************************
     * 功能：设置数据
     * 输入参数：
     *      dbValue, strValue -- 值
     *      eKey -- 键值
     ************************************************/
    void setItemValue( double dbValue, Config::KeyCode eKey );
    void setItemValue( const QString& strValue, Config::KeyCode eKey );

    /************************************************
     * 功能：获取值
     * 输入参数：
     *      eKey -- 键值
     * 返回：值
     ************************************************/
    QString itemValue( Config::KeyCode eKey  );
    /*********/

    /************************************************
     * 功能：获取最近一次错误码
     * 返回：
     *      错误码
     ************************************************/
    Config::ErrorCode lastError() const;

    /************************************************
     * 功能：获取最近一次错误码
     * 返回：
     *      错误码
     ************************************************/
    QString lastErrorString() const;

    /************************************************
     * 功能：保存至文件
     ************************************************/
    void save();
private:
    ConfigInstance(const ConfigInstance&);            // not available
    ConfigInstance& operator=(const ConfigInstance&); // not available

    /************************************************
     * 功能：合法性校验，检查键值的合法性
     * 输入参数：
     *      strValue -- 键值
     *      pInfo -- 键配置
     * 返回：
     *      错误码
     ************************************************/
    Config::ErrorCode checkKeyValueValid(const QString& strValue, Config::KeyInfo *pInfo );

    /************************************************
     * 功能：获取组配置
     * 输入参数：
     *      eGroup -- 组号
     * 返回：
     *      组配置
     ************************************************/
    Config::GroupInfo* getGroupInfo( Config::GroupCode eGroup );

    /************************************************
     * 功能：获取键配置
     * 输入参数：
     *      eKey -- 键值
     *      pGroupInfo -- 组配置
     * 返回：
     *      键配置
     ************************************************/
    Config::KeyInfo *getKeyInfo( Config::KeyCode eKey, Config::GroupInfo* pGroupInfo );

    /************************************************
     * 功能：保存默认数据入文件
     ************************************************/
    void saveDefault();
    /************************************************
     * 功能：保存默认数据入文件
     * 输入参数：
     *      pGroupInfo -- 组配置
     ************************************************/
    void saveDefault(Config::GroupInfo *pGroupInfo );

    /************************************************
     * 功能：设置最后一次错误码
     * 输入参数：
     *      code -- 错误码
     ************************************************/
    void setLastError( Config::ErrorCode code );

    /************************************************
     * 功能：判断是否Array模式
     * 返回：
     *      true: Array模式
     *      false:Normal模式
     ************************************************/
    bool isArrayMode() const;

    /************************************************
     * 功能：根据配置生成文件名
     * 输入参数：
     *      strFileName -- 配置文件名
     *      eMode -- 模式
     *      iLoopCount -- Loop的个数
     ************************************************/
    QString getFileName( const QString& strFileName, Config::Mode eMode, int iLoopCount );

    /************************************************
     * 功能：从配置文件中初始化
     * 输入参数：
     *      strFile -- 文件名
     * 返回：
     *      ERROR_NONE -- 成功
     *      其它 -- 失败
     ************************************************/
    Config::ErrorCode initFromFile( const QString& strFile );

    /************************************************
     * 功能：从序号中获取对应文件名
     * 输入参数：
     *      fileInfo -- 当前文件信息
     *      iLoopNumber -- 循环序号
     * 返回：
     *      文件名
     ************************************************/
    QString getLoopFileName( const QFileInfo& fileInfo, int iLoopNumber );

    /************************************************
     * 功能：从文件名中获取序号
     * 输入参数：
     *      fileName -- 文件名
     * 返回：
     *      序号
     ************************************************/
    int getNumberFromFileName( const QString& fileName );

    /************************************************
     * 功能：判断有无进入合法beginItem模式
     * 返回：
     *      true：进入beginItem模式
     *      false：没有
     ************************************************/
    bool isItemBegun()const;
private:
    //组条目
    typedef struct _GroupItem
    {
        Config::GroupCode eGroup;
        Config::GroupInfo* pInfo;
    }GroupItem;

    int m_iCode;//配置号
    Config::ConfigInfo* m_pInfo;//配置信息
    Config::Mode m_eMode;//模式
    XMLSettings* m_pSetting;//配置文件
    QString m_strFileName;//文件名

    QMutex m_mutex;//锁

    int m_iLoopCount;//循环模式的循环个数
    int m_iLoopNumber;//循环模式的当前序号
    Config::ErrorCode m_lastError;//最近一次错误码

    Config::GroupInfo* m_pCurrentGroup;//当前组信息

    QVector<GroupItem> m_vecGroupEntered;//已进入的组记录
    QVector<Config::GroupCode> m_vecGroupItemEntered;//已经进入条目模式的组记录
};

#endif // CONFIG_H
