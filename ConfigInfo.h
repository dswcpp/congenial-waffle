/*
* All rights reserved.
*
* ConfigInfo.h
*
* 初始版本：1.0
* 作者：duanshengwei
* 创建日期：2026年03月16日
* 摘要：配置模块相关信息定义
* 当前版本：2.0（移除 Qt 依赖）
*/
#ifndef CONFIGINFO_H
#define CONFIGINFO_H

#include <string>

namespace Config
{
    typedef enum _Mode
    {
        SINGLE = 0,//单文件模式
        LOOP,//循环存储模式
    }Mode;

    typedef int KeyCode;//键值
    typedef int GroupCode;//组号

    const int CURRENT_GROUP = -1;//表示当前组

    //组的类型
    typedef enum _GroupMode
    {
        NORMAL = 0,//普通模式
        ARRAY,     //数组模式
    }GroupMode;

    //数据类型
    typedef enum _DataType
    {
        NUMBER = 0,//数值
        TEXT,//文本
        DATETIME//日期时间
    }DataType;

    //组键
    typedef struct _GroupKey
    {
        _GroupKey( KeyCode eKey = -1, GroupCode eGroup = CURRENT_GROUP )
        {
            group = eGroup;
            key = eKey;
        }

        GroupCode group;
        KeyCode key;

        bool operator < (const _GroupKey &other) const
        {
            return (group < other.group) || ((group == other.group) && (key < other.key) );
        }
    }GroupKey;

    //键配置
    typedef struct _KeyInfo
    {
        KeyCode key;//键值
        std::string strName;//对应存储的名称
        DataType eDataType;//数值类型
        std::string strDefaultValue;//默认值
        double dbMin;//数值类型的最小值
        double dbMax;//数值类型的最大值
    }KeyInfo;

    //组配置
    typedef struct _GroupInfo
    {
        GroupCode group;//组号
        GroupMode eType;//组类型
        std::string strName;//组名

        KeyInfo* pKeys;//子键
        int iKeyCount;//键个数

        struct _GroupInfo* pSubGroups;//子组
        int iSubGroupCount;

        //判断配置是否合法
        inline bool isValid() const
        {
            bool bValid = true;

            if( ( iSubGroupCount < 0 )
                || ( iKeyCount < 0 )
                || ( ( iSubGroupCount > 0 ) && ( nullptr == pSubGroups ) )
                || ( ( iKeyCount > 0 ) && ( nullptr == pKeys ) ) )
            {
                bValid = false;
            }
            else
            {
                if( iSubGroupCount > 0 )
                {
                    for( int i=0; i<iSubGroupCount; i++ )
                    {
                        bValid = pSubGroups[i].isValid();
                        if( !bValid ) break;
                    }
                }
                else
                {
                    if( iKeyCount == 0 ) bValid = false;
                }
            }

            return bValid;
        }
    }GroupInfo;

    //配置文件信息
    typedef struct _ConfigInfo
    {
        int iCode;//配置文件号，如果只有一个配置文件，可用默认
        std::string strFileName;//文件路径
        std::string strRoot;//Root名称
        GroupInfo** pGroups;//组配置
        int iCount;//个数

        //判断配置是否合法
        inline bool isValid() const
        {
            bool bValid = false;
            if( ( iCount > 0 ) && ( nullptr != pGroups ) )
            {
                for( int i=0; i<iCount; i++ )
                {
                    bValid = ( nullptr != pGroups[i] ) && pGroups[i]->isValid();
                    if( !bValid ) break;
                }
            }
            return bValid;
        }
    }ConfigInfo;

    //错误码定义
    typedef enum _ErrorCode
    {
        ERROR_NONE = 0,
        FILE_NOT_EXIST = -1,
        KEY_NOT_EXIST = -2,
        GROUP_NOT_EXIST = -3,
        DATA_INVALID = -4,
        DATA_OUT_OF_RANGE = -5,
        CONFIG_CODE_INVALID = -6,
        INPUT_ERROR = -7,
        CONFIG_ALREADY_EXIST = -8,
        INVALID_USE_OF_ARRAYMODE = -9,
        OTHER_ERROR = -255
    }ErrorCode;
};

#endif // CONFIGINFO_H
