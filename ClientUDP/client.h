#pragma once

#include <Windows.h>

class Client
{
public:
	Client();
	void start();
	void stop();

private:
	static DWORD WINAPI threadStatic(void *param);
	void thread();

private:
	bool terminateThread_;
	HANDLE threadH_;
	DWORD threadDW_;

};	// class Client