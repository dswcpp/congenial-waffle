/*
* All rights reserved.
*
* ConfigManager.h
*
* 初始版本：1.0
* 作者：duanshengwei
* 创建日期：2026年03月16日
* 摘要：配置文件管理接口
* 当前版本：2.0（移除 Qt 依赖）
*/
#ifndef CONFIGMANAGE_H
#define CONFIGMANAGE_H

#include <map>
#include <mutex>

#include "ConfigInfo.h"
#include "ConfigInstance.h"

// 日志宏：若外部注入了 Logger，则使用；否则为空操作。
// Logger 接口由调用方提供，此处仅保留宏的形态以保持兼容。
#define CONFIG_LOG() if(false) (void)0 //(void)

#define CONFIG_DEFAULT -1

class ConfigManager
{
public:
    static ConfigManager* instance();

    Config::ErrorCode addConfig( Config::ConfigInfo* pInfo,
                                  Config::Mode eMode = Config::SINGLE,
                                  int iLoopCount = 0 );

    ConfigInstance* config( int iConfigCode = CONFIG_DEFAULT );

private:
    ConfigManager();
    ConfigManager( const ConfigManager& ) = delete;
    ConfigManager& operator=( const ConfigManager& ) = delete;
    ~ConfigManager();

private:
    static std::map<int, ConfigInstance*> m_mapConfigs;
    static std::recursive_mutex m_mutex;
};

#endif // CONFIGMANAGE_H
