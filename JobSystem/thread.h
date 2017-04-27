#pragma once

#include <limits>
#include <thread>
#include <string>

#include <Windows.h>
#undef max

class CThread
{
public:
	CThread(std::string& name)
		: m_name{name}
	{
	}

	template<typename functor>
	void Start(functor function, uint64_t affinityMask = std::numeric_limits<uint64_t>::max()) const
	{
		if (!m_thread.joinable())
		{
			if (SetThreadAffinityMask(m_thread.native_handle(), (DWORD_PTR)affinityMask) == 0)
			{
				std::cout << "[" << m_name << "] (const) starting..." << std::endl;
				m_thread = std::thread(function);
			}
			else
			{
				std::cout << "[" << m_name << "] unable to set affinity of [0x" << std::hex << affinityMask << "]" << std::endl;
			}
		}
		else
		{
			std::cout << "[" << m_name << "] already running..." << std::endl;
		}
	}

	template<typename functor>
	void Start(functor function, uint64_t affinityMask = std::numeric_limits<uint64_t>::max())
	{
		if (!m_thread.joinable())
		{
			if (SetThreadAffinityMask(m_thread.native_handle(), (DWORD_PTR)affinityMask) == 0)
			{
				std::cout << "[" << m_name << "] starting..." << std::endl;
				m_thread = std::thread(function);
			}
			else
			{
				std::cout << "[" << m_name << "] unable to set affinity of [0x" << std::hex << affinityMask << "]" << std::endl;
			}
		}
		else
		{
			std::cout << "[" << m_name << "] already running..." << std::endl;
		}
	}

	void Join()
	{
		if (m_thread.joinable())
		{
			m_thread.join();
		}
	}

	const char* GetName()
	{
		return m_name.c_str();
	}

	std::thread::id GetId()
	{
		return m_thread.get_id();
	}

protected:
private:
	std::string m_name;
	std::thread m_thread;
};
