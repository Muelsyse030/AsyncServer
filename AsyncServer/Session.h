#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <queue>
#include <mutex>
#include <memory>

#define MAX_BODY_LENGTH 2048
#define HEADER_LENGTH 4

using boost::asio::ip::tcp;
using namespace std;

class Server;
class MsgNode;

// ===================== MsgNode =====================
class MsgNode {
public:
    // 构造发送节点：自动封包（4字节长度前缀 + 数据）
    MsgNode(const char* msg, size_t len)
        : _total_len(len + HEADER_LENGTH), _cur_len(0)
    {
        _data = std::make_unique<char[]>(_total_len);
        uint32_t net_len = htonl(static_cast<uint32_t>(len));
        memcpy(_data.get(), &net_len, HEADER_LENGTH);
        memcpy(_data.get() + HEADER_LENGTH, msg, len);
    }

    // 构造接收节点：只分配内存
    explicit MsgNode(size_t len)
        : _total_len(len), _cur_len(0)
    {
        _data = std::make_unique<char[]>(_total_len);
    }

    char* Data() { return _data.get(); }
    size_t Length() const { return _total_len; }

private:
    std::unique_ptr<char[]> _data;
    size_t _total_len;
    size_t _cur_len;
};

// ===================== Session =====================
class Session : public std::enable_shared_from_this<Session> {
public:
    Session(boost::asio::io_context& io_context, Server* server);

    tcp::socket& Socket() { return _socket; }
    void Start();
    std::string& GetUuid() { return _uuid; }
    void Send(const std::shared_ptr<MsgNode>& msg);

private:
    void DoReadHeader();
    void DoReadBody(size_t body_length);
    void DoWrite();
    void HandleError(const boost::system::error_code& ec);

private:
    tcp::socket _socket;
    Server* _server;
    std::string _uuid;

    uint32_t _msg_len = 0;
    std::vector<char> _read_buffer;

    std::queue<std::shared_ptr<MsgNode>> _send_queue;
    std::mutex _send_lock;
};
