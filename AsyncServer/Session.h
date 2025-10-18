#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <queue>
#include <mutex>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "protocol.h"
#include "MsgNode.h"
#include <memory>

using boost::asio::ip::tcp;

class Server;  // Ç°ÏòÉùÃ÷

class Session : public std::enable_shared_from_this<Session> {
public:
	Session(boost::asio::io_context& io_context, Server* server);
	tcp::socket& Socket() { return _socket; }
	void Start();
	std::string& GetUuid() { return _uuid; }
	void Send(const std::shared_ptr<MsgNode>& msg);

private:
	void DoReadHeader();
	void DoReadBody();
	void DoWrite();
	void HandleError(const boost::system::error_code& ec);

	tcp::socket _socket;
	Server* _server;
	std::string _uuid;

	MsgHeader _current_header{};
	std::vector<char> _body_buffer;

	std::queue<std::shared_ptr<MsgNode>> _send_queue;
	std::mutex _send_lock;
};

class LogicNode {
	friend class LogicSystem;
public:
	LogicNode(std::shared_ptr<Session>, std::shared_ptr<MsgNode>);
private:
	std::shared_ptr<Session> _session;
	std::shared_ptr<MsgNode> _recvnode;
};
