#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>

template<class Task>
class Safe_queue
{
private:
	std::mutex taskLock;		// мьютекс для блокировки очереди задач
	std::condition_variable taskCondition; // для уведомлений
	std::queue<Task> taskList;	// очередь задач

public:
	// записывает в начало очереди новую задачу, при этом
	// захватывает мьютекс, а по окончании операции
	// нотифицируется условная переменная
	void push(const Task& task)
	{
		{
			std::lock_guard<std::mutex> lock(taskLock);
			taskList.push(task);
		}
		taskCondition.notify_one();
	}
	// находится в ожидании пока не придут уведомления
	// на условную переменную. При нотификации условной
	// переменной. данные считываются из очереди
	Task pop()
	{
		std::unique_lock<std::mutex> lock(taskLock);
		taskCondition.wait(lock, [this] { return !taskList.empty(); });

		Task task = taskList.front();
		taskList.pop();
		return task;
	}
	// возвращает true если очередь пуста
	bool empty()
	{
		std::lock_guard<std::mutex> lock(taskLock);
		return taskList.empty();
	}
};
