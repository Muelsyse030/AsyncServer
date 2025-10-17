#include "Session.h"
#include "Server.h"
#include <iostream>
using namespace std;

void Session::Start() {
	cout << "New Session Start:" << endl;
	handle_read_head();
}

void Session::handle_read_head() {
	auto self(shared_from_this());
	boost::asio::async_read(_socket,
		boost::asio::buffer(&_msg_len , HEAD_LEN),
		[this , self](const boost::system::error_code& ec , std::size_t) {
			if (!ec) {
				_msg_len = ntohl(_msg_len);
				if (_msg_len > 10 * 1024 * 1024) {
					std::cerr << "Message too large, closing connection." << std::endl;
					_server->ClearSession(_uuid);
					return;
					
				}
				handle_read_body(_msg_len);
			}
			else {
				headle_error(ec);
			}
		});
}

void Session::handle_read_body(size_t body_length) {
	auto self(shared_from_this());
	read_buffer.resize(body_length);
	boost::asio::async_read(_socket,
		boost::asio::buffer(read_buffer.data(), body_length),
		[this, self](const boost::system::error_code& ec , size_t){
			if (!ec) {
				string msg(read_buffer.begin(), read_buffer.end());
				std::cout << "Server received full message: " << msg << std::endl;
				send(msg);
				handle_read_head();
			}else{
				headle_error(ec);
			}
		});
}

void Session::handle_write() {
	auto self(shared_from_this());
	std::string& msg = _send_queue.front();

	boost::asio::async_write(_socket,
		boost::asio::buffer(msg.data(), msg.size()),
		[this , self](const boost::system::error_code& ec , size_t){
			if (!ec) {
				std::lock_guard<std::mutex> lock(_send_lock);
				_send_queue.pop();
				if (!_send_queue.empty())
					handle_write();
			}
			else {
				headle_error(ec);
			}
		});
}

std::string& Session::GetUuid() {
	return _uuid;
}

tcp::socket& Session::Socket() {
	return _socket;
}

void Session::send(const std::string& msg) {

	uint32_t len = htonl(static_cast<uint32_t>(msg.size()));

	string packet;
	packet.resize(4 + msg.size());
	memcpy(packet.data(), &len , 4);
	memcpy(packet.data() + 4, msg.data(), msg.size());
	bool is_writing = false;{
		std::lock_guard<std::mutex> lock(_send_lock);
		is_writing = !_send_queue.empty();
		_send_queue.push(std::move(packet));
	}
	if (!is_writing) {
		handle_write();
	}
}
void Session::headle_error(const boost::system::error_code& ec) {
	std::cout << "session error is:" << ec.message() << std::endl;
	_server->ClearSession(_uuid);
}

