#pragma once
#include <iostream>
#include <boost/asio.hpp>
#include <map>
#include "Session.h"

class Server {
public:
	Server(boost::asio::io_context& io_context, short port);
	void ClearSession(std::string& uuid);
private:
	void start_accept();
	void handle_accept(std::shared_ptr<Session> new_session, const boost::system::error_code& error);
	boost::asio::io_context& _io_context;
	boost::asio::ip::tcp::acceptor _acceptor;
	std::map<std::string, std::shared_ptr<Session>> _sessions;
};