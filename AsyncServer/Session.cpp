#include "Session.h"
#include "Server.h"
#include "MsgNode.h"
#include <iostream>
#include <json/json.h>  
#include <sstream>

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

    // ��ȡ��������Ϣͷ��6�ֽڣ�
    boost::asio::async_read(_socket,
        boost::asio::buffer(&_current_header, sizeof(MsgHeader)),
        [this, self](boost::system::error_code ec, size_t) {
            if (!ec) {
                // ������->������
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
                    << " len=" << _current_header.body_len
                    << " body=" << body << std::endl;

                // JSON parse...
                Json::Value root;
                Json::CharReaderBuilder readerBuilder;
                JSONCPP_STRING errs;
                std::istringstream s(body);
                if (!Json::parseFromStream(readerBuilder, s, &root, &errs)) {
                    std::cerr << "[Error] JSON parse failed: " << errs << std::endl;
                    HandleError(ec);
                    return;
                }

                std::string type = root.get("type", "unknown").asString();
                std::string content = root.get("content", "").asString();
                std::cout << "[JSON] type=" << type << " content=" << content << std::endl;

                // ������Ӧ JSON
                Json::Value reply;
                reply["status"] = "ok";
                reply["echo_type"] = type;
                reply["echo_content"] = content;
                reply["msg_id"] = _current_header.msg_id;

                Json::StreamWriterBuilder writer;
                std::string response = Json::writeString(writer, reply);

                // ���Ϊ MsgNode
                auto msg = std::make_shared<MsgNode>(_current_header.msg_id,
                    response.data(),
                    response.size());

                // �������� push������ȡ��־����Ҫ�����ڵ��� DoWrite()
                bool write_in_progress = false;
                {
                    std::lock_guard<std::mutex> lock(_send_lock);
                    write_in_progress = !_send_queue.empty();
                    _send_queue.push(msg);
                }
                if (!write_in_progress) {
                    DoWrite(); // �����ͷţ���ȫ����
                }

                // ����������һ��
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

    //��ȡ��Ҫ���͵���Ϣ������ȡ���ף�
    {
        std::lock_guard<std::mutex> lock(_send_lock);
        if (_send_queue.empty())
            return;
        msg = _send_queue.front();
    }

    // �첽д�����磨�������������У�
    boost::asio::async_write(
        _socket,
        boost::asio::buffer(msg->Data(), msg->Length()),
        [this, self](boost::system::error_code ec, std::size_t /*bytes*/) {
            if (!ec) {
                bool has_next = false;

                // �����޸Ķ��У����ӣ�
                {
                    std::lock_guard<std::mutex> lock(_send_lock);
                    _send_queue.pop();
                    has_next = !_send_queue.empty();
                }

                // ���ͷź��ٵݹ�
                if (has_next)
                    DoWrite(); //�����ͷţ����ﰲȫ
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
