/**
 * config_test.cpp
 * 简单的手工测试，覆盖以下场景：
 *   1. 基本读写（NUMBER / TEXT）
 *   2. 默认值回退
 *   3. restoreDefault
 *   4. Array 模式（写入后读回）
 *   5. ConfigManager 单例
 */

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>

#include "ConfigManager.h"

namespace fs = std::filesystem;

// ── 测试辅助宏 ────────────────────────────────────────────────────────────────
#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "[FAIL] " << __FILE__ << ":" << __LINE__ \
                      << "  " << #cond << "\n"; \
            return false; \
        } \
        std::cout << "[PASS] " << #cond << "\n"; \
    } while(0)

// ── 配置表定义 ────────────────────────────────────────────────────────────────
enum GroupCode { GRP_MAIN = 1, GRP_WIFI = 2 };
enum KeyCode   { KEY_IP = 1, KEY_PORT = 2, KEY_SSID = 3, KEY_PWD = 4 };

static Config::KeyInfo keysMain[] = {
    { KEY_IP,   "ip",   Config::TEXT,   "192.168.1.1", 0, 0 },
    { KEY_PORT, "port", Config::NUMBER, "8080",        1, 65535 },
};
static Config::KeyInfo keysWifi[] = {
    { KEY_SSID, "ssid", Config::TEXT, "MySSID",  0, 0 },
    { KEY_PWD,  "pwd",  Config::TEXT, "MyPwd",   0, 0 },
};

static Config::GroupInfo groupMain  = { GRP_MAIN, Config::NORMAL, "Main",  keysMain,  2, nullptr, 0 };
static Config::GroupInfo groupWifi  = { GRP_WIFI, Config::ARRAY,  "Wifi",  keysWifi,  2, nullptr, 0 };
static Config::GroupInfo* groups[]  = { &groupMain, &groupWifi };

static Config::ConfigInfo cfgInfo = { 1, "test_config.xml", "Config", groups, 2 };

// ── 测试函数 ──────────────────────────────────────────────────────────────────
bool test_basic_rw()
{
    std::cout << "\n=== test_basic_rw ===\n";
    ConfigInstance inst(&cfgInfo);

    // 写入
    inst.setValue("10.0.0.1", KEY_IP, GRP_MAIN);
    inst.setValue(9090.0,     KEY_PORT, GRP_MAIN);

    // 读回
    CHECK(inst.value(KEY_IP,   GRP_MAIN) == "10.0.0.1");
    CHECK(inst.value(KEY_PORT, GRP_MAIN) == "9090");

    return true;
}

bool test_default_fallback()
{
    std::cout << "\n=== test_default_fallback ===\n";
    // 使用全新文件名，不存在时应回退默认值
    Config::ConfigInfo ci = cfgInfo;
    ci.strFileName = "test_nonexistent.xml";
    ConfigInstance inst(&ci);

    CHECK(inst.value(KEY_IP,   GRP_MAIN) == "192.168.1.1");
    CHECK(inst.value(KEY_PORT, GRP_MAIN) == "8080");

    return true;
}

bool test_restore_default()
{
    std::cout << "\n=== test_restore_default ===\n";
    Config::ConfigInfo ci = cfgInfo;
    ci.strFileName = "test_restore.xml";
    ConfigInstance inst(&ci);

    inst.setValue("1.2.3.4", KEY_IP, GRP_MAIN);
    CHECK(inst.value(KEY_IP, GRP_MAIN) == "1.2.3.4");

    inst.restoreDefault(GRP_MAIN);
    CHECK(inst.value(KEY_IP, GRP_MAIN) == "192.168.1.1");

    return true;
}

bool test_array_mode()
{
    std::cout << "\n=== test_array_mode ===\n";
    Config::ConfigInfo ci = cfgInfo;
    ci.strFileName = "test_array.xml";
    ConfigInstance inst(&ci);

    // 写入两条 Wifi 记录
    inst.beginArray(GRP_WIFI);
    inst.beginItem(0);
    inst.setItemValue("HomeNet",  KEY_SSID);
    inst.setItemValue("pass1234", KEY_PWD);
    inst.endItem();
    inst.beginItem(1);
    inst.setItemValue("OfficeNet", KEY_SSID);
    inst.setItemValue("work5678",  KEY_PWD);
    inst.endItem();
    inst.endArray();

    // 读回
    inst.beginArray(GRP_WIFI);
    inst.beginItem(0);
    CHECK(inst.itemValue(KEY_SSID) == "HomeNet");
    CHECK(inst.itemValue(KEY_PWD)  == "pass1234");
    inst.endItem();
    inst.beginItem(1);
    CHECK(inst.itemValue(KEY_SSID) == "OfficeNet");
    CHECK(inst.itemValue(KEY_PWD)  == "work5678");
    inst.endItem();
    inst.endArray();

    return true;
}

bool test_config_manager()
{
    std::cout << "\n=== test_config_manager ===\n";
    Config::ConfigInfo ci = cfgInfo;
    ci.iCode       = 42;
    ci.strFileName = "test_manager.xml";

    auto* mgr = ConfigManager::instance();
    Config::ErrorCode ec = mgr->addConfig(&ci);
    CHECK(ec == Config::ERROR_NONE);

    // 重复添加应返回 CONFIG_ALREADY_EXIST
    CHECK(mgr->addConfig(&ci) == Config::CONFIG_ALREADY_EXIST);

    ConfigInstance* inst = mgr->config(42);
    CHECK(inst != nullptr);

    inst->setValue("172.16.0.1", KEY_IP, GRP_MAIN);
    CHECK(inst->value(KEY_IP, GRP_MAIN) == "172.16.0.1");

    return true;
}

// ── 错误场景测试 ─────────────────────────────────────────────────────────────

bool test_invalid_group_key()
{
    std::cout << "\n=== test_invalid_group_key ===\n";
    Config::ConfigInfo ci = cfgInfo;
    ci.strFileName = "test_error.xml";
    ConfigInstance inst(&ci);

    // 不存在的组
    std::string val = inst.value(KEY_IP, static_cast<Config::GroupCode>(999));
    CHECK(inst.lastError() == Config::GROUP_NOT_EXIST);

    // 不存在的键
    val = inst.value(static_cast<Config::KeyCode>(999), GRP_MAIN);
    CHECK(inst.lastError() == Config::KEY_NOT_EXIST);

    // 写入不存在的组
    inst.setValue("test", static_cast<Config::KeyCode>(999), GRP_MAIN);
    CHECK(inst.lastError() == Config::KEY_NOT_EXIST);

    return true;
}

bool test_number_out_of_range()
{
    std::cout << "\n=== test_number_out_of_range ===\n";
    Config::ConfigInfo ci = cfgInfo;
    ci.strFileName = "test_range.xml";
    ConfigInstance inst(&ci);

    // port 范围是 1~65535，写入 0 应报超范围
    inst.setValue(0.0, KEY_PORT, GRP_MAIN);
    CHECK(inst.lastError() == Config::DATA_OUT_OF_RANGE);

    // 写入 99999 也应报超范围
    inst.setValue(99999.0, KEY_PORT, GRP_MAIN);
    CHECK(inst.lastError() == Config::DATA_OUT_OF_RANGE);

    // 写入有效值应成功
    inst.setValue(443.0, KEY_PORT, GRP_MAIN);
    CHECK(inst.value(KEY_PORT, GRP_MAIN) == "443");

    return true;
}

bool test_datetime_validation()
{
    std::cout << "\n=== test_datetime_validation ===\n";

    // 创建一个带 DATETIME 类型键的配置
    enum { KEY_DATE = 10, GRP_DT = 10 };
    static Config::KeyInfo keysDate[] = {
        { KEY_DATE, "date", Config::DATETIME, "2026-01-01", 0, 0 },
    };
    static Config::GroupInfo groupDate = { GRP_DT, Config::NORMAL, "DateGroup", keysDate, 1, nullptr, 0 };
    static Config::GroupInfo* groupsDate[] = { &groupDate };
    static Config::ConfigInfo ciDate = { 99, "test_datetime.xml", "Config", groupsDate, 1 };

    ConfigInstance inst(&ciDate);

    // 有效日期
    inst.setValue("2026-03-15", KEY_DATE, GRP_DT);
    CHECK(inst.lastError() == Config::ERROR_NONE);

    // 有效日期时间
    inst.setValue("2026-03-15T10:30:00", KEY_DATE, GRP_DT);
    CHECK(inst.lastError() == Config::ERROR_NONE);

    // 无效格式
    inst.setValue("not-a-date", KEY_DATE, GRP_DT);
    CHECK(inst.lastError() == Config::DATA_INVALID);

    // 月份超范围（13月）
    inst.setValue("2026-13-01", KEY_DATE, GRP_DT);
    CHECK(inst.lastError() == Config::DATA_INVALID);

    // 日超范围（32日）
    inst.setValue("2026-01-32", KEY_DATE, GRP_DT);
    CHECK(inst.lastError() == Config::DATA_INVALID);

    // 月份为 0
    inst.setValue("2026-00-15", KEY_DATE, GRP_DT);
    CHECK(inst.lastError() == Config::DATA_INVALID);

    return true;
}

bool test_addconfig_null()
{
    std::cout << "\n=== test_addconfig_null ===\n";
    auto* mgr = ConfigManager::instance();

    // nullptr 输入
    CHECK(mgr->addConfig(nullptr) == Config::INPUT_ERROR);

    return true;
}

// ── main ──────────────────────────────────────────────────────────────────────
int main()
{
    bool ok = true;
    ok &= test_basic_rw();
    ok &= test_default_fallback();
    ok &= test_restore_default();
    ok &= test_array_mode();
    ok &= test_config_manager();
    ok &= test_invalid_group_key();
    ok &= test_number_out_of_range();
    ok &= test_datetime_validation();
    ok &= test_addconfig_null();

    // 清理生成的 xml 文件
    for (const char* f : {"test_config.xml", "test_nonexistent.xml",
                           "test_restore.xml", "test_array.xml", "test_manager.xml",
                           "test_error.xml", "test_range.xml", "test_datetime.xml"}) {
        fs::remove(f);
    }

    if (ok) {
        std::cout << "\n*** All tests PASSED ***\n";
        return 0;
    } else {
        std::cout << "\n*** Some tests FAILED ***\n";
        return 1;
    }
}
