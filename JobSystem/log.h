#pragma once

#include <iostream>
#include <fstream>
#include <string>

class CLog
{
public:
	enum eSeverity
	{
		eS_FATAL,
		eS_ERROR,
		eS_WARNING,
		eS_DEBUG,
		eS_INFORMATION,
		eS_VERBOSE,
	};

	CLog(eSeverity level = eS_INFORMATION, const char* name = nullptr)
		: m_name((name != nullptr) ? name : "anonymous")
		, m_level(level)
		, m_file((name != nullptr) ? new std::ofstream(name, std::ios_base::trunc | std::ios_base::out) : nullptr)
	{
	}
	~CLog()
	{
		if (m_file != nullptr)
		{
			delete m_file;
		}
	}

	eSeverity SetLogLevel(eSeverity level)
	{
		m_level = level;
	}

	template<typename ...Ts>
	CLog& write(eSeverity level, const char* file, unsigned long line, const char* format, Ts... args)
	{
		if (level <= m_level)
		{
			auto& out = (level > eS_ERROR) ? std::cout : std::cerr;
			std::string buffer = format_string("%s(%d) [%s] %s\n", file, line, level_to_string(level), format);
			if (sizeof...(args) > 0)
			{
				buffer = format_string(buffer.c_str(), args...);
			}
			out << buffer.c_str();
			if (m_file != nullptr)
				*m_file << buffer.c_str();
		}

		return *this;
	}

protected:
	template <typename ...Ts>
	std::string format_string(const char* format, Ts... args)
	{
		auto size = snprintf(nullptr, 0, format, args...) + 1; // +1 for the NULL
		std::unique_ptr<char[]> buffer(new char[size]);
		snprintf(buffer.get(), size, format, args...);
		return std::string(buffer.get(), buffer.get() + size - 1); // -1 as we don't store the NULL
	}

	const char* level_to_string(eSeverity level)
	{
		const char* ret = nullptr;
		switch (level)
		{
		case CLog::eS_FATAL:
			ret = "FATAL";
			break;
		case CLog::eS_ERROR:
			ret = "ERROR";
			break;
		case CLog::eS_WARNING:
			ret = "WARNING";
			break;
		case CLog::eS_INFORMATION:
			ret = "INFO";
			break;
		case CLog::eS_VERBOSE:
			ret = "VERBOSE";
			break;
		case CLog::eS_DEBUG:
			ret = "DEBUG";
			break;
		default:
			ret = "???";
			break;
		}
		return ret;
	}

private:
	const std::string m_name;
	eSeverity m_level;
	std::ofstream* m_file;
};

/*
class CWLog
{
public:
	enum eSeverity
	{
		eS_FATAL,
		eS_ERROR,
		eS_WARNING,
		eS_INFORMATION,
		eS_VERBOSE,
		eS_DEBUG
	};

	CWLog(const wchar_t* name = nullptr);
	~CWLog();

	eSeverity SetLogLevel(eSeverity level)
	{
		m_level = level;
	}

	template<typename ...Ts>
	CWLog& write(eSeverity level, std::wstring&& file, unsigned long line, const std::wstring&& format, Ts... args);

protected:
	template <typename ...Ts>
	std::wstring&& format_string(const std::wstring&& format, Ts... args)
	{
		auto size = swprintf(nullptr, 0, format.c_str(), args...) + 1; // +1 for the NULL
		std::unique_ptr<wchar_t[]> buffer(new wchar_t[size]);
		swprintf(buffer.get(), size, format.c_str(), Ts... args);
		return std::wstring(buffer.get(), buffer.get() + size - 1); // -1 as we don't store the NULL
	}

private:
	const std::wstring m_name;
	eSeverity m_level;
	std::unique_ptr<std::wofstream> m_file;
};
*/