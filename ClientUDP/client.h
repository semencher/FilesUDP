#pragma once

#include <string>
#include <Windows.h>

class Client
{
public:
	Client();
	void start();
	void stop();

	void setFileName(const std::string &fileName) { fileName_ = fileName; }
	std::string fileName() { return fileName_; }

private:
	static DWORD WINAPI threadStatic(void *param);
	void thread();

private:
	bool terminateThread_;
	HANDLE threadH_;
	DWORD threadDW_;

	std::string fileName_;

};	// class Client