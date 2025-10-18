#include "LogicSystem.h"
using namespace std;

LogicSystem::LogicSystem()
	: _b_stop(false)
{
	RegisterCallBack();
	_worker_thread = std::thread(&LogicSystem::DealMsg, this);

}

void LogicSystem::RegisterCallBack() {
	_fun_callback[MSG_HELLO_WORD] = std::bind(&LogicSystem::HelloWordCallBack, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

void LogicSystem::HelloWordCallBack(std::shared_ptr<Session> session, const std::uint16_t msg_id, const std::unique_ptr<MsgNode>& msg_data) {
	Json::Reader reader;
	Json::Value root;
	reader.parse(msg_data->Data(), root);
	std::cout << "recv msg id is:" << root["id"].asInt() << ", data is:" << root["data"].asString() << std::endl;
	root["data"] = "server has recv msg , msg data is" + root["data"].asString();

	std::string return_str = root.toStyledString();
	session->Send(std::make_unique<MsgNode>(msg_id, return_str.c_str(), return_str.length()));
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
                // ��ʱת����shared_ptr ת unique_ptr
                std::unique_ptr<MsgNode> unique_msg = std::make_unique<MsgNode>(*logic_node->_recvnode);
                it->second(logic_node->_session, msg_id, unique_msg);
            }
        }
    }
}