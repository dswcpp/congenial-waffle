#include <QTextStream>
#include <QDebug>
#include "XmlSettings.h"

QSettings::Format XMLSettings::m_format = QSettings::registerFormat("xml", readXmlFile, writeXmlFile);
#define XMLSETTINGS_KEY_ROOT "Root" //Root键值
#define XMLSETTINGS_DEFAULT_ROOT "Station" //默认Root

XMLSettings::XMLSettings(const QString &qStrFileName, QObject *parent)
    :QSettings(qStrFileName, m_format, parent)
{
}

//设置根节点
void XMLSettings::setRoot( const QString& strRoot )
{
    QSettings::setValue( XMLSETTINGS_KEY_ROOT, strRoot );
}

/*************************************************
函数名： setValue(const QString &qStrKey, const QString &qStrValue)
输入参数： qStrKey：键
          qStrValue：值
输出参数： NULL
返回值： NULL
功能： 设置键对应的值
*************************************************************/
void XMLSettings::setValue(const QString &qStrKey, const QString &qStrValue)
{
    QSettings::setValue(qStrKey, qStrValue);
}

/*************************************************
函数名： setValue(const QString &qStrKey, const double &dValue)
输入参数： qStrKey：键
          dValue：值
输出参数： NULL
返回值： NULL
功能： 设置键对应的值
*************************************************************/
void XMLSettings::setValue(const QString &qStrKey, const double &dValue)
{
    QSettings::setValue(qStrKey, QString::number(dValue));
}

/*************************************************
函数名： value(const QString &qStrKey, const QVariant &defaultValue)
输入参数： qStrKey：键
          defaultValue：默认键值
输出参数： NULL
返回值： 键值
功能： 获取键对应的值
*************************************************************/
QString XMLSettings::value(const QString &qStrKey, const QVariant &defaultValue) const
{
    return QSettings::value(qStrKey, defaultValue).toString();
}

/*************************************************
函数名： readXmlFile(QIODevice &qIODevice, QSettings::SettingsMap &qSettingsMap)
输入参数： qIODevice：读设备
输出参数： qSettingsMap：QSettings键值映射
返回值： 操作结果
功能： 从xml文件读取数据到QSettings
*************************************************************/
bool XMLSettings::readXmlFile(QIODevice &qIODevice, QSettings::SettingsMap &qSettingsMap)
{
    QDomDocument qDomDoc;
    QString strError;
    int iErrorLine;
    int iErrorColumn;
    if( !qDomDoc.setContent(&qIODevice,&strError,&iErrorLine,&iErrorColumn) )
    {
        qWarning() << "XMLSettings::readXmlFile: error " << strError << "line:" << iErrorLine << "column:" << iErrorColumn;
        return false;
    }

    QDomElement qRoot = qDomDoc.documentElement();
    QDomNodeList qNodeList = qRoot.childNodes();

    for (int i = 0; i < qNodeList.count(); i++)
    {
        QDomNode qDomNode = qNodeList.at(i);
        if (qDomNode.isElement())
        {
            QString strGroup = qDomNode.toElement().tagName();
            parseNode(qDomNode, strGroup, qSettingsMap);
        }
    }
    qSettingsMap.insert( XMLSETTINGS_KEY_ROOT, qRoot.nodeName() );

    return true;
}

/*************************************************
函数名： parseNode(const QDomNode &qDomNode, const QString &qStrNode, QSettings::SettingsMap &qSettingsMap)
输入参数： qDomNode：QDomNode节点
          qStrNode：节点名称
输出参数： qSettingsMap：QSettings键值映射
返回值： NULL
功能： 解析当前节点
*************************************************************/
void XMLSettings::parseNode(const QDomNode &qDomNode, const QString &qStrNode, QSettings::SettingsMap &qSettingsMap)
{
    QDomNodeList qNodeList = qDomNode.childNodes();

    for (int i = 0; i < qNodeList.count(); i++)
    {
        QDomNode qNodeSon = qNodeList.at(i);
        if (qNodeSon.isElement())
        {
            QString strGroupSon = qStrNode + "/" + qNodeSon.toElement().tagName();
            parseNode(qNodeSon, strGroupSon, qSettingsMap);
        }

        if (qNodeSon.isText())
        {
            qSettingsMap.insert(qStrNode, qNodeSon.toText().data());
        }
    }
}

/*************************************************
函数名： writeXmlFile(QIODevice &qIODevice, const QSettings::SettingsMap &qSettingsMap)
输入参数： qIODevice：写设备
          qSettingsMap：QSettings键值映射
输出参数： NULL
返回值： 操作结果
功能： 将QSettings的数据写入xml文件
*************************************************************/
bool XMLSettings::writeXmlFile(QIODevice &qIODevice, const QSettings::SettingsMap &qSettingsMap)
{
    QDomDocument qDomDoc;
    QDomProcessingInstruction qInstruction;

    //创建xml文件头
    qInstruction = qDomDoc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    qDomDoc.appendChild(qInstruction);

    //创建根节点
    QString strRoot = qSettingsMap.value( XMLSETTINGS_KEY_ROOT, XMLSETTINGS_DEFAULT_ROOT ).toString();
    QDomElement qRoot = qDomDoc.createElement( strRoot );
    qDomDoc.appendChild(qRoot);

    //遍历key/value映射表，写入qDomDoc
    for (QSettings::SettingsMap::const_iterator j = qSettingsMap.constBegin(); j != qSettingsMap.constEnd(); j++)
    {
        if( XMLSETTINGS_KEY_ROOT != j.key() )
        {
            writeKeyToFile(qDomDoc, j.key(), j.value());
        }
    }

    //写入文件
    QTextStream out(&qIODevice);
    out.setCodec("UTF-8");
    qDomDoc.save(out, 4, QDomNode::EncodingFromTextStream);

    return true;
}

/*************************************************
函数名： writeKeyToFile(QDomDocument &qDomDoc, const QString &qKey, const QVariant &qValue)
输入参数： qDomDoc：QDomDocument对象
          qKey：键
          qValue：值
输出参数： NULL
返回值： NULL
功能： 将单个键值对写入QDomDocument
*************************************************************/
void XMLSettings::writeKeyToFile(QDomDocument &qDomDoc, const QString &qKey, const QVariant &qValue)
{
    int iSlashPos = qKey.indexOf(QLatin1Char('/'));
    QString strGroupName;
    QString strTmpKey = qKey;
    QDomElement elemFather;
    QDomElement qRoot = qDomDoc.documentElement();

    if (iSlashPos != -1)  //判断是否为根节点下的元素
    {
        strGroupName = strTmpKey.left(iSlashPos);
        strTmpKey.remove(0, iSlashPos + 1);

        //判断Group是否已存在
        elemFather = qRoot.firstChildElement(strGroupName);
        if (elemFather.isNull())
        {
            elemFather = qDomDoc.createElement(strGroupName);
            qRoot.appendChild(elemFather);
        }
        iSlashPos = strTmpKey.indexOf(QLatin1Char('/'));

        //递归建立所有子Group
        while (iSlashPos != -1)
        {
            strGroupName = strTmpKey.left(iSlashPos);
            strTmpKey.remove(0, iSlashPos + 1);

            QDomElement elem = elemFather.firstChildElement(strGroupName);
            if (elem.isNull())
            {
                elem = qDomDoc.createElement(strGroupName);
                elemFather.appendChild(elem);
            }

            elemFather = elem;
            iSlashPos = strTmpKey.indexOf(QLatin1Char('/'));
        }

        //建立最后一个key
        QDomElement elem = qDomDoc.createElement(strTmpKey);
        elemFather.appendChild(elem);
        elemFather = elem;
    }
    else  //根节点下的元素
    {
        elemFather = qDomDoc.createElement(strTmpKey);
        qRoot.appendChild(elemFather);
    }

    //写入value
    QString strValue = qValue.toString();
    QDomText qText = qDomDoc.createTextNode(strValue);
    elemFather.appendChild(qText);
}
