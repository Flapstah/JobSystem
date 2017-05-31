// JobSystem.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "log.h"
CLog g_log(CLog::eS_DEBUG, "output.log");

#include "JobSystem.h"

int main(int argc, char *argv[])
{
	LOG_INFORMATION("Start");

	CJobSystem js;
	js.Update();

	LOG_INFORMATION("End");
	return 0;
}