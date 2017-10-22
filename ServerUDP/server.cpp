#include <string>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "server.h"

Server::Server()
{
	threadH_ = nullptr;
}

void Server::run()
{
	// Добавил, так как без этого 10093.
	WSADATA wsaData;
	int iResult;
	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		printf("WSAStartup failed with error: %d\n", iResult);
		return;
	}

	std::string command;
	startListen();

	while (true)
	{
		std::cout << "\n>";
		getline(std::cin, command);
		if (command == "quit")
			break;
		else
			std::cout << "Invalid command";
	}

	std::cout << "Shutting down the server...\n";
	stopListen();
	WSACleanup();
	system("pause");
}

void Server::startListen()
{
	terminateThread_ = false;
	threadH_ = CreateThread(nullptr, 0, &listenThreadStatic, this, 0, &threadDW_);
}

void Server::stopListen()
{
	terminateThread_ = true;
	if (threadH_)
	{
		WaitForSingleObject(threadH_, INFINITE);
		CloseHandle(threadH_);
		threadH_ = nullptr;
	}
}

DWORD WINAPI Server::listenThreadStatic(void *param)
{
	Server *server = (Server*)param;
	server->listenThread();
	return 0;
}

#define SERVER_PORT "21346"

void Server::listenThread()
{
	struct addrinfo *result = nullptr;
	struct addrinfo hints;
	SOCKET listenSocket = INVALID_SOCKET;
	int iResult;
	fd_set readfds;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(nullptr, SERVER_PORT, &hints, &result);
	if (iResult != 0)
	{
		printf("getaddrinfo failed with error: %d\n", iResult);
		return;
	}

	// Create a SOCKET for connecting to server
	listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		return;
	}

	// Setup the TCP listening socket
	iResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(listenSocket);
		return;
	}

	freeaddrinfo(result);

	unsigned int max_dgram_size = 0;
	int uint_size = sizeof(max_dgram_size);
	iResult = getsockopt(listenSocket, SOL_SOCKET, SO_MAX_MSG_SIZE, (char*)&max_dgram_size, &uint_size);
	if (iResult == SOCKET_ERROR)
	{
		printf("getsockopt failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		return;
	}
	//create a buffer to receive UDP data
	char *dgram_buffer = (char*)malloc(max_dgram_size);

	FD_ZERO(&readfds);
	struct timeval tv;
	tv.tv_sec = 1;//1 second
	tv.tv_usec = 0;
	while (!terminateThread_)
	{
		FD_SET(listenSocket, &readfds);
		int rv = select(1, &readfds, nullptr, nullptr, &tv);
		if (rv == -1)
		{
			perror("Select: some kind of an error"); // error occurred in select()
		}
		else
			if (rv == 0)
			{
				//printf("Timeout occurred! No data after 1 second.\n");
			}
			else
			{
				if (FD_ISSET(listenSocket, &readfds))
				{
					sockaddr_in remote_addr;
					int addr_size = sizeof(remote_addr);
					int read = recvfrom(listenSocket, (char*)dgram_buffer, max_dgram_size, 0, (sockaddr*)&remote_addr, &addr_size);
					if (read > 0)
					{//we just received UDP packet
						dgram_buffer[read] = 0;
						printf("From %d.%d.%d.%d:%d - %s", remote_addr.sin_addr.S_un.S_un_b.s_b1, remote_addr.sin_addr.S_un.S_un_b.s_b2,
							remote_addr.sin_addr.S_un.S_un_b.s_b3, remote_addr.sin_addr.S_un.S_un_b.s_b4, ntohs(remote_addr.sin_port),
							dgram_buffer);
					}
				}
			}
	}

	// No longer need server socket
	closesocket(listenSocket);

	free(dgram_buffer);
}