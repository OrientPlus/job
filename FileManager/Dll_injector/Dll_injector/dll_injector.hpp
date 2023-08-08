#pragma once
#include "pch.h"

#include <vector>
#include <string>
#include <Windows.h>
#include "zlib.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable:4996)

#define DEFAULT_PORT 8888
#define DEFAULT_IP "127.0.0.1"
#define BUFFER_SIZE 128

#define DECLSPEC extern "C" __declspec(dllexport)
#define DECLSPECPP extern "C++" __declspec(dllexport)

DWORD64 SSL_read_addr;
DWORD64 SSL_write_addr;

// Находит все процессы использующие openssl
DECLSPECPP std::vector<std::pair<HANDLE, DWORD64>> FindOpensslProcesses();

// Грузится в адресное пространство указанного процесса
DECLSPEC int Load(HANDLE hProcess);

// Заменяет адреса функций на собственные
DECLSPEC int Inject(HANDLE hProcess);

DECLSPEC int My_SSL_read(DWORD* ssl, void* buf, int num);
DECLSPEC int My_SSL_write(DWORD* ssl, const void* buf, int num);

DECLSPEC int _My_SSL_read(std::string &read_buf);
DECLSPEC int _My_SSL_write(std::string &write_buf);

DECLSPEC unsigned GetCRC32(std::string data);