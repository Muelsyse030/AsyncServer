#pragma once
#include "Singleton.h"
#include <queue>
#include <thread>
#include "Session.h"
#include <map>
#include <functional>
#include "const.h"
#include <Json/json.h>
#include <Json/value.h>
#include <Json/reader.h>
typedef function<void(std::shared_ptr<Session>, const std::uint16_t msg_id, const std::unique_ptr<MsgNode>& msg_data)> FunCallBack;
class LogicSystem : public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
private:
	LogicSystem();
public:
	~LogicSystem();
	void PostMsgToQueue(shared_ptr<LogicNode> msg);
private:
	void RegisterCallBack();
	void HelloWordCallBack(std::shared_ptr<Session>, const std::uint16_t msg_id, const std::unique_ptr<MsgNode>& msg_data);
	void DealMsg();

	std::queue<std::shared_ptr<LogicNode>> _msg_queue;
	std::mutex _mutex;
	std::condition_variable cv;
	std::thread _worker_thread;
	bool _b_stop;
	std::map<std::uint16_t , FunCallBack> _fun_callback;
};


