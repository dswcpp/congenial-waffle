/*
* All rights reserved.
*
* ConfigInstance.h
*
* 初始版本：1.0
* 作者：duanshengwei
* 创建日期：2026年03月16日
* 摘要：配置接口
* 当前版本：2.0（移除 Qt 依赖）
*/
#ifndef CONFIGINSTANCE_H
#define CONFIGINSTANCE_H

#include "xmlsetting/XmlSettings.h"
#include "ConfigInfo.h"

#include <mutex>
#include <string>
#include <vector>

class ConfigInstance
{
public:
    ConfigInstance( Config::ConfigInfo* pInfo, Config::Mode eMode = Config::SINGLE, int iLoopCount = 5 );
    ~ConfigInstance();

    Config::ErrorCode load( const std::string& strFile );

    Config::ErrorCode restoreDefault( const std::vector<Config::GroupKey>& vecGroupKeys );
    Config::ErrorCode restoreDefault( Config::GroupCode eGroup );
    Config::ErrorCode restoreDefault();

    // 普通模式
    Config::ErrorCode beginGroup( Config::GroupCode eGroup );
    void endGroup();

    void setValue( double dbValue, Config::KeyCode eKey, Config::GroupCode eGroup = Config::CURRENT_GROUP );
    void setValue( const std::string& strValue, Config::KeyCode eKey, Config::GroupCode eGroup = Config::CURRENT_GROUP );

    void setDefaultValue( double dDefaultValue, Config::KeyCode eKey, Config::GroupCode eGroup = Config::CURRENT_GROUP );
    void setDefaultValue( const std::string& strDefaultValue, Config::KeyCode eKey, Config::GroupCode eGroup = Config::CURRENT_GROUP );

    std::string value( Config::KeyCode eKey, Config::GroupCode eGroup = Config::CURRENT_GROUP );

    void removeGroup( Config::GroupCode eGroup = Config::CURRENT_GROUP );

    // Array 模式
    int beginArray( Config::GroupCode eGroup );
    void beginItem( int iIndex );
    void endItem();
    void endArray();

    void setItemValue( double dbValue, Config::KeyCode eKey );
    void setItemValue( const std::string& strValue, Config::KeyCode eKey );

    std::string itemValue( Config::KeyCode eKey );

    Config::ErrorCode lastError() const;
    std::string lastErrorString() const;

    void save();

private:
    ConfigInstance(const ConfigInstance&) = delete;
    ConfigInstance& operator=(const ConfigInstance&) = delete;

    Config::ErrorCode checkKeyValueValid( const std::string& strValue, Config::KeyInfo* pInfo );
    Config::GroupInfo* getGroupInfo( Config::GroupCode eGroup );
    Config::KeyInfo* getKeyInfo( Config::KeyCode eKey, Config::GroupInfo* pGroupInfo );

    void saveDefault();
    void saveDefault( Config::GroupInfo* pGroupInfo );

    void setLastError( Config::ErrorCode code );
    bool isArrayMode() const;

    std::string getFileName( const std::string& strFileName, Config::Mode eMode, int iLoopCount );
    Config::ErrorCode initFromFile( const std::string& strFile );
    std::string getLoopFileName( const std::string& basePath, const std::string& baseName,
                                 const std::string& ext, int iLoopNumber );
    int getNumberFromFileName( const std::string& fileName );

    bool isItemBegun() const;

private:
    typedef struct _GroupItem
    {
        Config::GroupCode eGroup;
        Config::GroupInfo* pInfo;
    }GroupItem;

    int m_iCode;
    Config::ConfigInfo* m_pInfo;
    Config::Mode m_eMode;
    XMLSettings* m_pSetting;
    std::string m_strFileName;

    mutable std::recursive_mutex m_mutex;

    int m_iLoopCount;
    int m_iLoopNumber;
    Config::ErrorCode m_lastError;

    Config::GroupInfo* m_pCurrentGroup;

    std::vector<GroupItem> m_vecGroupEntered;
    std::vector<Config::GroupCode> m_vecGroupItemEntered;
};

#endif // CONFIGINSTANCE_H
