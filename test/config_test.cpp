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

// ── main ──────────────────────────────────────────────────────────────────────
int main()
{
    bool ok = true;
    ok &= test_basic_rw();
    ok &= test_default_fallback();
    ok &= test_restore_default();
    ok &= test_array_mode();
    ok &= test_config_manager();

    // 清理生成的 xml 文件
    for (const char* f : {"test_config.xml", "test_nonexistent.xml",
                           "test_restore.xml", "test_array.xml", "test_manager.xml"}) {
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
