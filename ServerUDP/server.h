#pragma once

#include <Windows.h>

class Server
{
public:
	Server();
	void run();

private:
	void startListen();
	void stopListen();

	static DWORD WINAPI listenThreadStatic(void *param);
	void listenThread();

private:
	bool terminateThread_;
	HANDLE threadH_;
	DWORD threadDW_;
};	// class Server