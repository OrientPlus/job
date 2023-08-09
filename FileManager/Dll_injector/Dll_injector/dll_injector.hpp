#pragma once
#include "pch.h"

#include <iostream>
#include <vector>
#include <string>
#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <Dbghelp.h>

#include "zlib.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Dbghelp.lib")
//#pragma comment(lib, "zlib.lib")

#pragma warning(disable:4996)

#define DEFAULT_PORT 8888
#define DEFAULT_IP "127.0.0.1"
#define BUFFER_SIZE 128

#define DECLSPEC extern "C" __declspec(dllexport)
#define DECLSPECPP  __declspec(dllexport)

// Находит все процессы использующие openssl
DECLSPECPP std::vector<std::pair<HANDLE, DWORD64>> FindOpensslProcesses();

// Заменяет адреса функций на собственные
DECLSPECPP int Inject();
DECLSPECPP int FindModule(HMODULE *hModuleDll);

DECLSPEC int My_SSL_read(DWORD* ssl, void* buf, int num);
DECLSPEC int My_SSL_write(DWORD* ssl, const void* buf, int num);

DECLSPECPP unsigned GetCRC32(std::string data);