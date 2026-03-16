/*
* Copyright (c) 2016.11，南京华乘电气科技有限公司
* All rights reserved.
*
* ConfigInfo.h
*
* 初始版本：1.0
* 作者：邵震宇
* 创建日期：2016年11月29日
* 摘要：配置模块相关信息定义
* 当前版本：1.0
*/
/*
 * 配置模块说明
 * 1、支持以下模式：
 *   1）全局一个文件
 *   2）全局多个文件
 *   3）单个文件的，可动态切换文件；多个文件的，可动态切换文件夹
 * 2、可选择性进行循环存储：
 *   可配置循环的份数，每次存储时，更新存储序号，可回退
 * 3、支持启动时合法性校验，并给出校验结果，非法时，自动使用默认参数
 * 4、可全部或部分恢复设置参数为默认参数
 * 5、每一份配置文件，用配置表来初始化，可批量添加（用于多个模块使用）
 * 6、利用键值和组号来实现对参数的访问
 * 7、支持Array模式（用于同类别，但是数目不定的场景）
 *   Array模式场景：1）wifi的账号密码，wifi组数不定
 *                2）在线监测中，设备的信息，设备的数组不定
 *   注：Array支持嵌套
 */
#ifndef CONFIGINFO_H
#define CONFIGINFO_H
#include <QtCore>

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
        QString strName;//对应存储的名称
        DataType eDataType;//数值类型
        QString strDefaultValue;//默认值
        double dbMin;//数值类型的最小值
        double dbMax;//数值类型的最大值
    }KeyInfo;

    //组配置
    typedef struct _GroupInfo
    {
        GroupCode group;//组号
        GroupMode eType;//组类型
        QString strName;//组名

        KeyInfo* pKeys;//子键
        int iKeyCount;//键个数

        struct _GroupInfo* pSubGroups;//子组
        int iSubGroupCount;

        //判断配置是否合法
        inline bool isValid() const
        {
            bool bValid = true;

            //合法性判断
            if( ( iSubGroupCount < 0 ) //组数和键数不允许<0
                || ( iKeyCount < 0 )
                || ( ( iSubGroupCount > 0 ) && ( NULL == pSubGroups ) )//数目和配置不匹配
                || ( ( iKeyCount > 0 ) && ( NULL == pKeys ) ) )

            {
                bValid = false;
            }
            else
            {
                if( iSubGroupCount > 0 )//递归判断所有组的合法性
                {
                    for( int i=0; i<iSubGroupCount; i++ )
                    {
                        //递归判断所有组的合法性
                        bValid = pSubGroups[i].isValid();
                        if( !bValid )
                        {
                            break;
                        }
                    }
                }
                else
                {
                    if( iKeyCount == 0 )//组数目和键数目同时为0
                    {
                        bValid = false;
                    }
                }
            }

            return bValid;
        }
    }GroupInfo;

    //配置文件信息
    typedef struct _ConfigInfo
    {
        int iCode;//配置文件号，如果只有一个配置文件，可用默认
        QString strFileName;//文件路径
        QString strRoot;//Root名称
        GroupInfo** pGroups;//组配置
        int iCount;//个数

        //判断配置是否合法
        inline bool isValid() const
        {
            bool bValid = false;
            if( ( iCount > 0 ) && ( NULL != pGroups ) )
            {
                for( int i=0; i<iCount; i++ )
                {
                    bValid = ( NULL != pGroups[i] ) && pGroups[i]->isValid() ;
                    if( !bValid )
                    {
                        break;
                    }
                }
            }
            return bValid;
        }
    }ConfigInfo;

    //错误码定义
    typedef enum _ErrorCode
    {
        ERROR_NONE = 0,
        FILE_NOT_EXIST = -1,//文件不存在
        KEY_NOT_EXIST = -2,//键值缺失
        GROUP_NOT_EXIST = -3,//组号缺失
        DATA_INVALID = -4,//数据非法
        DATA_OUT_OF_RANGE = -5,//数据越限
        CONFIG_CODE_INVALID = -6,//配置号错误
        INPUT_ERROR = -7,//输入错误
        CONFIG_ALREADY_EXIST = -8,//配置已经存在
        INVALID_USE_OF_ARRAYMODE = -9,//错误的使用Array模式
        OTHER_ERROR = -255//其它错误
    }ErrorCode;
};

#endif // CONFIGINFO_H
