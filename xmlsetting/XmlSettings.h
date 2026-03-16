#pragma once

#include <map>
#include <string>
#include <vector>

/**
 * @brief XML 格式的配置文件管理，替代原基于 QSettings 的实现。
 *
 * 内部使用扁平化 key-value map，key 路径以 '/' 分隔（与 QSettings 行为一致）。
 * 通过 tinyxml2 实现 XML 读写。
 */
class XMLSettings
{
public:
    explicit XMLSettings(const std::string& fileName);
    ~XMLSettings() = default;

    // 设置根节点名称
    void setRoot(const std::string& root);

    // 设置键值（相对于当前 group 前缀）
    void setValue(const std::string& key, const std::string& value);
    void setValue(const std::string& key, double value);

    // 获取键值，不存在时返回 defaultValue
    std::string value(const std::string& key, const std::string& defaultValue = "") const;

    // 进入/退出 group（影响后续 setValue/value 的 key 前缀）
    void beginGroup(const std::string& prefix);
    void endGroup();

    // 判断当前完整路径的 key 是否存在
    bool contains(const std::string& key) const;

    // 删除以 key（完整路径）为前缀的所有条目
    void remove(const std::string& key);

    // 清除所有键值
    void clear();

    // 将内存中的 map 写入 XML 文件
    void sync();

    // 返回当前 group 前缀下直接子 group 的数量
    int childGroupCount(const std::string& groupPrefix) const;

    // 返回文件名
    std::string fileName() const;

private:
    // 将相对 key 转换为带当前前缀的完整 key
    std::string fullKey(const std::string& key) const;

    // 从 XML 文件加载到 map
    void load();

    std::string m_fileName;
    std::string m_root;
    std::map<std::string, std::string> m_map;
    std::vector<std::string> m_groupStack;  // beginGroup 的路径栈
};
