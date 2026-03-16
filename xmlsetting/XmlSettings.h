/*
* Copyright (c) 2016.08，南京华乘电气科技有限公司
* All rights reserved.
*
* xmlsettings.h
*
* 初始版本：1.0
* 作者：周定宇
* 创建日期：2016年08月12日
* 摘要：xml格式的配置文件管理，基于QSettings
* 当前版本：1.0
*/

#ifndef XMLSETTINGS_H
#define XMLSETTINGS_H

#include <QSettings>
#include <QDomDocument>
#include <QStringList>
#include "service_global.h"
class SERVICESHARED_EXPORT XMLSettings : public QSettings
{
public:
    XMLSettings(const QString &qStrFileName, QObject *parent = 0);

    //设置根节点
    void setRoot( const QString& strRoot );

    //设置键对应的值
    void setValue(const QString &qStrKey, const QString &qStrValue);
    void setValue(const QString &qStrKey, const double &dValue);

    //获取键对应的值
    QString value(const QString &qStrKey, const QVariant &defaultValue = QVariant()) const;
private:
    //从xml文件读取数据到QSettings
    static bool readXmlFile(QIODevice &qIODevice, QSettings::SettingsMap &qSettingsMap);

    //将QSettings的数据写入xml文件
    static bool writeXmlFile(QIODevice &qIODevice, const QSettings::SettingsMap &qSettingsMap);

    //将单个键值对写入QDomDocument
    static void writeKeyToFile(QDomDocument &qDomDoc,const QString &qKey,const QVariant &qValue);

    //解析当前节点
    static void parseNode(const QDomNode &qDomNode, const QString &qStrNode, QSettings::SettingsMap &qSettingsMap);

    static Format m_format;  //QSettings自定义文件格式
};

#endif // XMLSETTINGS_H
