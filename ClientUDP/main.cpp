#include <iostream>
#include <string>

#include "client.h"

#pragma comment (lib, "Ws2_32.lib")

int main()
{
	std::string command;

	Client secondClient;
	secondClient.start();

	while (true)
	{
		std::cout << "\n>";
		getline(std::cin, command);
		if (command == "quit")
			break;
		else
			std::cout << "Invalid command";
	}

	secondClient.stop();
}