#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <fstream>
#include "dir.h"

#pragma warning(disable: 4996)

#define CHUNK 8092

SOCKET connections[100];
int connectionIter = 0;

bool sendFile(int connIndex, std::string fileName) {
	long long int fileSize = 0;

	std::ifstream file_for_size(fileName, std::ifstream::ate | std::ios::binary);
	fileSize = file_for_size.tellg();
	file_for_size.close();

	//Send size
	send(connections[connIndex], (char*)&fileSize, sizeof(long long int), 0);

	//Send file
	std::ifstream file(fileName, std::ios::binary);

	if (!file.is_open()) {
		return 1;
	}

	char buffer[CHUNK];
	int read = 0;
	while (file.read((char*)buffer, CHUNK)) {
		send(connections[connIndex], buffer, sizeof(buffer), NULL);
		memset(buffer, 0x00, CHUNK);
		read += CHUNK;
	}
	send(connections[connIndex], buffer, fileSize - read, NULL);

	file.close();

	return 0;
}

bool sendDataArray(int connIndex, std::string *arr, int arraySize) {
	//Send array size
	send(connections[connIndex], (char*)&arraySize, sizeof(int), 0);
	//Send array
	for (int i = 0; i < arraySize; i++) {
		//std::cout << arr[i].c_str() << '\n';
		send(connections[connIndex], arr[i].c_str(), arr[i].length(), 0);
		Sleep(1);
	}

	return 0;
}

void connectionHandler(int connIndex) {
	char msg[CHUNK];

	//Send disks 
	ObjFS drives = getDir("", 1);
	sendDataArray(connIndex, drives.paths, drives.elements);

	int chouse = 0;
	bool firstTimeDiskUsage = 0;
	while (true) {
		int recv_res = recv(connections[connIndex], msg, CHUNK, 0);
		msg[recv_res] = 0x00;

		//Send folders or disks
		if (msg[0] == 'D' && msg[1] == 'R') {
			ObjFS drives = getDir(msg, 1);
			sendDataArray(connIndex, drives.paths, drives.elements);
		}
		else if (msg[0] == 'E' && msg[1] == 'X') {
			exit(0);
		}
		else{
			ObjFS folders = getDir(msg, 0);
			if (folders.type == 0) {
				std::string fileType[1] = {"F"};
				sendDataArray(connIndex, fileType, 1);
				std::cout << "Intent to send file!\n";
				if (sendFile(connIndex, msg)) {
					std::cout << "File has NOT been sent!\n";
				}
				else {
					std::cout << "File has been sent!\n";
				}
			}
			else {
				sendDataArray(connIndex, folders.paths, folders.elements);
			}
		}
		firstTimeDiskUsage = 1;
	}
}

int main(int argc, char* argv[]) {
	//WSAStartup
	WSAData wsaData;
	WORD DLLVersion = MAKEWORD(2, 1);
	if (WSAStartup(DLLVersion, &wsaData) != 0) {
		std::cout << "Error -> WSAStartup" << std::endl;
		exit(1);
	}

	SOCKADDR_IN addr;
	int sizeofaddr = sizeof(addr);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(1111);
	addr.sin_family = AF_INET;

	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
	listen(sListen, SOMAXCONN);

	std::cout << "Server started!\n";

	SOCKET newConnection;
	for (int i = 0; i < 100; i++) {
		newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);

		if (newConnection == -1) {
			std::cout << "Unsuccessful, accept()\n";
			std::cout << errno;
		}
		else {
			std::cout << "New connection!\n";
			connections[i] = newConnection;
			connectionIter++;
			CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)connectionHandler, (LPVOID)(i), NULL, NULL);
		}
	}
	
	return 0;
}