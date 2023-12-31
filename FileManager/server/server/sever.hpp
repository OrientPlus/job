#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <queue>
#include <filesystem>
#include <mutex>
#include <algorithm>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#include <zlib.h>

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

#define DEFAULT_PORT 8888
#define DEFAULT_IP "127.0.0.1"
#define TH_SIZE_MAXIMUM 5
#define TH_SIZE_MINIMUM 2
#define BUFFER_SIZE 128

#define DEF_PATH "files/"

enum Command { kCreateNew, kDelete, kWrite, kRead, kExit, kAppend, kList };

class FileManager
{
public:
    void run();

private:
    int StartServer();
    int StopServer();
    int SendData(std::string data, sockaddr_in client_addr);
    int RecvData(std::string &data, sockaddr_in &client_addr);
    unsigned GetCRC32(std::string data);

    static VOID CALLBACK StartHiddener(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work);
    void ExecHiddenCommand();

    static VOID CALLBACK ThreadStarter(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work);
    int ExecuteCommand(std::string command, sockaddr_in client_addr);

    void VectorUniquePush(std::vector<sockaddr_in>& vec, sockaddr_in elem);


    int MyCreateFile(std::string name);
    int MyDeleteFile(std::string filename);
    int Read(std::string filename, std::string &file_data);
    int Write(std::string filename, std::string data, Command cmd);
    int GetFileList(std::string& file_list);

    sockaddr_in server_addr_;
    WSADATA wsa_data_;
    SOCKET socket_;
    Command cmd;

    std::queue<std::pair<std::string, sockaddr_in>> requests_q;

    std::mutex mt_;
    std::condition_variable cv_;

    PTP_POOL pool_;
    PTP_CLEANUP_GROUP cleanupgroup_;
    PTP_WORK work_;
    PTP_WORK_CALLBACK workcallback_;
    TP_CALLBACK_ENVIRON call_back_environ_;

    std::string last_error_, 
        client_data_;
    std::vector<sockaddr_in> clients_addr_;
};

struct Args
{
    FileManager* ptr_;
    std::string data_;
    sockaddr_in client_addr_;
};