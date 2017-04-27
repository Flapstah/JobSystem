#pragma once

// job system stuff
//#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <limits>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

// temporary for debugging
#include <iostream>

#include <Windows.h>
#undef max

#define THREAD_ID "[" << std::this_thread::get_id() << "] "

class CJobSystem
{
public:
	enum EState : char
	{
		S_IDLE,
		S_RUNNING,
		S_SHUTTING_DOWN,
	};

	CJobSystem()
		: m_numThreads(std::thread::hardware_concurrency() - 1)
	{
		std::cout << THREAD_ID << "CJobSystem constructed : m_numThreads = " << m_numThreads << std::endl;
		CreateWorkerThreads();
	}

	CJobSystem(size_t numThreads)
		: m_numThreads(numThreads)
	{
		std::cout << THREAD_ID << "CJobSystem constructed : m_numThreads = " << m_numThreads << std::endl;
		CreateWorkerThreads();
	}

	~CJobSystem()
	{
		Shutdown();
	}

	void Update()
	{
		// Main update loop to be run on the main thread
		// Needs to service callbacks on the main thread
		std::cout << THREAD_ID << "CJobSystem::Update()" << std::endl;
	}

	void Shutdown()
	{
		std::cout << THREAD_ID << "CJobSystem::Shutdown()" << std::endl;
		for (CWorkerThread*& workerThread : m_workerThreads)
		{
			if (workerThread != nullptr)
			{
				delete workerThread;
				workerThread = nullptr;
			}
		}
	}

private:
	void CreateWorkerThreads()
	{
		m_workerThreads.resize(m_numThreads);
		for (size_t index = 0; index < m_numThreads; ++index)
		{
			m_workerThreads[index] = new CWorkerThread();
			m_workerThreads[index]->SetAffinityByProcessor(index);
			m_workerThreads[index]->RequestStart(&m_jobQueue);
			std::cout << THREAD_ID << "CJobSytem::CreateWorkerThreads() created [" << m_workerThreads[index]->id() << "]" << std::endl;
		}
	}

	class CJobQueue
	{
	public:
		size_t size()
		{
			return m_queue.size();
		}

		void push(std::function<void()>&& function, uint64_t&& affinityMask = std::numeric_limits<uint64_t>::max())
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			m_queue.push_back(SJobInfo(std::forward<std::function<void()>>(function), std::forward<uint64_t>(affinityMask)));
		}

		std::function<void()>&& pop()
		{
			static std::function<void()> none = nullptr;
			std::lock_guard<std::mutex> lock(m_mutex);
			if (!m_queue.empty())
			{
				std::function<void()>&& outFunction = std::move(m_queue.front().m_function);
				m_queue.pop_front();
				return std::move(outFunction);
			}
			return std::move(none);
		}

	private:
		struct SJobInfo
		{
			SJobInfo(std::function<void()>&& function, uint64_t&& affinityMask)
				: m_affinityMask(affinityMask)
				, m_function(function)
			{
			}

			uint64_t m_affinityMask;
			std::function<void()> m_function;
		};

		std::mutex m_mutex;
		std::deque<SJobInfo> m_queue;
	};

	class CWorkerThread
	{
	public:
		enum EState : char
		{
			S_IDLE,
			S_RUNNING,
			S_TERMINATING,
			S_TERMINATE,
		};

		CWorkerThread()
			: m_state(S_IDLE)
		{
		}

		~CWorkerThread()
		{
			RequestTerminate();
		}

		inline std::thread::id id()
		{
			return m_thread.get_id();
		}

		void RequestStart(CJobQueue* queue)
		{
			m_queue = queue;
			SetState(S_RUNNING);
			m_thread = std::thread([this]() { this->Main(); });
		}

		void RequestTerminate()
		{
			std::cout << THREAD_ID << "CWorkerThread::RequestTerminate() [" << m_thread.get_id() << "]" << std::endl;
			if (GetState() == S_RUNNING)
			{
				SetState(S_TERMINATE);
				if (m_thread.joinable())
				{
					m_thread.join();
				}
			}
		}

		bool SetAffinityByProcessor(uint64_t processorIndex)
		{
			uint64_t affinityMask = 1;
			bool success = false;
			if (processorIndex < std::thread::hardware_concurrency())
			{
				affinityMask <<= processorIndex;
				DWORD_PTR rc = SetThreadAffinityMask(m_thread.native_handle(), (DWORD_PTR)affinityMask);
				std::cout << THREAD_ID << "CWorkerThread::SetAffinityByProcessor() [" << std::hex << affinityMask << "] [" << ((rc != 0) ? "true" : "false") << "]" << std::endl;
				success = (rc != 0);
			}

			return success;
		}

	private:
		void SetState(EState state)
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			std::cout << THREAD_ID << "CWorkerThread::SetState() [" << m_thread.get_id() << "] " << GetStateString(m_state) << " -> " << GetStateString(state) << std::endl;
			m_state = state;
		}

#define CASE_ENUM_TO_STRING(_enum) case _enum : return #_enum
		inline const char* GetStateString(EState state)
		{
			switch (state)
			{
				CASE_ENUM_TO_STRING(S_IDLE);
				CASE_ENUM_TO_STRING(S_RUNNING);
				CASE_ENUM_TO_STRING(S_TERMINATING);
				CASE_ENUM_TO_STRING(S_TERMINATE);
			default:
				return "<UNKNOWN>";
			}
		}
		inline const EState GetState()
		{
			return m_state;
		}

		void Main()
		{
			std::cout << THREAD_ID << "CWorkerThread::Main() starting" << std::endl;

			while (GetState() == S_RUNNING)
			{
				std::function<void()>&& function = m_queue->pop();
				if (function != nullptr)
				{
					std::cout << THREAD_ID  << "CWorkerThread::Main() executing function" << std::endl;
					function();
				}
				else
				{
					std::cout << THREAD_ID  << "CWorkerThread::Main() waiting" << std::endl;
					//std::this_thread::yield();
					std::this_thread::sleep_for(std::chrono::microseconds(100));
				}
			}

			SetState(S_TERMINATING);
			std::cout << THREAD_ID << "CWorkerThread::Main() shutting down with " << m_queue->size() << " outstanding jobs in queue" << std::endl;

			SetState(S_IDLE);
		}

		EState m_state;
		std::mutex m_mutex;
		std::thread m_thread;
		CJobQueue* m_queue;
	};

	size_t m_numThreads;
	CJobQueue m_jobQueue;
	CJobQueue m_callbackQueue;
	std::vector<CWorkerThread*> m_workerThreads;
};


