#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <locale>
#include <codecvt>

#include <ws2tcpip.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)
#include <Windows.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")
#include <Iphlpapi.h>
#pragma comment(lib, "Iphlpapi.lib")
#include <Setupapi.h>
#pragma comment(lib, "Setupapi.lib")

#include "zlib.h"
#include <nlohmann/json.hpp>

#define DEFAULT_PORT 8888
#define DEFAULT_IP "127.0.0.1"
#define BUFFER_SIZE 512



using std::string;
using std::wstring;
using std::vector;
using std::cout;
using std::cerr;
using std::endl;
using nlohmann::json;

// - Процессы
// - Занятое место на дисках
// - Сетевая активность
// - Информация об устройствах
// - 


class SysInfo
{
public:
	int run();
private:
	// Служебные методы обеспечивающие взаимодействия с GUI
	int OpenSocket();
	int CloseSocket();
	int SendData(string data);
	int RecvData(string& data);
	unsigned GetCRC32(string data);
	string wstringToString(const wstring& wstr);


	// Методы собирающие информацию
	int GetProcessList(string& data);
	int GetDiskInfo(string& data);
	int GetNetworkActivity(string& data);
	int GetDeviceInfo(string& data);


	sockaddr_in addr_, gui_addr_;
	WSADATA wsa_data_;
	SOCKET socket_;
};
