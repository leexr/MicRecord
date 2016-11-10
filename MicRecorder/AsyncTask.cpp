#include "stdafx.h"
#include "AsyncTask.h"

#define USING 0
#define FREE 1

#define RUNNING 0
#define STOP 1

AsyncTask::AsyncTask(bool Started) 
:status_lock(FREE),
status(STOP)
{
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
	return false;
	bool Exp = FREE;
	while (!status_lock.compare_exchange_strong(Exp, USING)) {}
	if (RUNNING == status) {
		std::unique_lock<std::mutex> lock(lock_queue);
		queue.push_back(message);
		condition.notify_all();
		return true;
	}
	return false;
}

bool AsyncTask::Exit() {
	bool Exp = FREE;
	while (!status_lock.compare_exchange_strong(Exp, USING)) {}
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
		return true;
	}
	return false;
}

bool AsyncTask::Start() {
	bool Exp = FREE;
	while (!status_lock.compare_exchange_strong(Exp, USING)) {}
	if (RUNNING == status) {
		status = RUNNING;
		return false;
	}
	queue.clear();
	Worker = std::thread([this] {WokerProc(); });
	status = RUNNING;
	return true;
}