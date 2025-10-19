#include <iostream>
#include <boost/asio.hpp>
#include "Session.h"
#include "Server.h"
#include <mutex>
#include <thread>
#include <csignal> // 添加此头文件以支持 signal

using namespace std;
bool bstop = false;
std::condition_variable cv;
std::mutex mutex_quit;
void sig_handler(int sig) {
	if (sig == SIGINT || sig == SIGTERM) {
		std::unique_lock<std::mutex> lock_quit(mutex_quit);
		bstop = true;
		cv.notify_one();

	}
}

int main() {
	try {
		boost::asio::io_context io_context;
		std::thread net_work_thread([&io_context]{
			Server s(io_context, 10086);
			io_context.run();
			});

		std::signal(SIGINT , sig_handler);   
		std::signal(SIGTERM, sig_handler);   

		while (!bstop) {
			std::unique_lock<std::mutex> lock_quit(mutex_quit);
			cv.wait(lock_quit);
		}
		io_context.stop();
		net_work_thread.join();
	}
	catch (std::exception& e) {
		std::cerr << "Exception" << e.what() << std::endl;
	}
	return 0;
}