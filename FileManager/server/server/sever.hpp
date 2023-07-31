#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <mutex>
#include <algorithm>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "threadPool.hpp"

#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

#define DEFAULT_PORT 8888
#define DEFAULT_IP "localhost"
#define TH_SIZE_MAXIMUM 5
#define TH_SIZE_MINIMUM 2
#define BUFFER_SIZE 16

#define DEF_PATH "files"

enum Command { kCreateNew, kDelete, kWrite, kRead, kExit };

class FileManager
{
public:
    int run();

private:
    int StartServer();
    int StopServer();
    int SendData(SOCKET socket, std::string data, sockaddr clientAddr);
    sockaddr RecvData(SOCKET socket, std::string &data);

    static VOID CALLBACK ThreadStarter(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work);
    int ExecuteCommand(std::string command, sockaddr client_addr);


    int MyCreateFile(std::string name);
    int MyDeleteFile(std::string filename);
    std::string Read(std::string filename);
    int Write(std::string filename, std::string data);
    int Exit();

    std::vector<std::string> files;
    sockaddr_in server_addr_;
    WSADATA wsa_data_;
    SOCKET socket_;
    Command cmd;
    ThreadPool tp_;


    std::mutex mt_;

    PTP_POOL pool_;
    PTP_CLEANUP_GROUP cleanupgroup_;
    PTP_WORK work_;
    PTP_WORK_CALLBACK workcallback_;
    TP_CALLBACK_ENVIRON call_back_environ_;
};

struct Args
{
    FileManager* ptr;
    std::string data;
    sockaddr client_addr;
};