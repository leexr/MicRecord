#pragma once
#include <deque>
#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>

class AsyncMessage {
public:
	friend class AsyncTask;
    AsyncMessage() : ExitSingle(false) {}
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
    void WaitForExit();
protected:
    virtual void Process(const AsyncMessage &message) {};
private:
	std::deque<std::shared_ptr<AsyncMessage>> queue;
	std::mutex lock_queue;
	std::condition_variable condition;
	std::thread Worker;
	long status_lock;
	volatile int status;
    std::condition_variable exitSingle;
	HANDLE stuatus_event;
private:
	void WokerProc();
};

