/*
* Copyright (c) 2016.08，南京华乘电气科技有限公司
* All rights reserved.
*
* ConfigManager.h
*
* 初始版本：1.0
* 作者：周定宇
* 创建日期：2016年08月12日
* 修改日期：2016年11月29日
* 修改人：邵震宇
* 摘要：配置文件管理接口
* 当前版本：1.0
*/
/*
 * 配置模块说明
 * 1、支持以下模式：
 *   1）全局一个文件
 *   2）全局多个文件
 *   3）可动态切换文件
 * 2、可选择性进行循环存储：
 *   可配置循环的份数，每次存储时，更新存储序号，可回退
 * 3、每一份配置文件，用配置表来初始化，可批量添加（用于多个模块使用）
 */

#ifndef CONFIGMANAGE_H
#define CONFIGMANAGE_H

#include <QObject>
#include <QMutex>
#include "ConfigInfo.h"
#include "ConfigInstance.h"
#include "service_global.h"
#include "log/LogManager.h"


#define CONFIG_LOG() \
    if( NULL == ConfigManager::instance()->logger()) {} \
    else  ConfigManager::instance()->logger()->error()<< __FILE__ << '@' << __LINE__

#define CONFIG_DEFAULT -1
class SERVICESHARED_EXPORT ConfigManager
{
public:
    /************************************************
     * 功能：单例
     ************************************************/
    static ConfigManager* instance();

    /************************************************
     * 功能：添加单一型配置文件
     * 输入参数：
     *      pInfo -- 配置文件的配置信息
     *      eMode -- 模式：单独/Loop
     *      iLoopCount -- Loop模式时的个数
     * 返回：
     *      ERROR_NONE -- 成功
     *      其它 -- 失败
     ************************************************/
    Config::ErrorCode addConfig( Config::ConfigInfo* pInfo, Config::Mode eMode = Config::SINGLE, int iLoopCount = 0 );

    /************************************************
     * 功能：获取配置文件句柄
     * 输入参数：
     *      iConfigCode -- 配置文件号
     * 返回：
     *      NULL -- 不存在
     *      其它 -- 配置文件句柄
     ************************************************/
    ConfigInstance* config( int iConfigCode = CONFIG_DEFAULT );

    /************************************************
     * 功能：设置日志模块
     * 输入参数：
     *      pLogger -- 日志模块指针
     ************************************************/
    void setLogger( Logger* pLogger );

    /************************************************
     * 功能：获取日志模块句柄
     * 返回：
     *      NULL -- 不存在
     *      其它 -- 日志模块句柄
     ************************************************/
    Logger* logger();
private:
    /************************************************
     * 功能：构造函数
     ************************************************/
    ConfigManager();

    /****************************
    功能： disable 拷贝
    *****************************/
    ConfigManager( const ConfigManager& other );

    /****************************
    功能： disable 赋值
    *****************************/
    ConfigManager & operator = (const ConfigManager &);

    /************************************************
     * 功能：析构函数
     ************************************************/
    ~ConfigManager();
private:
    static QMap<int,ConfigInstance*> m_mapConfigs;//配置文件
    static QMutex m_mutex;//锁
    Logger* m_pLog;//日志模块
};

#endif // CONFIGMANAGE_H
