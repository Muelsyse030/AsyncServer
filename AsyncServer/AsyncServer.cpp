#include <iostream>
#include <boost/asio.hpp>
#include "Session.h"
#include "Server.h"

int main() {
	try {
		boost::asio::io_context io_context;
		using namespace std;
		Server s(io_context , 10086);
		io_context.run();
	}
	catch (std::exception& e) {
		std::cerr << "Exception" << e.what() << std::endl;
	}
	return 0;
}