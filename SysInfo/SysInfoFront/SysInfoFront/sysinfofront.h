#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_sysinfofront.h"

#include <QtWidgets/QMainWindow>
#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QSignalMapper>
#include <QMenu>
#include <QStandardItemModel>
#include <QTableView>
#include <QStringList>
#include <QDebug>

#include <string>
#include <vector>
#include <winsock2.h>

#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

#include "zlib.h"
#include "nlohmann/json.hpp"

#define DEFAULT_PORT 8888
#define DEFAULT_IP "127.0.0.1"
#define BUFFER_SIZE 512

using std::string;
using std::vector;

using nlohmann::json;

class SysInfoFront : public QMainWindow
{
    Q_OBJECT

public:
    SysInfoFront(QWidget* parent = nullptr);
    ~SysInfoFront();

private:
    int OpenSocket();
    int CloseSocket();
    int SendData(string data);
    int RecvData(string& data);
    unsigned GetCRC32(string data);

    void DisplayDiskInfo();
    void DisplayProcInfo();
    void DisplayNetworkInfo();
    void DisplayDeviceInfo();


    Ui::SysInfoFrontClass ui;

    sockaddr_in server_address_, addr_;
    WSADATA wsa_data_;
    SOCKET socket_;
};
