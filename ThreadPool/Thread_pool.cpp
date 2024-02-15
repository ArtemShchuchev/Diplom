#include "Thread_pool.h"

Thread_pool::Thread_pool(const unsigned numThr) : _timeout(std::chrono::seconds(2))
{
	pool.resize(numThr);	// устанавливаю размер вектора
	status.assign(numThr, { thread_mode::free, {} });
	
	// выборка задач из очереди, происходит в потоках
	for (unsigned i(0); i < numThr; ++i) {
		pool[i] = std::thread([this, i] { work(i); });
		pool[i].detach();
	}
}

Thread_pool::~Thread_pool()
{
	//wait();	// ожидание завершения работы потоками
}

// выбирает из очереди очередную задачу и выполняет ее
void Thread_pool::work(const unsigned idx)
{
	while (true)
	{
		const task_t task = squeue.pop();
		
		status[idx].start = std::chrono::steady_clock::now();
		status[idx].mode = thread_mode::busy;
		task();
		status[idx].mode = thread_mode::free;
	}
}

// помещает в очередь очередную задачу
void Thread_pool::add(const task_t& task)
{
	if (task)
		squeue.push(task);
}

void Thread_pool::setTimeout(const std::chrono::seconds timeout)
{
	_timeout = timeout;
}

// возвращает true если в очереди или
// хоть в одном работающем потоке есть задачи
bool Thread_pool::isBusy()
{
	int hangsCount(0);	// счетчик зависших потоков
	for (auto& [mode, start] : status) {
		if (mode == thread_mode::busy) {
			auto diff = std::chrono::steady_clock::now() - start;
			if (diff < _timeout) {
				// поток работает и его время
				// не превысило таймаут -> true
				return true;
			}
			// поток висит
			++hangsCount;
		}
		// хоть 1 поток свободен
	}
	// если очередь не пуста при хоть 1ом свободном потоке -> true
	// если все потоки висят -> false
	// если очередь пуста, а потоки висят или свободны -> false
	const bool queueempty(squeue.empty());
	const bool hangs_f(hangsCount == status.size());

	return (queueempty || hangs_f) ? false : true;
}

// ждет пока все потоки освободятся или все потоки timeout
void Thread_pool::wait()
{
	while (isBusy()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}
}
