#include "ConfigInstance.h"
#include "ConfigManager.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef __linux__
#include <unistd.h>
#endif

namespace fs = std::filesystem;

//#define WRITE_VALID_VALUE_WHEN_OPERATE

static std::string arrayIndex(int index)
{
    return "Index" + std::to_string(index);
}

// ─── 构造/析构 ───────────────────────────────────────────────────────────────

ConfigInstance::ConfigInstance( Config::ConfigInfo* pInfo, Config::Mode eMode, int iLoopCount )
    : m_pInfo( pInfo ), m_eMode( eMode ), m_iLoopCount( iLoopCount ), m_iLoopNumber( 0 ),
      m_lastError( Config::ERROR_NONE ), m_pCurrentGroup( nullptr ), m_pSetting( nullptr )
{
    if( ( nullptr == pInfo ) || ( !pInfo->isValid() ) )
    {
        m_lastError = Config::INPUT_ERROR;
        CONFIG_LOG() << "ConfigInstance::ConfigInstance: info is NULL or invalid!";
    }
    else
    {
        m_iCode = pInfo->iCode;
        initFromFile( m_pInfo->strFileName );
    }
}

ConfigInstance::~ConfigInstance()
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);
    delete m_pSetting;
    m_pSetting = nullptr;
}

// ─── load / initFromFile ─────────────────────────────────────────────────────

Config::ErrorCode ConfigInstance::load( const std::string& strFile )
{
    m_lastError = Config::ERROR_NONE;
    Config::ErrorCode code = Config::ERROR_NONE;

    delete m_pSetting;
    m_pSetting = new XMLSettings( strFile );

    if( Config::LOOP == m_eMode )
    {
        m_iLoopNumber = getNumberFromFileName( strFile );
    }

#ifdef WRITE_VALID_VALUE_WHEN_OPERATE
    if( !fs::exists( strFile ) )
    {
        code = Config::FILE_NOT_EXIST;
        CONFIG_LOG() << "ConfigInstance: config file not exist!";
        restoreDefault();
    }
    else
#endif
    {
        m_pSetting->setRoot( m_pInfo->strRoot );
    }

    m_pCurrentGroup = nullptr;
    setLastError( code );
    return code;
}

Config::ErrorCode ConfigInstance::initFromFile( const std::string& strFile )
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);
    std::string strFileName = getFileName( strFile, m_eMode, m_iLoopCount );
    return load( strFileName );
}

// ─── restoreDefault ──────────────────────────────────────────────────────────

Config::ErrorCode ConfigInstance::restoreDefault()
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    m_pSetting->clear();
    for( int i=0; i<m_pInfo->iCount; i++ )
    {
        Config::GroupInfo* pInfo = m_pInfo->pGroups[i];
        m_pSetting->beginGroup( pInfo->strName );

        if( pInfo->eType == Config::ARRAY )
            m_pSetting->beginGroup( arrayIndex(0) );

        saveDefault( pInfo );

        if( pInfo->eType == Config::ARRAY )
            m_pSetting->endGroup();

        m_pSetting->endGroup();
    }
    m_pSetting->setRoot( m_pInfo->strRoot );
    m_pSetting->sync();

    return Config::ERROR_NONE;
}

Config::ErrorCode ConfigInstance::restoreDefault( const std::vector<Config::GroupKey>& vecGroupKeys )
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    Config::ErrorCode code = Config::ERROR_NONE;

    for( size_t i=0; i<vecGroupKeys.size(); i++ )
    {
        Config::GroupKey groupKey = vecGroupKeys[i];
        Config::GroupInfo* pGroupInfo = getGroupInfo( groupKey.group );
        Config::KeyInfo* pKeyInfo = getKeyInfo( groupKey.key, pGroupInfo );

        if( nullptr != pKeyInfo )
        {
            setValue( pKeyInfo->strDefaultValue, groupKey.key, groupKey.group );
        }
        else
        {
            code = Config::KEY_NOT_EXIST;
            setLastError( code );
            CONFIG_LOG() << "ConfigInstance::restoreDefault: invalid group/key";
        }
    }

    m_pSetting->sync();
    return code;
}

Config::ErrorCode ConfigInstance::restoreDefault( Config::GroupCode eGroup )
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );
    Config::ErrorCode code = Config::ERROR_NONE;

    if( nullptr == pGroupInfo )
    {
        code = Config::GROUP_NOT_EXIST;
        CONFIG_LOG() << "ConfigInstance::restoreDefault: group not exist!";
    }
    else
    {
        m_pSetting->remove( pGroupInfo->strName );
        if( Config::ARRAY != pGroupInfo->eType )
        {
            m_pSetting->beginGroup( pGroupInfo->strName );

            for( int i=0; i<pGroupInfo->iSubGroupCount; i++ )
                restoreDefault( pGroupInfo->pSubGroups[i].group );

            for( int i=0; i<pGroupInfo->iKeyCount; i++ )
                m_pSetting->setValue( pGroupInfo->pKeys[i].strName, pGroupInfo->pKeys[i].strDefaultValue );

            m_pSetting->endGroup();
        }
    }
    setLastError( code );
    return code;
}

// ─── saveDefault ─────────────────────────────────────────────────────────────

void ConfigInstance::saveDefault( Config::GroupInfo* pGroupInfo )
{
    for( int i=0; i<pGroupInfo->iSubGroupCount; i++ )
    {
        Config::GroupInfo* pInfo = pGroupInfo->pSubGroups + i;
        m_pSetting->beginGroup( pInfo->strName );

        if( pInfo->eType == Config::ARRAY )
            m_pSetting->beginGroup( arrayIndex(0) );

        saveDefault( pInfo );

        if( pInfo->eType == Config::ARRAY )
            m_pSetting->endGroup();

        m_pSetting->endGroup();
    }
    for( int i=0; i<pGroupInfo->iKeyCount; i++ )
    {
        Config::KeyInfo* pInfo = pGroupInfo->pKeys + i;
        m_pSetting->setValue( pInfo->strName, pInfo->strDefaultValue );
    }
}

// ─── checkKeyValueValid ──────────────────────────────────────────────────────

Config::ErrorCode ConfigInstance::checkKeyValueValid( const std::string& strValue,
                                                       Config::KeyInfo* pInfo )
{
    Config::ErrorCode code = Config::ERROR_NONE;

    switch( pInfo->eDataType )
    {
    case Config::NUMBER:
    {
        try
        {
            size_t pos = 0;
            double dbValue = std::stod( strValue, &pos );
            if( pos == 0 || pos != strValue.size() )
            {
                code = Config::DATA_INVALID;
                CONFIG_LOG() << "Key: " << pInfo->strName << " value not valid!";
            }
            else if( (dbValue < pInfo->dbMin) || (dbValue > pInfo->dbMax) )
            {
                code = Config::DATA_OUT_OF_RANGE;
                CONFIG_LOG() << "Key: " << pInfo->strName << " value out of range!";
            }
        }
        catch(...)
        {
            code = Config::DATA_INVALID;
            CONFIG_LOG() << "Key: " << pInfo->strName << " value not valid!";
        }
        break;
    }
    case Config::TEXT:
    {
        if( strValue.empty() )
        {
            code = Config::DATA_INVALID;
            CONFIG_LOG() << "Key: " << pInfo->strName << " value not valid!";
        }
        break;
    }
    case Config::DATETIME:
    {
        // 简单检查 ISO8601 格式：YYYY-MM-DDTHH:MM:SS 或 YYYY-MM-DD
        static const std::regex isoDate(
            R"(\d{4}-\d{2}-\d{2}(T\d{2}:\d{2}:\d{2})?)" );
        if( !std::regex_match( strValue, isoDate ) )
        {
            code = Config::DATA_INVALID;
            CONFIG_LOG() << "Key: " << pInfo->strName << " datetime not valid!";
        }
        break;
    }
    default:
        break;
    }

    setLastError( code );
    return code;
}

// ─── setValue ────────────────────────────────────────────────────────────────

void ConfigInstance::setValue( double dbValue, Config::KeyCode eKey, Config::GroupCode eGroup )
{
    std::ostringstream oss;
    oss << dbValue;
    setValue( oss.str(), eKey, eGroup );
}

void ConfigInstance::setValue( const std::string& strValue, Config::KeyCode eKey,
                                Config::GroupCode eGroup )
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    if( isArrayMode() )
    {
        m_lastError = Config::INVALID_USE_OF_ARRAYMODE;
        CONFIG_LOG() << "ConfigInstance::setValue() is not allowed at array mode!";
        return;
    }

    Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );
    Config::KeyInfo* pKeyInfo = getKeyInfo( eKey, pGroupInfo );

    if( nullptr == pGroupInfo )
    {
        m_lastError = Config::GROUP_NOT_EXIST;
        CONFIG_LOG() << "ConfigInstance::setValue: Group not exist!";
    }
    else if( nullptr == pKeyInfo )
    {
        m_lastError = Config::KEY_NOT_EXIST;
        CONFIG_LOG() << "ConfigInstance::setValue: Key not exist!";
    }
    else
    {
        Config::ErrorCode code = checkKeyValueValid( strValue, pKeyInfo );
        if( Config::ERROR_NONE == code )
        {
            if( eGroup == Config::CURRENT_GROUP )
            {
                m_pSetting->setValue( pKeyInfo->strName, strValue );
            }
            else
            {
                m_pSetting->beginGroup( pGroupInfo->strName );
                m_pSetting->setValue( pKeyInfo->strName, strValue );
                m_pSetting->endGroup();
            }
        }
        else
        {
            m_lastError = code;
        }
    }

    save();
}

// ─── setDefaultValue ─────────────────────────────────────────────────────────

void ConfigInstance::setDefaultValue( double dDefaultValue, Config::KeyCode eKey,
                                       Config::GroupCode eGroup )
{
    std::ostringstream oss;
    oss << dDefaultValue;
    setDefaultValue( oss.str(), eKey, eGroup );
}

void ConfigInstance::setDefaultValue( const std::string& strDefaultValue, Config::KeyCode eKey,
                                       Config::GroupCode eGroup )
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    if( isArrayMode() )
    {
        m_lastError = Config::INVALID_USE_OF_ARRAYMODE;
        CONFIG_LOG() << "ConfigInstance::setDefaultValue() is not allowed at array mode!";
        return;
    }

    Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );
    Config::KeyInfo* pKeyInfo = getKeyInfo( eKey, pGroupInfo );

    if( nullptr == pGroupInfo )
    {
        m_lastError = Config::GROUP_NOT_EXIST;
        CONFIG_LOG() << "ConfigInstance::setDefaultValue: Group not exist!";
    }
    else if( nullptr == pKeyInfo )
    {
        m_lastError = Config::KEY_NOT_EXIST;
        CONFIG_LOG() << "ConfigInstance::setDefaultValue: Key not exist!";
    }
    else
    {
        Config::ErrorCode code = checkKeyValueValid( strDefaultValue, pKeyInfo );
        if( Config::ERROR_NONE == code )
        {
            pKeyInfo->strDefaultValue = strDefaultValue;
        }
        else
        {
            m_lastError = code;
        }
    }
}

// ─── removeGroup ─────────────────────────────────────────────────────────────

void ConfigInstance::removeGroup( Config::GroupCode eGroup )
{
    Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );
    if( pGroupInfo != nullptr )
        m_pSetting->remove( pGroupInfo->strName );
}

// ─── value ───────────────────────────────────────────────────────────────────

std::string ConfigInstance::value( Config::KeyCode eKey, Config::GroupCode eGroup )
{
    std::string strValue;
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    if( isArrayMode() )
    {
        m_lastError = Config::INVALID_USE_OF_ARRAYMODE;
        CONFIG_LOG() << "ConfigInstance::value() is not allowed at array mode!";
        return strValue;
    }

    Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );
    Config::KeyInfo* pKeyInfo = getKeyInfo( eKey, pGroupInfo );

    if( nullptr == pGroupInfo )
    {
        m_lastError = Config::GROUP_NOT_EXIST;
        CONFIG_LOG() << "ConfigInstance::value: Group not exist!";
    }
    else if( nullptr == pKeyInfo )
    {
        m_lastError = Config::KEY_NOT_EXIST;
        CONFIG_LOG() << "ConfigInstance::value: Key not exist!";
    }
    else
    {
        if( eGroup != Config::CURRENT_GROUP )
            m_pSetting->beginGroup( pGroupInfo->strName );

        strValue = m_pSetting->value( pKeyInfo->strName, pKeyInfo->strDefaultValue );
        Config::ErrorCode code = checkKeyValueValid( strValue, pKeyInfo );

        if( ( !m_pSetting->contains( pKeyInfo->strName ) ) || ( Config::ERROR_NONE != code ) )
        {
#ifdef WRITE_VALID_VALUE_WHEN_OPERATE
            m_pSetting->setValue( pKeyInfo->strName, pKeyInfo->strDefaultValue );
#endif
            CONFIG_LOG() << "Value not exist or invalid, use default.";
            strValue = pKeyInfo->strDefaultValue;
        }
        setLastError( code );

        if( eGroup != Config::CURRENT_GROUP )
            m_pSetting->endGroup();
    }

    return strValue;
}

// ─── beginGroup / endGroup ───────────────────────────────────────────────────

Config::ErrorCode ConfigInstance::beginGroup( Config::GroupCode eGroup )
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    Config::ErrorCode code = Config::ERROR_NONE;
    Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );

    if( nullptr != pGroupInfo )
    {
        if( Config::ARRAY == pGroupInfo->eType )
        {
            code = Config::INVALID_USE_OF_ARRAYMODE;
            setLastError( code );
            CONFIG_LOG() << "cannot beginGroup an array group!";
        }
        else
        {
            m_pCurrentGroup = pGroupInfo;
            m_pSetting->beginGroup( m_pCurrentGroup->strName );

            GroupItem groupInfo;
            groupInfo.eGroup = eGroup;
            groupInfo.pInfo = m_pCurrentGroup;
            m_vecGroupEntered.push_back( groupInfo );
        }
    }
    else
    {
        code = Config::GROUP_NOT_EXIST;
        setLastError( code );
        CONFIG_LOG() << "Group not exist!";
    }

    return code;
}

void ConfigInstance::endGroup()
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    if( !m_vecGroupEntered.empty() )
    {
        m_pSetting->endGroup();
        m_vecGroupEntered.erase( m_vecGroupEntered.end() - 1 );

        if( !m_vecGroupEntered.empty() )
            m_pCurrentGroup = m_vecGroupEntered.back().pInfo;
        else
            m_pCurrentGroup = nullptr;
    }
    else
    {
        CONFIG_LOG() << "ConfigInstance::endGroup: already group exit!";
    }
}

// ─── Array 模式 ──────────────────────────────────────────────────────────────

int ConfigInstance::beginArray( Config::GroupCode eGroup )
{
    int iCount = 0;
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    if( (!isArrayMode()) || isItemBegun() )
    {
        Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );
        if( nullptr != pGroupInfo )
        {
            if( Config::NORMAL == pGroupInfo->eType )
            {
                setLastError( Config::INVALID_USE_OF_ARRAYMODE );
                CONFIG_LOG() << "cannot beginArray a normal group!";
            }
            else
            {
                m_pCurrentGroup = pGroupInfo;
                m_pSetting->beginGroup( m_pCurrentGroup->strName );
                iCount = m_pSetting->childGroupCount( m_pCurrentGroup->strName );

                GroupItem groupInfo;
                groupInfo.eGroup = eGroup;
                groupInfo.pInfo = m_pCurrentGroup;
                m_vecGroupEntered.push_back( groupInfo );
            }
        }
        else
        {
            setLastError( Config::GROUP_NOT_EXIST );
            CONFIG_LOG() << "Group not exist!";
        }
    }
    else
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::beginArray(): invalid use!";
    }

    return iCount;
}

void ConfigInstance::beginItem( int iIndex )
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    if( isArrayMode() )
    {
        if( isItemBegun() )
        {
            setLastError( Config::INVALID_USE_OF_ARRAYMODE );
            CONFIG_LOG() << "ConfigInstance::beginItem(): already in item mode!";
        }
        else
        {
            m_pSetting->beginGroup( arrayIndex(iIndex) );
            m_vecGroupItemEntered.push_back( m_vecGroupEntered.back().eGroup );
        }
    }
    else
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::beginItem() can only used in array mode!";
    }
}

void ConfigInstance::endItem()
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    if( isArrayMode() )
    {
        m_pSetting->endGroup();
        if( !m_vecGroupItemEntered.empty() )
            m_vecGroupItemEntered.erase( m_vecGroupItemEntered.end() - 1 );
        else
        {
            setLastError( Config::INVALID_USE_OF_ARRAYMODE );
            CONFIG_LOG() << "ConfigInstance::endItem(): invalid endItem used.";
        }
    }
    else
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::endItem() can only used in array mode!";
    }
}

void ConfigInstance::endArray()
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    if( isArrayMode() )
        endGroup();
    else
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::endArray() can only used in array mode!";
    }
}

void ConfigInstance::setItemValue( double dbValue, Config::KeyCode eKey )
{
    std::ostringstream oss;
    oss << dbValue;
    setItemValue( oss.str(), eKey );
}

void ConfigInstance::setItemValue( const std::string& strValue, Config::KeyCode eKey )
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    if( !isArrayMode() )
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::setItemValue() can only used in array mode!";
        return;
    }

    if( !isItemBegun() )
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::setItemValue() can only used after beginItem called!";
        return;
    }

    Config::KeyInfo* pKeyInfo = getKeyInfo( eKey, m_pCurrentGroup );
    if( nullptr != pKeyInfo )
    {
        Config::ErrorCode code = checkKeyValueValid( strValue, pKeyInfo );
        setLastError( code );
        if( Config::ERROR_NONE != code )
        {
#ifdef WRITE_VALID_VALUE_WHEN_OPERATE
            m_pSetting->setValue( pKeyInfo->strName, pKeyInfo->strDefaultValue );
#endif
        }
        else
        {
            m_pSetting->setValue( pKeyInfo->strName, strValue );
        }
    }
    else
    {
        setLastError( Config::KEY_NOT_EXIST );
        CONFIG_LOG() << "Key not exist!";
    }
}

std::string ConfigInstance::itemValue( Config::KeyCode eKey )
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);
    std::string strValue;

    if( !isArrayMode() )
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::itemValue() can only used in array mode!";
        return strValue;
    }

    if( !isItemBegun() )
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::itemValue() can only used after beginItem called!";
        return strValue;
    }

    Config::KeyInfo* pKeyInfo = getKeyInfo( eKey, m_pCurrentGroup );
    if( nullptr != pKeyInfo )
    {
        strValue = m_pSetting->value( pKeyInfo->strName, pKeyInfo->strDefaultValue );
        Config::ErrorCode code = checkKeyValueValid( strValue, pKeyInfo );

        if( ( !m_pSetting->contains( pKeyInfo->strName ) ) || ( Config::ERROR_NONE != code ) )
        {
            strValue = pKeyInfo->strDefaultValue;
            setLastError( code );
            m_pSetting->setValue( pKeyInfo->strName, strValue );
        }
    }
    else
    {
        setLastError( Config::KEY_NOT_EXIST );
    }

    return strValue;
}

// ─── getGroupInfo / getKeyInfo ───────────────────────────────────────────────

Config::GroupInfo* ConfigInstance::getGroupInfo( Config::GroupCode eGroup )
{
    if( Config::CURRENT_GROUP == eGroup )
        return m_pCurrentGroup;

    if( nullptr != m_pCurrentGroup )
    {
        for( int i=0; i<m_pCurrentGroup->iSubGroupCount; i++ )
        {
            if( eGroup == m_pCurrentGroup->pSubGroups[i].group )
                return m_pCurrentGroup->pSubGroups + i;
        }
    }
    else
    {
        for( int i=0; i<m_pInfo->iCount; i++ )
        {
            if( eGroup == m_pInfo->pGroups[i]->group )
                return m_pInfo->pGroups[i];
        }
    }

    return nullptr;
}

Config::KeyInfo* ConfigInstance::getKeyInfo( Config::KeyCode eKey,
                                              Config::GroupInfo* pGroupInfo )
{
    if( nullptr == pGroupInfo ) return nullptr;

    for( int i=0; i<pGroupInfo->iKeyCount; i++ )
    {
        if( eKey == pGroupInfo->pKeys[i].key )
            return pGroupInfo->pKeys + i;
    }

    return nullptr;
}

// ─── 文件名处理 ──────────────────────────────────────────────────────────────

std::string ConfigInstance::getLoopFileName( const std::string& basePath,
                                              const std::string& baseName,
                                              const std::string& ext,
                                              int iLoopNumber )
{
    return basePath + "/" + baseName + std::to_string(iLoopNumber) + ext;
}

int ConfigInstance::getNumberFromFileName( const std::string& fileName )
{
    fs::path p( fileName );
    std::string stem = p.stem().string();                      // e.g. "config2"
    fs::path base( m_pInfo->strFileName );
    std::string baseStem = base.stem().string();               // e.g. "config"

    if( stem.size() > baseStem.size() )
    {
        std::string numStr = stem.substr( baseStem.size() );
        try { return std::stoi( numStr ); } catch(...) {}
    }
    return 0;
}

std::string ConfigInstance::getFileName( const std::string& strFileName,
                                          Config::Mode eMode, int iLoopCount )
{
    (void)iLoopCount;
    if( Config::LOOP != eMode ) return strFileName;

    fs::path baseP( strFileName );
    std::string dirPath  = baseP.parent_path().string();
    std::string baseName = baseP.stem().string();
    std::string ext      = baseP.extension().string();   // includes '.'

    // 扫描目录，找出所有以 baseName 开头的同扩展名文件
    std::vector<fs::path> candidates;
    if( fs::exists( baseP.parent_path() ) )
    {
        for( const auto& entry : fs::directory_iterator( baseP.parent_path() ) )
        {
            if( !entry.is_regular_file() ) continue;
            std::string s = entry.path().stem().string();
            std::string e = entry.path().extension().string();
            if( e == ext && s.size() >= baseName.size() &&
                s.compare(0, baseName.size(), baseName) == 0 )
            {
                candidates.push_back( entry.path() );
            }
        }
    }

    if( candidates.empty() )
    {
        m_iLoopNumber = 0;
        return getLoopFileName( dirPath, baseName, ext, 0 );
    }

    // 找最新修改时间的文件
    fs::path latest = candidates[0];
    for( size_t i=1; i<candidates.size(); i++ )
    {
        if( fs::last_write_time( candidates[i] ) >= fs::last_write_time( latest ) )
            latest = candidates[i];
    }

    m_iLoopNumber = getNumberFromFileName( latest.string() );
    return latest.string();
}

// ─── save ────────────────────────────────────────────────────────────────────

void ConfigInstance::save()
{
    std::lock_guard<std::recursive_mutex> locker(m_mutex);

    m_pSetting->sync();

    if( Config::LOOP == m_eMode )
    {
        m_iLoopNumber = ( m_iLoopNumber + 1 ) % m_iLoopCount;

        fs::path baseP( m_pInfo->strFileName );
        std::string dirPath  = baseP.parent_path().string();
        std::string baseName = baseP.stem().string();
        std::string ext      = baseP.extension().string();

        std::string strNextFile = getLoopFileName( dirPath, baseName, ext, m_iLoopNumber );

        // 删除目标文件（如存在），再拷贝
        std::error_code ec;
        fs::remove( strNextFile, ec );
        fs::copy_file( m_pSetting->fileName(), strNextFile,
                       fs::copy_options::overwrite_existing, ec );

        initFromFile( strNextFile );
    }

#ifdef __linux__
    sync();
#endif
}

// ─── 错误处理 ────────────────────────────────────────────────────────────────

Config::ErrorCode ConfigInstance::lastError() const
{
    return m_lastError;
}

std::string ConfigInstance::lastErrorString() const
{
    switch( m_lastError )
    {
    case Config::ERROR_NONE:             return "No error.";
    case Config::FILE_NOT_EXIST:         return "File not exist.";
    case Config::KEY_NOT_EXIST:          return "Key not exist.";
    case Config::GROUP_NOT_EXIST:        return "Group not exist.";
    case Config::DATA_INVALID:           return "Data invalid.";
    case Config::DATA_OUT_OF_RANGE:      return "Data out of range.";
    case Config::CONFIG_CODE_INVALID:    return "Config code invalid.";
    case Config::INPUT_ERROR:            return "Input error.";
    case Config::CONFIG_ALREADY_EXIST:   return "Config already exist.";
    case Config::INVALID_USE_OF_ARRAYMODE: return "Invalid use of Array mode.";
    case Config::OTHER_ERROR:            return "Other error.";
    default:                             return "";
    }
}

void ConfigInstance::setLastError( Config::ErrorCode code )
{
    if( Config::ERROR_NONE != code )
        m_lastError = code;
}

bool ConfigInstance::isArrayMode() const
{
    return ( nullptr != m_pCurrentGroup ) && ( m_pCurrentGroup->eType == Config::ARRAY );
}

bool ConfigInstance::isItemBegun() const
{
    return ( !m_vecGroupItemEntered.empty() )
        && ( m_vecGroupItemEntered.back() == m_pCurrentGroup->group );
}
