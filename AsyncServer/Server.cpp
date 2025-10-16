#include "Server.h"
#include <iostream>
#include <boost/asio.hpp>
#include "Session.h"
using namespace std;

Server::Server(boost::asio::io_context& io_context, short port) : _io_context(io_context),
_acceptor(io_context, tcp::endpoint(tcp::v4(), port)) {
	cout << "Server start success,on port :" << port << endl;
	start_accept();
}

void Server::start_accept() {
	std::shared_ptr<Session> new_session = std::make_shared<Session>(_io_context, this);
	_acceptor.async_accept(new_session->Socket(),
		std::bind(&Server::handle_accept, this, new_session, std::placeholders::_1));
}

void Server::handle_accept(std::shared_ptr<Session> new_session, const boost::system::error_code& error) {
	if (!error) {
		new_session->Start();
		_sessions.insert(make_pair(new_session->GetUuid(), new_session));
	}
	else {

	}
	start_accept();
}

void Server::ClearSession(std::string& uuid) {
	_sessions.erase(uuid);
}