#include "Session.h"
#include "Server.h"
#include <iostream>
using namespace std;

void Session::Start() {
	memset(_data, 0 ,max_length);
	_socket.async_read_some(boost::asio::buffer(_data, max_length),
		std::bind(&Session::handle_read, this, std::placeholders::_1, std::placeholders::_2,shared_from_this()));
}

void Session::handle_read(const boost::system::error_code& error, size_t bytes_transferred , std::shared_ptr<Session> _self_shared) {
	if (b_head_parse) {
		int copy_len = 0;
		while (bytes_transferred > 0) {
			if (b_head_parse) {
				if (bytes_transferred + _recv_head_node->_cur_len < HEAD_LENGTH) {
					memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, bytes_transferred);
					_recv_head_node->_cur_len += bytes_transferred;
					memset(_data, 0, MAX_LENGTH);
					_socket.async_read_some(boost::asio::buffer(_data, max_length),
						[this, _self_shared](const boost::system::error_code& error, std::size_t bytes_tansferred) {
							handle_read(error, bytes_tansferred, _self_shared);
						});
					return;
				}
			}
		}
		int head_remain = HEAD_LENGTH - _recv_head_node->_cur_len;
		memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len , head_remain);
		copy_len += head_remain;
		bytes_transferred -= head_remain;
		short data_len = 0;
		memcpy(&data_len, _recv_head_node->_data, HEAD_LENGTH);
		cout << "data_len is:" << data_len << endl;
		if (data_len > MAX_LENGTH) {
			std::cout << "invalid data length is" << data_len << std::endl;
			_server->ClearSession(_uuid);
			return;
		}
		_recv_msg_node = std::make_shared<MsgNode>(data_len);
		if (bytes_transferred < data_len) {
			memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_transferred);
			_recv_msg_node->_cur_len += bytes_transferred;
			memset(_data, 0, MAX_LENGTH);
			_socket.async_read_some(boost::asio::buffer(_data, max_length),
				[this, _self_shared](const boost::system::error_code& error, std::size_t bytes_tansferred) {
					handle_read(error, bytes_tansferred, _self_shared);
				});
			b_head_parse = true;
			return;
		}
		memcpy()
	}
	
}

void Session::handle_write(const boost::system::error_code& error , std::shared_ptr<Session> _self_shared) {
	if (!error) {
		std::lock_guard <std::mutex> lock(_send_lock);
		_send_queue.pop();
		if (!_send_queue.empty()) {
			auto& msgnode = _send_queue.front();
			boost::asio::async_write(_socket, boost::asio::buffer(msgnode->_data, msgnode->max_len),
				bind(&Session::handle_write, this, std::placeholders::_1, _self_shared));
		}
	}
	else {
		std::cout << "handle write failed , error is" << error.what() << std::endl;
		_server->ClearSession(_uuid);
	}
}

std::string& Session::GetUuid() {
	return _uuid;
}

std::queue<std::shared_ptr<MsgNode>>& Session::GetSendQueue() {
	return _send_queue;
}

tcp::socket& Session::Socket() {
	return _socket;
}

void Session::send(char* msg, int max_length) {
	bool pending = false;
	std::lock_guard<std::mutex> lock(_send_lock);
	if (_send_queue.size() > 0) {
		pending = true;
	}
	_send_queue.push(std::make_shared<MsgNode>(msg , max_length));
	if (pending) {
		return;
	}
	boost::asio::async_write(_socket, boost::asio::buffer(msg, max_length),
		bind(&Session::handle_write, this, std::placeholders::_1, shared_from_this()));
}

