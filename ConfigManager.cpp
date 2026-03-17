#include "ConfigManager.h"

ConfigManager::ConfigManager()
{
}

ConfigManager* ConfigManager::instance()
{
    // Meyer's Singleton: C++11 保证局部 static 变量的线程安全初始化
    static ConfigManager configManager;
    return &configManager;
}

ConfigManager::~ConfigManager()
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);
    for( auto& kv : m_mapConfigs )
    {
        kv.second->save();
    }
    m_mapConfigs.clear();
}

Config::ErrorCode ConfigManager::addConfig( Config::ConfigInfo* pInfo,
                                             Config::Mode eMode,
                                             int iLoopCount )
{
    Config::ErrorCode code = Config::ERROR_NONE;
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    if( nullptr == pInfo )
    {
        code = Config::INPUT_ERROR;
    }
    else if( ( Config::LOOP == eMode ) && ( 0 == iLoopCount ) )
    {
        code = Config::INPUT_ERROR;
    }
    else
    {
        if( m_mapConfigs.count( pInfo->iCode ) > 0 )
        {
            code = Config::CONFIG_ALREADY_EXIST;
        }
        else
        {
            m_mapConfigs[pInfo->iCode] = std::make_unique<ConfigInstance>( pInfo, eMode, iLoopCount );
        }
    }

    return code;
}

ConfigInstance* ConfigManager::config( int iConfigCode )
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    auto it = m_mapConfigs.find( iConfigCode );
    if( it != m_mapConfigs.end() )
        return it->second.get();

    if( ( CONFIG_DEFAULT == iConfigCode ) && ( !m_mapConfigs.empty() ) )
        return m_mapConfigs.begin()->second.get();

    return nullptr;
}
