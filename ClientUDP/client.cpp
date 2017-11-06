#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <fstream>

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
	bool sendName = false;
	// Переменная с индексом текущего пакета.
	int id = 1;
	while (!terminateThread_)
	{
		bool forDelete = false;
		char *buffer = nullptr;
		if (!fileName_.empty()) {

			const size_t sizeBuffer = 400;
			const size_t sizeSpecBuffer = 100;
			char specBuffer[sizeSpecBuffer];

			std::ifstream iFile;
			iFile.open(fileName_, std::ios::in | std::ios::binary);

			iFile.seekg(0, std::ios::end);
			size_t length = iFile.tellg();
			iFile.seekg(0, std::ios::beg);

			buffer = new char[sizeBuffer + 1];

			// Убеждаемся в доставке.
			auto l = [&] (int i) {
				// Убеждаемся в доставке.
				while (true) {
					sockaddr_in remote_addr;
					int addr_size = sizeof(remote_addr);
					int read = recvfrom(clientSocket, (char*)specBuffer, sizeSpecBuffer, 0, (sockaddr*)&remote_addr, &addr_size);
					if (read > 0) {
						int index = 0;
						std::string idStr = "";
						for (; index < read; ++index) {
							idStr += specBuffer[index];
						}
						if (id == std::stoi(idStr)) {
							id++;
							break;
						}
					}
					iResult = sendto(clientSocket, buffer, i, 0, server_addr->ai_addr, (int)server_addr->ai_addrlen);
				}
			};

			if (!sendName) {

				// Здесь добавляем в начало пакета индекс.
				std::string idStrM = std::to_string(id) + "*";
				std::strcpy(buffer, idStrM.c_str());

				int k = idStrM.size();
				int i = 0;
				for (; i < fileName_.size(); ++i) {
					buffer[i + k] = fileName_[i];
				}
				buffer[i + k] = '*';
				++i;
				std::string size = std::to_string(length);
				for (int j = 0; j < size.size(); ++j, ++i) {
					buffer[i + k] = size[j];
				}
				buffer[i + k] = '*';
				++i;

				iResult = sendto(clientSocket, buffer, i + k, 0, server_addr->ai_addr, (int)server_addr->ai_addrlen);

				// Убеждаемся в доставке.
				l(i + k);
				sendName = true;
			}

			int sent = 0;
			while (length > sizeBuffer - std::to_string(id).size() - 1) {
				// Здесь помещаем индекс в начало сообщения.
				std::string idStrM = std::to_string(id) + "*";
				std::strcpy(buffer, idStrM.c_str());
				int i = idStrM.size();

				iFile.read(&buffer[i], sizeBuffer - i);
				iResult = sendto(clientSocket, buffer, sizeBuffer, 0, server_addr->ai_addr, (int)server_addr->ai_addrlen);

				// Убеждаемся в доставке.
				l(sizeBuffer);

				if (iResult == SOCKET_ERROR)
				{
					printf("send from client failed with error: %d\n", WSAGetLastError());
					std::cout << sent;
					break;
				}
				sent += iResult;
				length = length - sizeBuffer + i;
			}
			if (length > 0) {
				// Здесь снова помещаем индекс в начало сообщения.
				std::string idStrM = std::to_string(id) + "*";
				std::strcpy(buffer, idStrM.c_str());
				int i = idStrM.size();

				iFile.read(&buffer[i], length);
				iResult = sendto(clientSocket, buffer, length + i, 0, server_addr->ai_addr, (int)server_addr->ai_addrlen);
				
				// Здесь убеждаемся в доставке.
				l(length + i);

				if (iResult == SOCKET_ERROR)
				{
					printf("send from client failed with error: %d\n", WSAGetLastError());
					break;
				}
				sent += iResult;
			}
			if (fileName_ != "") {
				std::cout << "File " << fileName_ << " was sent!\n";
				id = 1;
			}

			fileName_ = "";
			forDelete = true;
			sendName = false;
		}

		Sleep(1000);
		if (forDelete)
			delete[]buffer;
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