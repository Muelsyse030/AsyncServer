#pragma once
#include <cstdint>

// 消息协议相关的常量定义
constexpr size_t MAX_BODY_LENGTH = 1024 * 1024;  // 1MB

#pragma pack(push, 1)

// 消息头定义
struct MsgHeader {
    uint16_t msg_id;     // 消息类型
    uint32_t body_len;   // 正文字节长度（不含头）
};