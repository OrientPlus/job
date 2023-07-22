#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "cryptographer.hpp"
#include "access_rights.hpp"

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "8888"
#define TH_SIZE_MAXIMUM 5
#define TH_SIZE_MINIMUM 2
#define RECV_BUFFER_SIZE 4096


using std::string;
using std::vector;
using std::lock_guard;
using std::mutex;

enum Command { kCreateNew, kDelete, kWrite, kRead, kSaveAll, kLoadAll, kGetActualNoteList, kGetNoteTypeInfo, kChangeType };

class NotesManager
{
public:
    int run();

private:
    int StartServer();
    int StopServer();
    int SendData(SOCKET socket, string data);
    int RecvData(SOCKET socket, string &data);

    static VOID CALLBACK ThreadStarter(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work);
    int ExecuteCommand();
    int ParsCommand(User user, string &data);
    Note CreateNote(string name, NoteType type, string key, string owner);
    int DeleteNote(vector<Note>::iterator note_it);
    string ReadNote(vector<Note>::iterator note_it);
    int WriteNote(vector<Note>::iterator note_it, string data);
    int SaveAllNotes();
    string LoadAllNotes();
    string GetNoteList();
    NoteType GetNoteTypeInfo(string note_name);
    bool IdentificationClient(User &user, SOCKET user_socket);


    AccessRights access_rights_;
    Cryptographer crypt_;

    mutex mt_;

    // Поля многопотока
    PTP_POOL pool_;
    PTP_CLEANUP_GROUP cleanupgroup_;
    PTP_WORK work_;
    PTP_WORK_CALLBACK workcallback_;
    TP_CALLBACK_ENVIRON call_back_environ_;

    SOCKET listen_socket_, client_socket_;
    int server_port;


};