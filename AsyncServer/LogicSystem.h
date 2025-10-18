// LogicSystem.h
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

// 修改为使用 shared_ptr
typedef function<void(std::shared_ptr<Session>, const std::uint16_t msg_id, const std::shared_ptr<MsgNode>& msg_data)> FunCallBack;

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
	// 修改为使用 shared_ptr
	void HelloWordCallBack(std::shared_ptr<Session>, const std::uint16_t msg_id, const std::shared_ptr<MsgNode>& msg_data);
	void DealMsg();
	void SendErrorResponse(std::shared_ptr<Session> session, uint16_t msg_id, const std::string& error_msg);
	std::queue<std::shared_ptr<LogicNode>> _msg_queue;
	std::mutex _mutex;
	std::condition_variable cv;
	std::thread _worker_thread;
	bool _b_stop;
	std::map<std::uint16_t, FunCallBack> _fun_callback;
};