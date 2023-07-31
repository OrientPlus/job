#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

#define DEFAULT_PORT 8888
#define DEFAULT_IP "localhost"
#define BUFFER_SIZE 16

enum Command { kCreateNew, kDelete, kWrite, kRead, kExit };

class FileManager
{
public:
	int run();
private:
	int StartClient();
	int StopClient();
	int SendData(std::string data);
	int RecvData(std::string &data);

	std::string ParsCommand();
	

	WSADATA wsa_data_;
	SOCKET socket_;
	sockaddr_in server_address_;
};