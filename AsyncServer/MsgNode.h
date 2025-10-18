#pragma once
#include <cstring>
#include <memory>
#include "protocol.h"

// ��Ϣ�ڵ㣺�����װ����
class MsgNode {
public:
	// ���췢�ͽڵ㣨������Ϣͷ + ���ģ�
	MsgNode(uint16_t msg_id, const char* body, size_t len) {
		_header.msg_id = msg_id;
		_header.body_len = static_cast<uint32_t>(len);

		_total_len = sizeof(MsgHeader) + len;
		_buffer = std::make_unique<char[]>(_total_len);

		// �����ֽ���ת��
		uint16_t net_id = htons(msg_id);
		uint32_t net_len = htonl(len);

		std::memcpy(_buffer.get(), &net_id, sizeof(net_id));
		std::memcpy(_buffer.get() + sizeof(net_id), &net_len, sizeof(net_len));
		std::memcpy(_buffer.get() + sizeof(MsgHeader), body, len);
	}

	// ������սڵ㣨ֻ����ռ䣩
	explicit MsgNode(size_t len) {
		_total_len = len;
		_buffer = std::make_unique<char[]>(_total_len);
	}

	// �� buffer ����ͷ��
	static MsgHeader ParseHeader(const char* data) {
		MsgHeader hdr{};
		std::memcpy(&hdr.msg_id, data, sizeof(hdr.msg_id));
		std::memcpy(&hdr.body_len, data + sizeof(hdr.msg_id), sizeof(hdr.body_len));
		hdr.msg_id = ntohs(hdr.msg_id);
		hdr.body_len = ntohl(hdr.body_len);
		return hdr;
	}

	char* Data() { return _buffer.get(); }
	size_t Length() const { return _total_len; }
	const MsgHeader& Header() const { return _header; }

private:
	MsgHeader _header{};
	std::unique_ptr<char[]> _buffer;
	size_t _total_len{};
};
