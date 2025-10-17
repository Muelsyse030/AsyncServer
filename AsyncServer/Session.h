#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <map>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <queue>
#define MAX_LENGTH 1024*2
#define HEAD_LENGTH 2

using  boost::asio::ip::tcp;
using namespace std;
class Server;
class MsgNode;

class Session : public std::enable_shared_from_this<Session>{
public:
	Session(boost::asio::io_context& io_context , Server* server) : _socket(io_context) , _server(server){
		boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
		_uuid = boost::uuids::to_string(a_uuid);
	}
	tcp::socket& Socket();
	void Start();
	std::string& GetUuid();
	void send(const string& msg);
	std::queue<std::shared_ptr<MsgNode>>& GetSendQueue();

private:
	void handle_read_head();
	void handle_read_body(size_t body_length);
	void handle_write();
	void headle_error(const boost::system::error_code& ec);

	tcp::socket _socket;
	enum {max_length = 1024 };
	enum {HEAD_LEN = 4};
	char _data[max_length];
	std::vector<char> read_buffer;
	uint32_t _msg_len = 0;
	Server* _server;
	std::string _uuid;
	std::queue <std::string> _send_queue;
	std::mutex _send_lock;
	std::shared_ptr<MsgNode> _recv_msg_node;
	bool b_head_parse = false;
	std::shared_ptr<MsgNode> _recv_head_node;
};

class MsgNode {
	friend class Session;
public:
	MsgNode(char* msg, short max_len) : _total_len(max_len + HEAD_LENGTH) , _cur_len(0){
		_data = new char[ _total_len +1 ]();
		memcpy(_data, &max_len, HEAD_LENGTH);
		memcpy(_data + HEAD_LENGTH, msg, max_len);
		_data[_total_len] =  '\0' ;
	}

	MsgNode(short max_len) : _total_len(max_len) , _cur_len(0){
		_data = new char[_total_len + 1];
	}

	~MsgNode() {
		delete[] _data;
	}
	void Clear() {
		memset(_data, 0, _total_len);
		_cur_len = 0;
	}
private:
	short _cur_len;
	short _total_len;
	short max_len;
	char* _data;
};