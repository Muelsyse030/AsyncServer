#pragma once
#include <cstdint>
#pragma pack(push, 1)
// ��ϢЭ����صĳ�������
constexpr size_t MAX_BODY_LENGTH = 1024 * 1024;  // 1MB



// ��Ϣͷ����
struct MsgHeader {
	uint16_t msg_id;     // ��Ϣ����
	uint32_t body_len;   // �����ֽڳ��ȣ�����ͷ��
};