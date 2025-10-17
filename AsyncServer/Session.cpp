#include "Session.h"
#include "Server.h"
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

// ===================== 异步读取头部 =====================
void Session::DoReadHeader() {
    auto self = shared_from_this();

    boost::asio::async_read(
        _socket,
        boost::asio::buffer(&_msg_len, HEADER_LENGTH),
        [this, self](const boost::system::error_code& ec, size_t /*length*/) {
            if (!ec) {
                _msg_len = ntohl(_msg_len);
                if (_msg_len > 10 * 1024 * 1024) {
                    cerr << "Message too large, closing connection." << endl;
                    HandleError(boost::asio::error::operation_aborted);
                    return;
                }
                DoReadBody(_msg_len);
            }
            else {
                HandleError(ec);
            }
        });
}

// ===================== 异步读取正文 =====================
void Session::DoReadBody(size_t body_length) {
    auto self = shared_from_this();
    _read_buffer.resize(body_length);

    boost::asio::async_read(
        _socket,
        boost::asio::buffer(_read_buffer.data(), body_length),
        [this, self](const boost::system::error_code& ec, size_t /*length*/) {
            if (!ec) {
                string msg(_read_buffer.begin(), _read_buffer.end());
                cout << "Server received message: " << msg << endl;
                //反序列化json
				Json::Value root;
				Json::CharReaderBuilder readerBuilder;
                JSONCPP_STRING errs;
				std::istringstream s(msg);
                if (!Json::parseFromStream(readerBuilder, s, &root, &errs)) {
                    cerr << "JSON parse error: " << errs << endl;
                    HandleError(boost::asio::error::invalid_argument);
                    return;
                }
                string type = root.get("type", "unknown").asString();
                string content = root.get("content", "").asString();
                cout << "Parsed JSON: type=" << type << ", content=" << content << endl;
                //构造json响应
                Json::Value reply;
                reply["status"] = "ok";
                reply["echo_type"] = type;
                reply["echo_content"] = content;
                reply["uuid"] = _uuid;

                Json::StreamWriterBuilder writer;
                string response = Json::writeString(writer, reply);
                //打包发送

                auto reply_msg = make_shared<MsgNode>(response.data(), response.size());
                Send(reply_msg);

                DoReadHeader();  // 继续下一条
            }
            else {
                HandleError(ec);
            }
        });
}

// ===================== 异步发送 =====================
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

void Session::DoWrite() {
    auto self = shared_from_this();

    std::shared_ptr<MsgNode> msg;
    {
        std::lock_guard<std::mutex> lock(_send_lock);
        if (_send_queue.empty()) return;
        msg = _send_queue.front();
    }

    boost::asio::async_write(
        _socket,
        boost::asio::buffer(msg->Data(), msg->Length()),
        [this, self](const boost::system::error_code& ec, size_t /*length*/) {
            if (!ec) {
                std::lock_guard<std::mutex> lock(_send_lock);
                _send_queue.pop();
                if (!_send_queue.empty())
                    DoWrite();
            }
            else {
                HandleError(ec);
            }
        });
}

// ===================== 错误处理 =====================
void Session::HandleError(const boost::system::error_code& ec) {
    cerr << "Session " << _uuid << " error: " << ec.message() << endl;
    _socket.close();
    if (_server)
        _server->ClearSession(_uuid);
}
