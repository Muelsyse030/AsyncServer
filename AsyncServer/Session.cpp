#include "Session.h"
#include "Server.h"
#include "MsgNode.h"
#include "LogicSystem.h"  // 添加 LogicSystem 头文件
#include <iostream>

using namespace std;

Session::Session(boost::asio::io_context& io_context, Server* server)
    : _socket(io_context), _server(server)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
}

void Session::Start() {
    cout << "New session started. UUID: " << _uuid << endl;
    DoReadHeader();
}

void Session::DoReadHeader() {
    auto self = shared_from_this();

    // 读取消息头（6字节）
    boost::asio::async_read(_socket,
        boost::asio::buffer(&_current_header, sizeof(MsgHeader)),
        [this, self](boost::system::error_code ec, size_t) {
            if (!ec) {
                // 网络序->主机序
                _current_header.msg_id = ntohs(_current_header.msg_id);
                _current_header.body_len = ntohl(_current_header.body_len);

                std::cout << "[Header] msg_id=" << _current_header.msg_id
                    << " body_len=" << _current_header.body_len << std::endl;

                if (_current_header.body_len > MAX_BODY_LENGTH) {
                    std::cerr << "Invalid body length: " << _current_header.body_len << std::endl;
                    HandleError(ec);
                    return;
                }

                _body_buffer.resize(_current_header.body_len);
                DoReadBody();
            }
            else {
                HandleError(ec);
            }
        });
}

void Session::DoReadBody() {
    std::cout << "[TRACE] Enter DoReadBody by thread "
        << std::this_thread::get_id() << std::endl;

    auto self = shared_from_this();

    boost::asio::async_read(_socket,
        boost::asio::buffer(_body_buffer.data(), _current_header.body_len),
        [this, self](boost::system::error_code ec, size_t) {
            if (!ec) {
                std::string body(_body_buffer.begin(), _body_buffer.end());
                std::cout << "[Recv] id=" << _current_header.msg_id
                    << " len=" << _current_header.body_len << std::endl;

                // 创建 MsgNode（包含原始数据）
                auto msg = std::make_shared<MsgNode>(_current_header.msg_id,
                    body.data(),
                    body.size());

                // 创建 LogicNode 并提交到逻辑系统
                auto logic_node = std::make_shared<LogicNode>(shared_from_this(), msg);

                // 获取 LogicSystem 单例并提交消息
                auto logic_system = LogicSystem::GetInstance();
                logic_system->PostMsgToQueue(logic_node);

                // 继续读取下一条消息
                DoReadHeader();
            }
            else {
                HandleError(ec);
            }
        });
}

void Session::DoWrite() {
    std::cout << "[TRACE] Enter DoWrite by thread "
        << std::this_thread::get_id() << std::endl;

    auto self = shared_from_this();
    std::shared_ptr<MsgNode> msg;

    // 获取发送队列中的消息（但不移除）
    {
        std::lock_guard<std::mutex> lock(_send_lock);
        if (_send_queue.empty())
            return;
        msg = _send_queue.front();
    }

    // 异步发送数据
    boost::asio::async_write(
        _socket,
        boost::asio::buffer(msg->Data(), msg->Length()),
        [this, self](boost::system::error_code ec, std::size_t /*bytes*/) {
            if (!ec) {
                bool has_next = false;

                // 成功发送后移除消息（出队）
                {
                    std::lock_guard<std::mutex> lock(_send_lock);
                    _send_queue.pop();
                    has_next = !_send_queue.empty();
                }

                // 如果还有消息继续发送
                if (has_next)
                    DoWrite();
            }
            else {
                HandleError(ec);
            }
        });
}

void Session::HandleError(const boost::system::error_code& ec) {
    cerr << "Session " << _uuid << " error: " << ec.message() << endl;
    _socket.close();
    if (_server)
        _server->ClearSession(_uuid);
}

void Session::Send(const shared_ptr<MsgNode>& msg) {
    bool writing = false;
    {
        std::lock_guard<std::mutex> lock(_send_lock);
        writing = !_send_queue.empty();
        _send_queue.push(msg);
    }

    if (!writing)
        DoWrite();
}

LogicNode::LogicNode(shared_ptr<Session> session, shared_ptr<MsgNode> recvnode)
    : _session(session), _recvnode(recvnode) {
}