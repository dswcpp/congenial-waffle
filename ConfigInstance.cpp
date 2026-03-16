#include "ConfigInstance.h"
#include "ConfigManager.h"
#ifdef Q_OS_LINUX
#include <unistd.h>
#endif
//#define WRITE_VALID_VALUE_WHEN_OPERATE

#define ARRAY_INDEX(index) QString("Index%1").arg(index)
/************************************************
 * 功能：构造函数
 * 输入参数：
 *      pInfo -- 配置
 *      eMode -- 模式
 *      iLoopCount -- Loop模式时的个数
 ************************************************/
ConfigInstance::ConfigInstance( Config::ConfigInfo* pInfo, Config::Mode eMode, int iLoopCount )
    : m_pInfo( pInfo ), m_eMode( eMode ),m_mutex(QMutex::Recursive), m_iLoopCount( iLoopCount ), m_iLoopNumber( 0 )
{
    m_pSetting = NULL;
    if( ( NULL == pInfo ) || ( !pInfo->isValid() ) )
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

/************************************************
 * 功能：析构函数
 ************************************************/
ConfigInstance::~ConfigInstance()
{
    QMutexLocker locker(&m_mutex);


    if( NULL != m_pSetting )
    {
        delete m_pSetting;
        m_pSetting = NULL;
    }
}

/************************************************
 * 功能：载入配置文件
 * 输入参数：
 *      strFile -- 文件名
 * 返回：
 *      ERROR_NONE -- 成功
 *      其它 -- 失败
 ************************************************/
Config::ErrorCode ConfigInstance::load( const QString& strFile )
{
    m_lastError = Config::ERROR_NONE;

    Config::ErrorCode code = Config::ERROR_NONE;

    if( NULL != m_pSetting )
    {
        delete m_pSetting;
    }
    m_pSetting = new XMLSettings( strFile );

    if( Config::LOOP == m_eMode )
    {
        //从文件名中获取序号
        m_iLoopNumber = getNumberFromFileName( strFile );
    }
#ifdef WRITE_VALID_VALUE_WHEN_OPERATE
    if( !QFile::exists( strFile ) )
    {
        code = Config::FILE_NOT_EXIST;
        CONFIG_LOG() << "ConfigInstance: config file " << strFile << " not exist!";

        restoreDefault();//将所有参数恢复默认
    }
    else
#endif
    {
        m_pSetting->setRoot( m_pInfo->strRoot );
    }
    m_pCurrentGroup = NULL;

    setLastError( code );

    return code;
}

/************************************************
 * 功能：从配置文件中初始化
 * 输入参数：
 *      strFile -- 文件名
 * 返回：
 *      ERROR_NONE -- 成功
 *      其它 -- 失败
 ************************************************/
Config::ErrorCode ConfigInstance::initFromFile( const QString& strFile )
{
    QMutexLocker locker(&m_mutex);

    //Loop模式的文件名要特殊处理
    QString strFileName = getFileName( strFile, m_eMode, m_iLoopCount );

    return load( strFileName );
}

/************************************************
 * 功能：将所有参数恢复默认
 * 返回：
 *      ERROR_NONE -- 成功
 *      其它 -- 失败
 ************************************************/
Config::ErrorCode ConfigInstance::restoreDefault()
{
    QMutexLocker locker(&m_mutex);

    m_pSetting->clear();
    //遍历组
    for( int i=0; i<m_pInfo->iCount; i++ )
    {
        Config::GroupInfo* pInfo = m_pInfo->pGroups[i];
        m_pSetting->beginGroup( pInfo->strName );

        if( pInfo->eType == Config::ARRAY )
        {
            m_pSetting->beginGroup( ARRAY_INDEX(0) );
        }

        saveDefault( pInfo );

        if( pInfo->eType == Config::ARRAY )
        {
            m_pSetting->endGroup();
        }

        m_pSetting->endGroup();
    }
    m_pSetting->setRoot( m_pInfo->strRoot );

    m_pSetting->sync();

    return Config::ERROR_NONE;
}

/************************************************
 * 功能：将部分参数恢复默认
 * 输入参数：
 *      vecGroupKeys -- 所要恢复的组键
 * 返回：
 *      ERROR_NONE -- 成功
 *      其它 -- 失败
 ************************************************/
Config::ErrorCode ConfigInstance::restoreDefault( const QVector<Config::GroupKey>& vecGroupKeys )
{
    QMutexLocker locker(&m_mutex);

    Config::ErrorCode code = Config::ERROR_NONE;

    for( int i=0; i<vecGroupKeys.count(); i++ )
    {
        Config::GroupKey groupKey = vecGroupKeys.at(i);
        Config::GroupInfo* pGroupInfo = getGroupInfo( groupKey.group );
        Config::KeyInfo* pKeyInfo = getKeyInfo( groupKey.key, pGroupInfo );

        if( NULL != pKeyInfo )
        {
            setValue( pKeyInfo->strDefaultValue, groupKey.key, groupKey.group );
        }
        else
        {
            code = Config::KEY_NOT_EXIST;
            setLastError( code );
            CONFIG_LOG() << "ConfigInstance::restoreDefault: invalid group " << groupKey.group << " and key " << groupKey.key;
        }
    }

    m_pSetting->sync();

    return code;
}

/************************************************
 * 功能：将部分参数恢复默认
 * 输入参数：
 *      eGroup -- 所要恢复的组键
 * 返回：
 *      ERROR_NONE -- 成功
 *      其它 -- 失败
 ************************************************/
Config::ErrorCode ConfigInstance::restoreDefault( Config::GroupCode eGroup )
{
    QMutexLocker locker(&m_mutex);

    Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );
    Config::ErrorCode code = Config::ERROR_NONE;

    if( NULL == pGroupInfo )
    {
        code = Config::GROUP_NOT_EXIST;
        CONFIG_LOG() << "ConfigInstance::restoreDefault: group" << eGroup << "not exist!";
    }
    else
    {
        //先清空
        m_pSetting->remove( pGroupInfo->strName );
        if( Config::ARRAY != pGroupInfo->eType )//非组模式，要写入
        {
            m_pSetting->beginGroup( pGroupInfo->strName );

            //将组恢复默认
            for( int i=0; i<pGroupInfo->iSubGroupCount; i++ )
            {
                restoreDefault( pGroupInfo->pSubGroups[i].group );
            }

            //将键值恢复默认
            for( int i=0; i<pGroupInfo->iKeyCount; i++ )
            {
                m_pSetting->setValue( pGroupInfo->pKeys[i].strName, pGroupInfo->pKeys[i].strDefaultValue );
            }

            m_pSetting->endGroup();
        }
    }
    setLastError( code );

    return code;
}

/************************************************
 * 功能：保存默认数据入文件
 * 输入参数：
 *      pGroupInfo -- 组配置
 ************************************************/
void ConfigInstance::saveDefault( Config::GroupInfo* pGroupInfo )
{
    for( int i=0; i<pGroupInfo->iSubGroupCount; i++ )
    {
        Config::GroupInfo* pInfo = pGroupInfo->pSubGroups + i;
        m_pSetting->beginGroup( pInfo->strName );

        if( pInfo->eType == Config::ARRAY )
        {
            m_pSetting->beginGroup( ARRAY_INDEX(0) );
        }

        saveDefault( pInfo );

        if( pInfo->eType == Config::ARRAY )
        {
            m_pSetting->endGroup();
        }

        m_pSetting->endGroup();
    }
    for( int i=0; i<pGroupInfo->iKeyCount; i++ )
    {
        Config::KeyInfo* pInfo = pGroupInfo->pKeys + i;
        m_pSetting->setValue( pInfo->strName, pInfo->strDefaultValue );
    }
}


/************************************************
 * 功能：合法性校验，检查键值的合法性
 * 输入参数：
 *      strValue -- 键值
 *      pInfo -- 键配置
 * 返回：
 *      错误码
 ************************************************/
Config::ErrorCode ConfigInstance::checkKeyValueValid( const QString& strValue, Config::KeyInfo* pInfo )
{
    Config::ErrorCode code = Config::ERROR_NONE;
    bool bValid = false;
    switch( pInfo->eDataType )
    {
    case Config::NUMBER:
    {
        double dbValue = strValue.toDouble(&bValid);
        if (bValid)
        {
            code = ( (dbValue < pInfo->dbMin) || (dbValue > pInfo->dbMax)) ? Config::DATA_OUT_OF_RANGE : Config::ERROR_NONE;
            if( code != Config::ERROR_NONE )
            {
                CONFIG_LOG() << "Key:" << pInfo->strName << ",Value:" << dbValue
                             << " out of range!";
            }
        }
        else
        {
            code = Config::DATA_INVALID;
            CONFIG_LOG() << "Key:" << pInfo->strName << ",Value:" << strValue
                         << "not valid!";
        }
    }
        break;
    case Config::TEXT:
    {
        code = strValue.isEmpty() ? Config::DATA_INVALID : Config::ERROR_NONE;
        if( Config::DATA_INVALID == code )
        {
            CONFIG_LOG() << "Key:" << pInfo->strName << ",Value:" << strValue
                         << "not valid!";
        }
    }
        break;

    case Config::DATETIME:
    {
        code = QDateTime::fromString(strValue,Qt::ISODate).isValid() ? Config::ERROR_NONE:Config::DATA_INVALID;

        if( Config::DATA_INVALID == code )
        {
            CONFIG_LOG() << "Key:" << pInfo->strName << ",Value:" << strValue
                         << "not valid!";
        }
    }
        break;
    default:
        break;
    }
    setLastError( code );

    return code;
}

/************************************************
 * 功能：设置值
 * 输入参数：
 *      dbValue, strValue -- 值
 *      eKey -- 键值
 *      eGroup -- 组号
 ************************************************/
void ConfigInstance::setValue( double dbValue, Config::KeyCode eKey, Config::GroupCode eGroup )
{
    QString strValue = QString::number( dbValue );

    setValue( strValue, eKey, eGroup );
}
void ConfigInstance::setValue( const QString& strValue, Config::KeyCode eKey, Config::GroupCode eGroup )
{
    QMutexLocker locker(&m_mutex);

    //该接口不能在Array模式使用
    if( isArrayMode() )
    {
        m_lastError = Config::INVALID_USE_OF_ARRAYMODE;
        CONFIG_LOG() << "ConfigInstance::setValue() is not allowed at array mode!";
    }
    else
    {
        Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );
        Config::KeyInfo* pKeyInfo = getKeyInfo( eKey, pGroupInfo );
        if( NULL == pGroupInfo )
        {
            m_lastError = Config::GROUP_NOT_EXIST;
            CONFIG_LOG() << "ConfigInstance::setValue:Group code " << eGroup << " not exist!";
        }
        else if( NULL == pKeyInfo )
        {
            m_lastError = Config::KEY_NOT_EXIST;
            CONFIG_LOG() << "ConfigInstance::setValue:Group code " << eGroup << " key code "
                         << eKey << "not exist!";
        }
        else
        {
            Config::ErrorCode code = checkKeyValueValid( strValue, pKeyInfo );
            if( Config::ERROR_NONE == code )
            {
                if (eGroup == Config::CURRENT_GROUP)
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
    }

    save();         //其实没必要每把save方法这儿，建议上面判断进入异常逻辑就可以return，或者把这儿的save放到需要setValue之后，值更新了才进行save

    return;
}

/************************************************
 * 功能：设置默认值
 * 输入参数：
 *      dDefaultValue, strDefaultValue -- 默认值
 *      eKey -- 键值
 *      eGroup -- 组号
 ************************************************/
void ConfigInstance::setDefaultValue(double dDefaultValue, Config::KeyCode eKey, Config::GroupCode eGroup)
{
    QString strDefaultValue = QString::number(dDefaultValue);
    setDefaultValue(strDefaultValue, eKey, eGroup);
    return;
}

void ConfigInstance::setDefaultValue(const QString& strDefaultValue, Config::KeyCode eKey, Config::GroupCode eGroup)
{
    QMutexLocker locker(&m_mutex);

    //该接口不能在Array模式使用
    if(isArrayMode())
    {
        m_lastError = Config::INVALID_USE_OF_ARRAYMODE;
        CONFIG_LOG() << "ConfigInstance::setValue() is not allowed at array mode!";
    }
    else
    {
        Config::GroupInfo* pGroupInfo = getGroupInfo(eGroup);
        Config::KeyInfo* pKeyInfo = getKeyInfo(eKey, pGroupInfo);
        if(NULL == pGroupInfo)
        {
            m_lastError = Config::GROUP_NOT_EXIST;
            CONFIG_LOG() << "ConfigInstance::setValue:Group code " << eGroup << " not exist!";
        }
        else if(NULL == pKeyInfo)
        {
            m_lastError = Config::KEY_NOT_EXIST;
            CONFIG_LOG() << "ConfigInstance::setValue:Group code " << eGroup << " key code "
                         << eKey << "not exist!";
        }
        else
        {
            Config::ErrorCode code = checkKeyValueValid(strDefaultValue, pKeyInfo);
            if(Config::ERROR_NONE == code)
            {
                pKeyInfo->strDefaultValue = strDefaultValue;
            }
            else
            {
                m_lastError = code;
            }
        }
    }

    return;
}

/*************************************************
函数名： removeGroup
输入参数： eGroup -- 组号
输出参数： NULL
返回值： NULL
功能： 删除group
*************************************************************/
void ConfigInstance::removeGroup(Config::GroupCode eGroup)
{
    Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );
    m_pSetting->remove(pGroupInfo->strName);
}

/************************************************
 * 功能：获取值
 * 输入参数：
 *      eKey -- 键值
 *      eGroup -- 组号
 * 返回：值
 ************************************************/
QString ConfigInstance::value( Config::KeyCode eKey, Config::GroupCode eGroup )
{
    QString strValue;

    QMutexLocker locker(&m_mutex);

    //该接口不能在Array模式使用
    if( isArrayMode() )
    {
        m_lastError = Config::INVALID_USE_OF_ARRAYMODE;
        CONFIG_LOG() << "ConfigInstance::value() is not allowed at array mode!";
    }
    else
    {
        Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );
        Config::KeyInfo* pKeyInfo = getKeyInfo( eKey, pGroupInfo );

        if( NULL == pGroupInfo )//组配置不存在
        {
            m_lastError = Config::GROUP_NOT_EXIST;
            CONFIG_LOG() << "ConfigInstance::value:Group code " << eGroup << " not exist!";
        }
        else if( NULL == pKeyInfo )//键配置
        {
            m_lastError = Config::KEY_NOT_EXIST;
            CONFIG_LOG() << "ConfigInstance::value:Group code " << eGroup << " key code "
                         << eKey << "not exist!";
        }
        else
        {
            if (eGroup != Config::CURRENT_GROUP)
            {
                m_pSetting->beginGroup( pGroupInfo->strName );
            }

            strValue = m_pSetting->value( pKeyInfo->strName, pKeyInfo->strDefaultValue );
            Config::ErrorCode code = checkKeyValueValid( strValue, pKeyInfo );
            //键值不存在或非法，用默认代替
            if( ( !m_pSetting->contains( pKeyInfo->strName ) ) || ( Config::ERROR_NONE != code ) )
            {
#ifdef WRITE_VALID_VALUE_WHEN_OPERATE
                m_pSetting->setValue( pKeyInfo->strName, pKeyInfo->strDefaultValue );
#endif
                CONFIG_LOG() << "Value of Group:" << eGroup << "Key:" << eKey << "not exist or invalid, use default.";
                strValue = pKeyInfo->strDefaultValue;
            }
            setLastError( code );
            if (eGroup != Config::CURRENT_GROUP)
            {
                m_pSetting->endGroup();
            }

        }
    }

    return strValue;
}

/************************************************
 * 功能：进入Array模式组
 * 输入参数：
 *      eGroup -- 组号
 * 返回：
 *      array中的成员个数
 ************************************************/
int ConfigInstance::beginArray( Config::GroupCode eGroup )
{
    int iCount = 0;

    QMutexLocker locker(&m_mutex);

    //非Array模式，或者是Array模式下，已经beginItem
    if( (!isArrayMode()) || isItemBegun() )
    {
        Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );
        if( NULL != pGroupInfo )
        {
            if( Config::NORMAL == pGroupInfo->eType )
            {
                setLastError( Config::INVALID_USE_OF_ARRAYMODE );
                CONFIG_LOG() << "cannot beginArray an normal group!";
            }
            else
            {
                m_pCurrentGroup = pGroupInfo;
                m_pSetting->beginGroup( m_pCurrentGroup->strName );
                iCount = m_pSetting->childGroups().count();

                //缓存当前组信息
                GroupItem groupInfo;
                groupInfo.eGroup = eGroup;
                groupInfo.pInfo = m_pCurrentGroup;
                m_vecGroupEntered << groupInfo;
            }
        }
        else
        {
            setLastError( Config::GROUP_NOT_EXIST );
            CONFIG_LOG() << "Group code " << eGroup << " not exist!";
        }
    }
    else
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::beginArray():invalid use of array mode!";
    }

    return iCount;
}
/************************************************
 * 功能：进入普通组
 * 输入参数：
 *      eGroup -- 组号
 * 返回：
 *      ERROR_NONE -- 成功
 *      其它 -- 失败
 ************************************************/
Config::ErrorCode ConfigInstance::beginGroup( Config::GroupCode eGroup )
{
    QMutexLocker locker(&m_mutex);

    Config::ErrorCode code = Config::ERROR_NONE;

    Config::GroupInfo* pGroupInfo = getGroupInfo( eGroup );

    if( NULL != pGroupInfo )
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

            //缓存当前组信息
            GroupItem groupInfo;
            groupInfo.eGroup = eGroup;
            groupInfo.pInfo = m_pCurrentGroup;
            m_vecGroupEntered << groupInfo;
        }
    }
    else
    {
        code = Config::GROUP_NOT_EXIST;
        setLastError( code );
        CONFIG_LOG() << "Group code " << eGroup << " not exist!";
    }

    return code;
}

/************************************************
 * 功能：退出进入的普通组
 ************************************************/
void ConfigInstance::endGroup()
{
    QMutexLocker locker(&m_mutex);

    if( m_vecGroupEntered.count() > 0 )
    {
        m_pSetting->endGroup();
        m_vecGroupEntered.remove( m_vecGroupEntered.count() - 1 );

        if( m_vecGroupEntered.count() > 0 )
        {
            m_pCurrentGroup = m_vecGroupEntered.at( m_vecGroupEntered.count() - 1 ).pInfo;
        }
        else
        {
            m_pCurrentGroup = NULL;
        }
    }
    else
    {
        CONFIG_LOG() << "ConfigInstance::endGroup: already group exit!";
    }
}

/************************************************
 * 功能：开始一个条目
 ************************************************/
void ConfigInstance::beginItem( int iIndex )
{
    QMutexLocker locker(&m_mutex);

    if( isArrayMode() )
    {
        if( isItemBegun() )
        {
            setLastError( Config::INVALID_USE_OF_ARRAYMODE );
            CONFIG_LOG() << "ConfigInstance::beginItem(): already in item mode!";
        }
        else
        {
            m_pSetting->beginGroup( ARRAY_INDEX(iIndex) );
            m_vecGroupItemEntered << m_vecGroupEntered.last().eGroup;
        }
    }
    else
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::beginItem() can only used in array mode!";
    }
}

/************************************************
 * 功能：结束一个条目
 ************************************************/
void ConfigInstance::endItem( )
{
    QMutexLocker locker(&m_mutex);

    if( isArrayMode() )
    {
        m_pSetting->endGroup();
        if( m_vecGroupItemEntered.count() > 0 )
        {
            m_vecGroupItemEntered.remove( m_vecGroupItemEntered.count() - 1 );
        }
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

/************************************************
 * 功能：退出进入的Array组
 ************************************************/
void ConfigInstance::endArray()
{
    QMutexLocker locker(&m_mutex);

    if( isArrayMode() )
    {
        endGroup();
    }
    else
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::endArray() can only used in array mode!";
    }
}

/************************************************
 * 功能：设置数据
 * 输入参数：
 *      dbValue, strValue -- 值
 *      eKey -- 键值
 ************************************************/
void ConfigInstance::setItemValue( double dbValue, Config::KeyCode eKey )
{
    QString strValue = QString::number( dbValue );

    setItemValue( strValue, eKey );
}

void ConfigInstance::setItemValue( const QString& strValue, Config::KeyCode eKey )
{
    QMutexLocker locker(&m_mutex);

    if( isArrayMode() )
    {
        if( isItemBegun() )
        {
            Config::KeyInfo* pKeyInfo = getKeyInfo( eKey, m_pCurrentGroup );

            if( NULL != pKeyInfo )
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
                CONFIG_LOG() << "Key code " << eKey << "not exist!";
            }
        }
        else
        {
            setLastError( Config::INVALID_USE_OF_ARRAYMODE );
            CONFIG_LOG() << "ConfigInstance::setItemValue() can only used after beginItem called!";
        }
    }
    else
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::setItemValue() can only used in array mode!";
    }
}

/************************************************
 * 功能：判断有无进入合法beginItem模式
 * 返回：
 *      true：进入beginItem模式
 *      false：没有
 ************************************************/
bool ConfigInstance::isItemBegun()const
{
    return ( ( m_vecGroupItemEntered.count() > 0 )
             && ( m_vecGroupItemEntered.at(m_vecGroupItemEntered.count()-1)
                  == m_pCurrentGroup->group ) );
}
/************************************************
 * 功能：获取值
 * 输入参数：
 *      eKey -- 键值
 * 返回：值
 ************************************************/
QString ConfigInstance::itemValue( Config::KeyCode eKey  )
{
    QMutexLocker locker(&m_mutex);
    QString strValue;

    if( isArrayMode() )
    {
        if( isItemBegun() )
        {
            Config::KeyInfo* pKeyInfo = getKeyInfo( eKey, m_pCurrentGroup );

            if( NULL != pKeyInfo )
            {
                strValue = m_pSetting->value( pKeyInfo->strName, pKeyInfo->strDefaultValue );
                Config::ErrorCode code = checkKeyValueValid( strValue, pKeyInfo );
                //键值不存在或非法，用默认代替
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
        }
        else
        {
            setLastError( Config::INVALID_USE_OF_ARRAYMODE );
            CONFIG_LOG() << "ConfigInstance::itemValue() can only used after beginItem called!";
        }
    }
    else
    {
        setLastError( Config::INVALID_USE_OF_ARRAYMODE );
        CONFIG_LOG() << "ConfigInstance::itemValue() can only used in array mode!";
    }

    return strValue;
}

/************************************************
 * 功能：获取组配置
 * 输入参数：
 *      eGroup -- 组号
 * 返回：
 *      组配置
 ************************************************/
Config::GroupInfo* ConfigInstance::getGroupInfo( Config::GroupCode eGroup )
{
    Config::GroupInfo* pGroupInfo = NULL;

    if( Config::CURRENT_GROUP == eGroup )
    {
        pGroupInfo = m_pCurrentGroup;
    }
    //有当前组
    else if( NULL != m_pCurrentGroup )
    {
        for( int i=0; i<m_pCurrentGroup->iSubGroupCount; i++ )
        {
            if( eGroup == m_pCurrentGroup->pSubGroups[i].group )
            {
                pGroupInfo = m_pCurrentGroup->pSubGroups + i;
                break;
            }
        }
    }
    else
    {
        for( int i=0; i<m_pInfo->iCount; i++ )
        {
            if( eGroup == m_pInfo->pGroups[i]->group )
            {
                pGroupInfo = m_pInfo->pGroups[i];
                break;
            }
        }
    }

    return pGroupInfo;
}

/************************************************
 * 功能：获取键配置
 * 输入参数：
 *      eKey -- 键值
 *      pGroupInfo -- 组配置
 * 返回：
 *      键配置
 ************************************************/
Config::KeyInfo* ConfigInstance::getKeyInfo( Config::KeyCode eKey, Config::GroupInfo* pGroupInfo )
{
    Config::KeyInfo* pKeyInfo = NULL;

    if( NULL != pGroupInfo )
    {
        for( int i=0; i<pGroupInfo->iKeyCount; i++ )
        {
            if( eKey == pGroupInfo->pKeys[i].key )
            {
                pKeyInfo = pGroupInfo->pKeys + i;
                break;
            }
        }
    }

    return pKeyInfo;
}

/************************************************
 * 功能：根据配置生成文件名
 * 输入参数：
 *      strFileName -- 配置文件名
 *      eMode -- 模式
 *      iLoopCount -- Loop的个数
 ************************************************/
QString ConfigInstance::getFileName( const QString& strFileName, Config::Mode eMode, int iLoopCount )
{
    Q_UNUSED( iLoopCount );
    QString strFileNameMode = strFileName;

    //Loop模式下，返回最新文件的文件名
    if( Config::LOOP == eMode )
    {
        QFileInfo fileInfo( strFileName );
        QString strPath = fileInfo.path();

        QDir dir( strPath  );
        QFileInfoList fileList = dir.entryInfoList( QStringList(fileInfo.baseName() + "*") );

        QDateTime timeLatest;
        int iIndexLatest;
        //如果已有配置文件，则从最新的文件中恢复
        if( fileList.count() > 0 )
        {
            timeLatest = fileList.at(0).lastModified();
            iIndexLatest = 0;

            //寻找最新的文件
            for( int i=1; i<fileList.count(); i++ )
            {
                QDateTime timeModified = fileList.at(i).lastModified();

                if( timeModified >= timeLatest )
                {
                    iIndexLatest = i;
                    timeLatest = timeModified;
                }
            }

            fileInfo = fileList.at(iIndexLatest);
            strFileNameMode = fileInfo.absoluteFilePath();

            //从文件名中获取序号
            m_iLoopNumber = getNumberFromFileName( fileInfo.fileName() );
        }
        //不存在则从0开始
        else
        {
            m_iLoopNumber = 0;
            strFileNameMode = getLoopFileName( fileInfo, m_iLoopNumber );
        }
    }

    return strFileNameMode;
}

/************************************************
 * 功能：从文件名中获取序号
 * 输入参数：
 *      fileName -- 文件名
 * 返回：
 *      序号
 ************************************************/
int ConfigInstance::getNumberFromFileName( const QString& fileName )
{
    QFileInfo fileInfo( fileName );

    QFileInfo fileBase( m_pInfo->strFileName );
    QString strNumber = fileInfo.baseName().remove( fileBase.baseName() );

    return strNumber.toUInt();
}

/************************************************
 * 功能：从序号中获取对应文件名
 * 输入参数：
 *      fileInfo -- 当前文件信息
 *      iLoopNumber -- 循环序号
 * 返回：
 *      文件名
 ************************************************/
QString ConfigInstance::getLoopFileName( const QFileInfo& fileInfo, int iLoopNumber )
{
    return fileInfo.path()
            + QDir::separator()
            + fileInfo.baseName()
            + QString::number(iLoopNumber)
            + "." +
            fileInfo.suffix();
}

/************************************************
 * 功能：保存至文件
 ************************************************/
void ConfigInstance::save()
{
    QMutexLocker locker(&m_mutex);

    m_pSetting->sync();

    //循环模式，则循环保存
    if( Config::LOOP == m_eMode )
    {
        m_iLoopNumber = ( m_iLoopNumber + 1 )%m_iLoopCount;
        QFileInfo fileInfo( m_pInfo->strFileName );

        QString strFileName = getLoopFileName( fileInfo, m_iLoopNumber );

        //先删除
        QFile file( strFileName );
        file.open( QIODevice::WriteOnly );
        file.remove();
        file.close();
        //再拷贝
        QFile::copy( m_pSetting->fileName(), strFileName );

        initFromFile( strFileName );
    }

#ifdef Q_OS_LINUX
    sync();
#endif
}

/************************************************
 * 功能：获取最近一次错误码
 * 返回：
 *      错误码
 ************************************************/
Config::ErrorCode ConfigInstance::lastError() const
{
    return m_lastError;
}

/************************************************
 * 功能：获取最近一次错误码
 * 返回：
 *      错误码
 ************************************************/
QString ConfigInstance::lastErrorString() const
{
    QString str;
    switch( m_lastError )
    {
    case Config::ERROR_NONE:
        str = "No error.";
        break;
    case Config::FILE_NOT_EXIST:
        str = "File not exist.";
        break;
    case Config::KEY_NOT_EXIST:
        str = "Key not exist.";
        break;
    case Config::GROUP_NOT_EXIST:
        str = "Group not exist.";
        break;
    case Config::DATA_INVALID:
        str = "Data invalid.";
        break;
    case Config::DATA_OUT_OF_RANGE:
        str = "Data out of range.";
        break;
    case Config::CONFIG_CODE_INVALID:
        str = "Config code invalid.";
        break;
    case Config::INPUT_ERROR:
        str = "Input error.";
        break;
    case Config::CONFIG_ALREADY_EXIST:
        str = "Config already exist.";
        break;
    case Config::OTHER_ERROR:
        str = "Other error.";
        break;
    case Config::INVALID_USE_OF_ARRAYMODE:
        str = "Invalid use of Array mode.";
        break;
    default:
        break;
    }

    return str;
}

/************************************************
 * 功能：设置最后一次错误码
 * 输入参数：
 *      code -- 错误码
 ************************************************/
void ConfigInstance::setLastError( Config::ErrorCode code )
{
    if( Config::ERROR_NONE != code )
    {
        m_lastError = code;
    }
}

/************************************************
 * 功能：判断是否Array模式
 * 返回：
 *      true: Array模式
 *      false:Normal模式
 ************************************************/
bool ConfigInstance::isArrayMode() const
{
    return ( ( NULL != m_pCurrentGroup ) && ( m_pCurrentGroup->eType == Config::ARRAY ) );
}
