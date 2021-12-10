#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <iostream>
#include <string>
#include <fstream>

#pragma warning(disable: 4996)

#define CHUNK 8092
#define DFOLDER "P:\\Download\\"

SOCKET connection;

struct Data {
	float size = 0;
	std::string unit = "";

	Data(float size, std::string unit) : size(size), unit(unit) {};
};

Data smartConvert(long long int bytes) {
	if (bytes >= 1099511627776) {
		return Data((float)bytes / (1024.f * 1024.f * 1024.f * 1024.f), "TB");
	}
	else if (bytes >= 1024 * 1024 * 1024) {
		return Data((float)bytes / (1024.f * 1024.f * 1024.f), "GB");
	}
	else if (bytes >= 1024 * 1024) {
		return Data((float)bytes / (1024.f * 1024.f), "MB");
	}
	else if (bytes >= 1024) {
		return Data((float)bytes / 1024.f, "KB");
	}
	else if (bytes < 1024) {
		return Data(bytes, "BYTES");
	}
}

bool receiveFile(std::string fileName) {
	//Receive file size
	long long int fileSize = 0;
	while (true) {
		int recv_res = recv(connection, (char*)&fileSize, sizeof(long long int), MSG_WAITALL);
		if (fileSize == -1) {
			std::cout << "File can NOT be received!";
			return 1;
		}
		if (recv_res == 0) {
			std::cout << "Server disconnected!\n";
			return 1;
		}
		if (recv_res < 0) {
			continue;
		}
		else {
			break;
		}
	}

	Data fileSizeInUnits = smartConvert(fileSize);
	std::cout << "To receive: " << fileSizeInUnits.size << ' ' << fileSizeInUnits.unit << '\n';

	int lastFragment = 0;

	//Receive file
	std::cout << "Receiving: ";

	int received = 0;
	std::ofstream file(fileName, std::ios::binary);

	char buffer[CHUNK + 1];
	while (true) {
		int recv_res = recv(connection, buffer, CHUNK, NULL);
		if (recv_res == 0) {
			std::cout << "Server disconnected!\n";
			file.close();
			return 1;
		}
		if (recv_res < 0) {
			continue;
		}

		buffer[recv_res] = 0x00;

		if (file.is_open()) {
			file.write(buffer, recv_res);
		}

		received += recv_res;

		//Percentage (download is divided in 10 pieces)
		int fragment = (float)received / (float)fileSize * 10.f;
		if (lastFragment < fragment) {
			for (int i = lastFragment; fragment > i; i++) {
				std::cout << char(219);
			}
			lastFragment = fragment;
		}

		if (received >= fileSize) {
			std::cout << "\nReceived: " << received << " BYTES\n";
			file.close();
			return 0;
		}
	}

	return 0;
}

bool recvDataArray(std::string *arr, int &arraySize) {
	//Recv array size
	while (true) {
		int recv_res = recv(connection, (char*)&arraySize, sizeof(int), 0);
		if (arraySize == -1) {
			std::cout << "Array is empty!";
			return 1;
		}
		if (recv_res == 0) {
			std::cout << "Server disconnected!\n";
			return 1;
		}
		if (recv_res < 0) {
			continue;
		}
		else {
			break;
		}
	}

	//std::cout << "Size " << arraySize << '\n';

	//Recv array
	int received = 0;

	char buffer[CHUNK + 1];
	while (true) {
		int recv_res = recv(connection, buffer, CHUNK, NULL);
		if (recv_res == -1) {
			std::cout << "Server disconnected!\n";
			return 1;
		}
		if (recv_res < 0) {
			continue;
		}

		buffer[recv_res] = 0x00;
		
		arr[received] = buffer;
		received++;

		if (received >= arraySize) {
			return 0;
		}
	}

	return 0;
}

std::string arr[1000];
std::string lastFolder = "";
void connectionHandler() {
	while (true) {
		int arraySize = 0;
		recvDataArray(arr, arraySize);

		if (arraySize <= 0) {
			continue;
		}

		if (arr[0] == "F") {
			int index = lastFolder.find_last_of('\\');
			std::string fileName = lastFolder.substr(index, lastFolder.size() - index);
			//Sleep to wait main thread to clear screen
			Sleep(100);
			receiveFile(DFOLDER + fileName);
		}
		else {
			for (int i = 0; i < arraySize; i++) {
				std::cout << i + 1 << ". ";
				std::cout << arr[i] << '\n';
			}
		}
	}
}

bool isNumber(const std::string& s)
{
	std::string::const_iterator it = s.begin();
	while (it != s.end() && std::isdigit(*it)) ++it;
	return !s.empty() && it == s.end();
}

int main(int argc, char* argv[]) {
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

	connection = socket(AF_INET, SOCK_STREAM, NULL);
	if (connect(connection, (SOCKADDR*)&addr, sizeof(addr)) != 0) {
		std::cout << "Error -> failed connect to server\n";
		return 1;
	}
	std::cout << "Connected!\n";

	std::cout << "x -> closes client and server\nq -> closes only client\nb -> moves directory level up\n";

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)connectionHandler, NULL, NULL, NULL);

	while (true) {
		std::string chouse = "";
		std::cin >> chouse;

		if (arr[0] != "") {
			if (isNumber(chouse)) { //Command to open folder or send file
				int chouseInt = std::stoi(chouse);
				if (arr[chouseInt - 1] != "") {
					lastFolder = arr[chouseInt - 1];
					send(connection, arr[chouseInt - 1].c_str(), arr[chouseInt - 1].length(), 0);
					std::cout << "\n\n";
					continue;
				}
			}
			else { //Command handler
				if (chouse == "b") {
					if (lastFolder.length() == 3) {
						send(connection, "DR", sizeof("DR"), 0);
						std::cout << "\n\n";
						continue;
					}

					int index = lastFolder.find_last_of('\\');
					if (index != -1) {
						lastFolder = lastFolder.substr(0, lastFolder.length() - (lastFolder.length() - index) + 1);
						send(connection, lastFolder.c_str(), lastFolder.length(), 0);
						std::cout << "\n\n";
						continue;
					}
				}
				else if (chouse == "q") {
					exit(0);
				}
				else if (chouse == "x") {
					send(connection, "EX", sizeof("EX"), 0);
					exit(0);
				}
			}
		}
		else {
			std::cout << "EMPTY!\n";
		}
	}

	return 0;
}