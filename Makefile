# 配置管理库 - 生成 libconfig.so
CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -fPIC -O2
LDFLAGS  := -shared

BUILD_DIR := build
TARGET    := $(BUILD_DIR)/libconfig.so

SRCS := ConfigInstance.cpp \
        ConfigManager.cpp \
        xmlsetting/XmlSettings.cpp \
        xmlsetting/tinyxml2.cpp

OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS:.cpp=.o))

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(BUILD_DIR)
