#pragma once
#include <cstdint>

// ��ϢЭ����صĳ�������
constexpr size_t MAX_BODY_LENGTH = 1024 * 1024;  // 1MB

#pragma pack(push, 1)

// ��Ϣͷ����
struct MsgHeader {
    uint16_t msg_id;     // ��Ϣ����
    uint32_t body_len;   // �����ֽڳ��ȣ�����ͷ��
};