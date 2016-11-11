#include "stdafx.h"
#include "AsyncTask.h"
#include <Windows.h>

#define USING 0
#define FREE 1

#define RUNNING 0
#define STOP 1

class Spin_lock {
public:
    Spin_lock(long &stauts):stauts(stauts){
		while (USING == InterlockedCompareExchange(&stauts, USING, FREE)) {}
    }
    ~Spin_lock() {
		InterlockedExchange(&stauts, FREE);
    }
private:
    long &stauts;
};

AsyncTask::AsyncTask(bool Started) 
:status_lock(FREE),
status(STOP)
{
	stuatus_event = CreateEventW(NULL, false, false, NULL);
	if (Started) {
		Start();
	}
}

void AsyncTask::WokerProc() {
	while (true) {
		std::shared_ptr<AsyncMessage> message;
		{
			std::unique_lock<std::mutex> lock(lock_queue);
			condition.wait(lock, [this] {
				return 0 == queue.empty();
			});
			if (queue.empty()) { continue; }
			message = *queue.begin();
			queue.pop_front();
		}
		if (message->ExitSingle) {
			return;
		}
		Process(*message);
	}
}

bool AsyncTask::Post_Message(std::shared_ptr<AsyncMessage> message) {
	std::unique_lock<std::mutex> lock(lock_queue);
	queue.push_back(message);
	condition.notify_all();
	return true;
}

bool AsyncTask::Exit() {
    Spin_lock l(status_lock);
	if (RUNNING == status) {
		std::shared_ptr<AsyncMessage> message(new AsyncMessage());
		message->ExitSingle = true;
		{
			std::unique_lock<std::mutex> lock(lock_queue);
			queue.push_back(message);
			condition.notify_all();
		}
		Worker.join();
		status = STOP;
		SetEvent(stuatus_event);
		return true;
	}
	return false;
}

bool AsyncTask::Start() {
    Spin_lock l(status_lock);
	if (RUNNING == status) {
		status = RUNNING;
		return false;
	}
	queue.clear();
	Worker = std::thread([this] {WokerProc(); });
	ResetEvent(stuatus_event);
	status = RUNNING;
	return true;
}

void AsyncTask::WaitForExit() {
	WaitForSingleObject(stuatus_event, INFINITE);
}