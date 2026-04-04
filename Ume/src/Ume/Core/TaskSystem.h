#pragma once

#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <type_traits>
#include <vector>

namespace Ume {

class TaskSystem
{
public:
	TaskSystem(const TaskSystem&) = delete;
	TaskSystem& operator=(const TaskSystem&) = delete;

	static TaskSystem& Get();

	template<typename Function>
	auto Submit(Function&& function) -> std::future<std::invoke_result_t<Function>>
	{
		using ResultType = std::invoke_result_t<Function>;
		auto task = std::make_shared<std::packaged_task<ResultType()>>(std::forward<Function>(function));
		std::future<ResultType> future = task->get_future();

		{
			std::lock_guard<std::mutex> lock(m_QueueMutex);
			if (m_Stopping)
				return future;

			m_Tasks.emplace([task]() { (*task)(); });
		}

		m_WakeCondition.notify_one();
		return future;
	}

	void Shutdown();

private:
	TaskSystem();
	~TaskSystem();

	void WorkerLoop();

private:
	std::vector<std::thread> m_Workers;
	std::queue<std::function<void()>> m_Tasks;
	std::mutex m_QueueMutex;
	std::condition_variable m_WakeCondition;
	bool m_Stopping = false;
};

}
