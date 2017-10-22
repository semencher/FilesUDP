#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "client.h"

Client::Client()
{
	threadH_ = nullptr;
}

void Client::start()
{
	terminateThread_ = false;
	threadH_ = CreateThread(nullptr, 0, &threadStatic, this, 0, &threadDW_);
}

void Client::stop()
{
	terminateThread_ = true;
	if (threadH_)
	{
		WaitForSingleObject(threadH_, INFINITE);
		CloseHandle(threadH_);
		threadH_ = nullptr;
	}
}

DWORD WINAPI Client::threadStatic(void *param)
{
	Client *client = (Client*)param;
	client->thread();
	return 0;
}

#define SERVER_PORT "21346"
#define SERVER_ADDR "localhost"

void Client::thread()
{
	std::cout << "Client has been started.\n";
	struct addrinfo *server_addr = nullptr;
	struct sockaddr_in client_addr;
	struct addrinfo hints;
	SOCKET clientSocket = INVALID_SOCKET;
	int iResult;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	WSADATA wsaData;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return;
	}

	iResult = getaddrinfo("localhost", SERVER_PORT, &hints, &server_addr);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		return;
	}

	client_addr = *(sockaddr_in*)server_addr->ai_addr;
	client_addr.sin_port = 0;
	client_addr.sin_addr.S_un.S_addr = 0;

	clientSocket = socket(server_addr->ai_family, server_addr->ai_socktype, server_addr->ai_protocol);
	if (clientSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(server_addr);
		return;
	}

	iResult = bind(clientSocket, (sockaddr*)&client_addr, sizeof(client_addr));
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(server_addr);
		closesocket(clientSocket);
		return;
	}
	unsigned int max_dgram_size = 0;
	int uint_size = sizeof(max_dgram_size);
	iResult = getsockopt(clientSocket, SOL_SOCKET, SO_MAX_MSG_SIZE, (char*)&max_dgram_size, &uint_size);
	if (iResult == SOCKET_ERROR)
	{
		printf("getsockopt failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(server_addr);
		closesocket(clientSocket);
		return;
	}
	while (!terminateThread_)
	{
		char *buffer = "Hello from client by UDP!\n";
		int buf_size = (int)strlen(buffer);
		iResult = sendto(clientSocket, buffer, buf_size, 0, server_addr->ai_addr, (int)server_addr->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			printf("send from client by UDP failed with error: %d\n", WSAGetLastError());
			break;
		}
		Sleep(1000);
	}
	freeaddrinfo(server_addr);
	iResult = shutdown(clientSocket, SD_SEND);
	if (iResult == SOCKET_ERROR)
	{
		printf("shutdown failed with error: %d\n", WSAGetLastError());
	}
	closesocket(clientSocket);

	std::cout << "Client has been stopped.\n";
	WSACleanup();
}