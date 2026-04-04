#include "umepch.h"
#include "Ume/Core/TaskSystem.h"

namespace Ume {

TaskSystem& TaskSystem::Get()
{
	static TaskSystem instance;
	return instance;
}

TaskSystem::TaskSystem()
{
	const uint32_t hardwareThreads = std::max(1u, std::thread::hardware_concurrency());
	const uint32_t workerCount = std::clamp(hardwareThreads > 1 ? hardwareThreads - 1 : 1u, 1u, 4u);

	m_Workers.reserve(workerCount);
	for (uint32_t i = 0; i < workerCount; i++)
		m_Workers.emplace_back(&TaskSystem::WorkerLoop, this);
}

TaskSystem::~TaskSystem()
{
	Shutdown();
}

void TaskSystem::Shutdown()
{
	{
		std::lock_guard<std::mutex> lock(m_QueueMutex);
		if (m_Stopping)
			return;
		m_Stopping = true;
	}

	m_WakeCondition.notify_all();

	for (std::thread& worker : m_Workers)
	{
		if (worker.joinable())
			worker.join();
	}

	m_Workers.clear();
}

void TaskSystem::WorkerLoop()
{
	while (true)
	{
		std::function<void()> task;
		{
			std::unique_lock<std::mutex> lock(m_QueueMutex);
			m_WakeCondition.wait(lock, [this]() { return m_Stopping || !m_Tasks.empty(); });

			if (m_Stopping && m_Tasks.empty())
				return;

			task = std::move(m_Tasks.front());
			m_Tasks.pop();
		}

		task();
	}
}

}
