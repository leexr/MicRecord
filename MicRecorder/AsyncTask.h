#pragma once
#include <deque>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

class AsyncMessage {
	friend class AsyncTask;
private:
	bool ExitSingle;
};

class AsyncTask {
public:
	AsyncTask(bool Started = true);
	~AsyncTask() { Exit(); }
	bool Post_Message(std::shared_ptr<AsyncMessage> message);
	bool Exit();
	bool Start();
protected:
	virtual void Process(const AsyncMessage &message) = 0;
private:
	std::deque<std::shared_ptr<AsyncMessage>> queue;
	std::mutex lock_queue;
	std::condition_variable condition;
	std::thread Worker;
	std::atomic<bool> status_lock;
	volatile int status;
private:
	void WokerProc();
};

