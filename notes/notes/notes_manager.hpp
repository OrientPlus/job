#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "cryptographer.hpp"
#include "access_rights.hpp"

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "8888"
#define TH_SIZE_MAXIMUM 5
#define TH_SIZE_MINIMUM 2
#define RECV_BUFFER_SIZE 1024


using std::string;
using std::vector;


class NotesManager
{
public:
    int run();

private:
    int StartServer();
    int StopServer();
    int ExecuteCommand();
    int SendData(SOCKET socket, string data);
    int RecvData(SOCKET socket, string data);

    int CreateNote(string name);
    int DeleteNote(string name);
    int EditNote(string name);
    int ReadNote(string name);
    int SaveAllNotes();
    int LoadAllNotes();

    bool IdentificationClient(User &user, SOCKET user_socket);


    AccessRights access_rights_;
    Cryptographer crypt_;

    // Поля многопотока
    PTP_POOL pool_;
    PTP_CLEANUP_GROUP cleanupgroup_;
    PTP_WORK work_;
    PTP_WORK_CALLBACK workcallback_;
    TP_CALLBACK_ENVIRON call_back_environ_;

    SOCKET listen_socket_, client_socket_;
    int server_port;


};