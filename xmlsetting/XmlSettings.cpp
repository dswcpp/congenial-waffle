#include "XmlSettings.h"
#include "tinyxml2.h"

#include <set>
#include <sstream>

using namespace tinyxml2;

static const char* KEY_ROOT = "Root";
static const char* DEFAULT_ROOT = "Station";

// ─── 辅助：将扁平化 key（如 "GroupA/SubGroup/key1"）解析为路径段 ──────────────
static std::vector<std::string> splitPath(const std::string& key)
{
    std::vector<std::string> parts;
    std::string token;
    for (char c : key) {
        if (c == '/') {
            if (!token.empty()) {
                parts.push_back(token);
                token.clear();
            }
        } else {
            token += c;
        }
    }
    if (!token.empty()) {
        parts.push_back(token);
    }
    return parts;
}

// ─── 辅助：递归解析 tinyxml2 节点到 flat map ────────────────────────────────
static void parseNode(XMLElement* elem, const std::string& prefix,
                      std::map<std::string, std::string>& map)
{
    if (elem == nullptr) return;

    for (XMLElement* child = elem->FirstChildElement(); child != nullptr;
         child = child->NextSiblingElement())
    {
        std::string childPath = prefix.empty() ? child->Name()
                                               : prefix + "/" + child->Name();

        // 叶节点：没有子元素，取文本内容
        if (child->FirstChildElement() == nullptr) {
            const char* text = child->GetText();
            map[childPath] = text ? text : "";
        } else {
            parseNode(child, childPath, map);
        }
    }
}

// ─── 辅助：将扁平 map 写入 XMLDocument ──────────────────────────────────────
static void writeKeyToDoc(XMLDocument& doc, XMLElement* root,
                          const std::string& key, const std::string& val)
{
    std::vector<std::string> parts = splitPath(key);
    if (parts.empty()) return;

    XMLElement* cur = root;
    for (size_t i = 0; i + 1 < parts.size(); ++i) {
        XMLElement* child = cur->FirstChildElement(parts[i].c_str());
        if (child == nullptr) {
            child = doc.NewElement(parts[i].c_str());
            cur->InsertEndChild(child);
        }
        cur = child;
    }

    // 最后一段是叶节点
    const std::string& leafName = parts.back();
    XMLElement* leaf = doc.NewElement(leafName.c_str());
    leaf->SetText(val.c_str());
    cur->InsertEndChild(leaf);
}

// ────────────────────────────────────────────────────────────────────────────

XMLSettings::XMLSettings(const std::string& fileName)
    : m_fileName(fileName), m_root(DEFAULT_ROOT)
{
    load();
}

void XMLSettings::setRoot(const std::string& root)
{
    m_root = root;
    m_map[KEY_ROOT] = root;
}

std::string XMLSettings::fullKey(const std::string& key) const
{
    if (m_groupStack.empty()) return key;

    std::string prefix;
    for (const auto& g : m_groupStack) {
        if (!prefix.empty()) prefix += '/';
        prefix += g;
    }
    return prefix + '/' + key;
}

void XMLSettings::setValue(const std::string& key, const std::string& value)
{
    m_map[fullKey(key)] = value;
}

void XMLSettings::setValue(const std::string& key, double value)
{
    // 去掉尾部多余的 0（模拟 Qt 的 QString::number 行为）
    std::ostringstream oss;
    oss << value;
    setValue(key, oss.str());
}

std::string XMLSettings::value(const std::string& key,
                               const std::string& defaultValue) const
{
    auto it = m_map.find(fullKey(key));
    if (it != m_map.end()) return it->second;
    return defaultValue;
}

void XMLSettings::beginGroup(const std::string& prefix)
{
    m_groupStack.push_back(prefix);
}

void XMLSettings::endGroup()
{
    if (!m_groupStack.empty()) {
        m_groupStack.pop_back();
    }
}

bool XMLSettings::contains(const std::string& key) const
{
    return m_map.count(fullKey(key)) > 0;
}

void XMLSettings::remove(const std::string& key)
{
    // 删除完整路径 key 本身，以及所有以 key+"/" 开头的子项
    auto it = m_map.begin();
    while (it != m_map.end()) {
        if (it->first == key ||
            (it->first.size() > key.size() &&
             it->first.compare(0, key.size() + 1, key + '/') == 0))
        {
            it = m_map.erase(it);
        } else {
            ++it;
        }
    }
}

void XMLSettings::clear()
{
    m_map.clear();
}

void XMLSettings::sync()
{
    XMLDocument doc;

    // XML 声明
    doc.InsertFirstChild(doc.NewDeclaration("xml version=\"1.0\" encoding=\"UTF-8\""));

    // 根节点
    std::string rootName = m_root.empty() ? DEFAULT_ROOT : m_root;
    auto it = m_map.find(KEY_ROOT);
    if (it != m_map.end() && !it->second.empty()) {
        rootName = it->second;
    }
    XMLElement* root = doc.NewElement(rootName.c_str());
    doc.InsertEndChild(root);

    // 写入所有 key（跳过 Root 元键）
    for (const auto& kv : m_map) {
        if (kv.first == KEY_ROOT) continue;
        writeKeyToDoc(doc, root, kv.first, kv.second);
    }

    doc.SaveFile(m_fileName.c_str());
}

int XMLSettings::childGroupCount(const std::string& groupPrefix) const
{
    // 统计直接子 group 数量：前缀下恰好多一段路径的唯一段
    std::string base = groupPrefix.empty() ? "" : groupPrefix + '/';
    std::set<std::string> children;

    for (const auto& kv : m_map) {
        if (kv.first == KEY_ROOT) continue;
        const std::string& k = kv.first;
        if (k.compare(0, base.size(), base) == 0) {
            std::string rest = k.substr(base.size());
            size_t slash = rest.find('/');
            if (slash != std::string::npos) {
                children.insert(rest.substr(0, slash));
            }
        }
    }
    return static_cast<int>(children.size());
}

std::string XMLSettings::fileName() const
{
    return m_fileName;
}

void XMLSettings::load()
{
    m_map.clear();

    XMLDocument doc;
    if (doc.LoadFile(m_fileName.c_str()) != XML_SUCCESS) {
        return;  // 文件不存在或解析失败，保持空 map
    }

    XMLElement* root = doc.RootElement();
    if (root == nullptr) return;

    m_root = root->Name();
    m_map[KEY_ROOT] = m_root;

    parseNode(root, "", m_map);
}
