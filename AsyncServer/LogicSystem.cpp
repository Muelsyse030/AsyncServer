#include "LogicSystem.h"
#include <iostream>
#include <json/json.h>
#include <sstream>

using namespace std;

LogicSystem::LogicSystem()
	: _b_stop(false)
{
	RegisterCallBack();
	_worker_thread = std::thread(&LogicSystem::DealMsg, this);
}

LogicSystem::~LogicSystem() {
	_b_stop = true;
	cv.notify_all();
	if (_worker_thread.joinable()) {
		_worker_thread.join();
	}
}

void LogicSystem::RegisterCallBack() {
	
	_fun_callback[MSG_HELLO_WORD] = [this](std::shared_ptr<Session> session, uint16_t msg_id, const std::shared_ptr<MsgNode>& msg_data) {
		this->HelloWordCallBack(session, msg_id, msg_data);
		};

	// ����ע�����ص�����
	// _fun_callback[OTHER_MSG_TYPE] = [this](...){ ... };
}

void LogicSystem::PostMsgToQueue(shared_ptr<LogicNode> msg) {
	std::unique_lock<std::mutex> lock(_mutex);
	_msg_queue.push(msg);
	cv.notify_one();
}

void LogicSystem::HelloWordCallBack(std::shared_ptr<Session> session, const std::uint16_t msg_id, const std::shared_ptr<MsgNode>& msg_data) {
	try {
		// ��ȡ��Ϣ�����ݣ�������Ϣͷ��
		std::string body_data(msg_data->Data() + sizeof(MsgHeader), msg_data->Header().body_len);

		// JSON ����
		Json::Value root;
		Json::CharReaderBuilder readerBuilder;
		JSONCPP_STRING errs;
		std::istringstream s(body_data);

		if (!Json::parseFromStream(readerBuilder, s, &root, &errs)) {
			std::cerr << "[LogicSystem] JSON parse failed: " << errs << std::endl;
			SendErrorResponse(session, msg_id, "Invalid JSON format");
			return;
		}

		// ��ȡҵ������
		std::string type = root.get("type", "unknown").asString();
		std::string content = root.get("content", "").asString();
		int id = root.get("id", 0).asInt();

		std::cout << "[LogicSystem] Processing message - Type: " << type
			<< ", Content: " << content << ", ID: " << id << std::endl;

		// ҵ���߼�����
		Json::Value response;
		response["status"] = "success";
		response["msg_id"] = msg_id;
		response["processed_data"] = "Server processed: " + content;
		response["original_id"] = id;

		// ������Ӧ
		Json::StreamWriterBuilder writer;
		std::string response_str = Json::writeString(writer, response);

		auto return_msg = std::make_shared<MsgNode>(msg_id, response_str.c_str(), response_str.length());
		session->Send(return_msg);
	}
	catch (const std::exception& e) {
		std::cerr << "[LogicSystem] Error processing message: " << e.what() << std::endl;
		SendErrorResponse(session, msg_id, "Internal processing error");
	}
}

void LogicSystem::DealMsg() {
	for (;;) {
		std::shared_ptr<LogicNode> logic_node;
		{
			std::unique_lock<std::mutex> lock(_mutex);
			// �ȴ�ֱ������Ϣ���յ�ֹͣ�ź�
			cv.wait(lock, [this] {
				return _b_stop || !_msg_queue.empty();
				});

			// ����Ƿ���Ҫֹͣ
			if (_b_stop && _msg_queue.empty()) {
				return;
			}

			// ��ȡ�����еĵ�һ����Ϣ
			logic_node = _msg_queue.front();
			_msg_queue.pop();
		}

		// ������Ϣ
		if (logic_node != nullptr && logic_node->_recvnode != nullptr) {
			uint16_t msg_id = logic_node->_recvnode->Header().msg_id;
			auto it = _fun_callback.find(msg_id);
			if (it != _fun_callback.end()) {
				// ���߼��߳��д�����Ϣ������JSON������
				it->second(logic_node->_session, msg_id, logic_node->_recvnode);
			}
			else {
				std::cerr << "No callback registered for message ID: " << msg_id << std::endl;
				SendErrorResponse(logic_node->_session, msg_id, "Unknown message type");
			}
		}
	}
}

void LogicSystem::SendErrorResponse(std::shared_ptr<Session> session, uint16_t msg_id, const std::string& error_msg) {
	try {
		if (!session) return;

		Json::Value error_response;
		error_response["status"] = "error";
		error_response["msg_id"] = msg_id;
		error_response["error"] = error_msg;

		Json::StreamWriterBuilder writer;
		std::string response_str = Json::writeString(writer, error_response);

		auto error_msg_node = std::make_shared<MsgNode>(msg_id, response_str.c_str(), response_str.length());
		session->Send(error_msg_node);
	}
	catch (const std::exception& e) {
		std::cerr << "Failed to send error response: " << e.what() << std::endl;
	}
}