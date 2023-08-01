#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <filesystem>
#include <mutex>
#include <algorithm>
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
    int run();

private:
    int StartServer();
    int StopServer();
    int SendData(std::string data, sockaddr client_addr);
    int RecvData(std::string &data, sockaddr &client_addr);
    unsigned GetCRC32(std::string data);

    static VOID CALLBACK ThreadStarter(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work);
    int ExecuteCommand(std::string command, sockaddr client_addr);


    int MyCreateFile(std::string name);
    int MyDeleteFile(std::string filename);
    int Read(std::string filename, std::string &file_data);
    int Write(std::string filename, std::string data, Command cmd);
    int GetFileList(std::string& file_list);

    sockaddr_in server_addr_;
    WSADATA wsa_data_;
    SOCKET socket_;
    Command cmd;

    std::mutex mt_;
    PTP_POOL pool_;
    PTP_CLEANUP_GROUP cleanupgroup_;
    PTP_WORK work_;
    PTP_WORK_CALLBACK workcallback_;
    TP_CALLBACK_ENVIRON call_back_environ_;

    std::string last_error_;
};

struct Args
{
    FileManager* ptr_;
    std::string data_;
    sockaddr client_addr_;
};