#include "ConfigManager.h"

std::map<int, ConfigInstance*> ConfigManager::m_mapConfigs;
std::recursive_mutex ConfigManager::m_mutex;

ConfigManager::ConfigManager()
{
}

ConfigManager* ConfigManager::instance()
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);
    static ConfigManager configManager;
    return &configManager;
}

ConfigManager::~ConfigManager()
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);
    for( auto& kv : m_mapConfigs )
    {
        kv.second->save();
        delete kv.second;
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
            m_mapConfigs[pInfo->iCode] = new ConfigInstance( pInfo, eMode, iLoopCount );
        }
    }

    return code;
}

ConfigInstance* ConfigManager::config( int iConfigCode )
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    auto it = m_mapConfigs.find( iConfigCode );
    if( it != m_mapConfigs.end() )
        return it->second;

    if( ( CONFIG_DEFAULT == iConfigCode ) && ( !m_mapConfigs.empty() ) )
        return m_mapConfigs.begin()->second;

    return nullptr;
}
