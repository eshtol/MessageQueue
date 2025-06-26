#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "ConcurrentContainers.h"


template <typename TaskType, template <typename> typename PtrT> class TaskQueueThread
{
	public:
		typedef PtrT<TaskType> TaskPtr;
		TaskQueueThread() { std::thread(ThreadLoop).detach(); }
		~TaskQueueThread() = default;  // NO

		inline bool IsFree() const { return !current_task; }

		void AcceptTask(const TaskPtr&& task)
		{
			m_queue.emplace(task);
			have_task.notify_one();
		}

	private:
		std::mutex task_mtx;
		std::condition_variable have_task;
		concurrent_queue<TaskPtr> m_queue;
		TaskPtr current_task;

		std::function<void()> ThreadLoop = [&]()
		{
			std::unique_lock<decltype(task_mtx)> task_lock(task_mtx);
			const auto not_empty = [this]() { return !std::empty(m_queue); };
			
			while (true)
				have_task.wait(task_lock, not_empty),
				(current_task = m_queue.extract_first())->execute(),
				current_task.reset();
		};
};