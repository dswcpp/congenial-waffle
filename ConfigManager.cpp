#include <QDebug>
#include "ConfigManager.h"
QMap<int,ConfigInstance*> ConfigManager::m_mapConfigs;
QMutex ConfigManager::m_mutex(QMutex::Recursive);

/************************************************
 * 功能：构造函数
 ************************************************/
ConfigManager::ConfigManager()
{
    m_pLog = NULL;
}

/************************************************
 * 功能：单例
 ************************************************/
ConfigManager* ConfigManager::instance()
{
    m_mutex.lock();

    static ConfigManager configManager;
    ConfigManager* pInstance = &configManager;

    m_mutex.unlock();

    return pInstance;
}

/************************************************
 * 功能：析构函数
 ************************************************/
ConfigManager::~ConfigManager()
{
    m_mutex.lock();

    QMapIterator<int,ConfigInstance*> i( m_mapConfigs );
    while( i.hasNext() )
    {
        i.next();
        i.value()->save();
        delete i.value();
    }
    m_mapConfigs.clear();

    m_mutex.unlock();
}

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
Config::ErrorCode ConfigManager::addConfig( Config::ConfigInfo* pInfo, Config::Mode eMode, int iLoopCount )
{
    Config::ErrorCode code = Config::ERROR_NONE;

    m_mutex.lock();

    if( NULL == pInfo )
    {
        code = Config::INPUT_ERROR;
        CONFIG_LOG() << "Input error, NULL pointer!";
    }
    else if( ( Config::LOOP == eMode ) && ( 0 == iLoopCount ) )
    {
        code = Config::INPUT_ERROR;
        CONFIG_LOG() << "Input error, loop mode cannot set loopcount to 0!";
    }
    else
    {
        if( m_mapConfigs.contains( pInfo->iCode ) )
        {
            code = Config::CONFIG_ALREADY_EXIST;
        }
        else
        {
            ConfigInstance* pInstance = new ConfigInstance( pInfo, eMode, iLoopCount );
            m_mapConfigs[pInfo->iCode] = pInstance;
        }
    }

    m_mutex.unlock();

    return code;
}

/************************************************
 * 功能：获取配置文件句柄
 * 输入参数：
 *      iConfigCode -- 配置文件号
 * 返回：
 *      NULL -- 不存在
 *      其它 -- 配置文件句柄
 ************************************************/
ConfigInstance* ConfigManager::config( int iConfigCode )
{
    ConfigInstance* pInstance = NULL;

    m_mutex.lock();

    if( m_mapConfigs.contains( iConfigCode ) )
    {
        pInstance = m_mapConfigs[iConfigCode];
    }
    else if( ( CONFIG_DEFAULT == iConfigCode ) && ( m_mapConfigs.count() > 0 ) )
    {
        QList<int> keys = m_mapConfigs.keys();
        pInstance = m_mapConfigs[keys.at(0)];
    }

    m_mutex.unlock();

    return pInstance;
}


/************************************************
 * 功能：设置日志模块
 * 输入参数：
 *      pLogger -- 日志模块指针
 ************************************************/
void ConfigManager::setLogger( Logger* pLogger )
{
    m_pLog = pLogger;
}

/************************************************
 * 功能：获取日志模块句柄
 * 返回：
 *      NULL -- 不存在
 *      其它 -- 日志模块句柄
 ************************************************/
Logger* ConfigManager::logger()
{
    return m_pLog;
}
