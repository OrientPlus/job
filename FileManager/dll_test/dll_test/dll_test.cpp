#include <Windows.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <Dbghelp.h>
#pragma comment(lib, "Dbghelp.lib")
#include <iostream>
#include <vector>
#include <string>
#include <set>

using namespace std;

// Находит все процессы использующие OpenSSL
int FindOpensslProcesses(vector<pair<HANDLE, DWORD>> &openssl_process)
{
    vector<pair<HANDLE, DWORD>> openssl_processes;
    DWORD processes[1024], bytes_needed;

    if (!EnumProcesses(processes, sizeof(processes), &bytes_needed))
    {
        cout << "Error in EnumProcess: " << GetLastError();
        return 0;
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
                            openssl_process.push_back(make_pair(hProcess, processes[i]));
                            break;
                        }
                    }
                }
            }
        }

        CloseHandle(hProcess);
    }
    return 0;
}

int main()
{
    // Находим все процессы использующие openssl
    auto ret = LoadLibraryA("Dll_injector.dll");
    if (!ret)
        cout << "Error load: " << GetLastError() << endl;


    vector<pair<HANDLE, DWORD>> ssl_proc;
    FindOpensslProcesses(ssl_proc);
    HMODULE hModule;

    // Пробегаемся по всем процессам использующим OpenSSL
    for (auto it : ssl_proc)
    {
        cout << "load" << endl;
        // Грузим библиотеку в процесс
        HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, it.second);
        if (!hProc)
        {
            std::cerr << "Failed to open the process. Error: " << GetLastError() << std::endl;
            return 1;
        }

        HMODULE hKernel32 = GetModuleHandle(L"kernel32.dll");
        FARPROC pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");

        if (!pLoadLibraryW) 
        {
            std::cerr << "Failed to get address of LoadLibraryW" << std::endl;
            return -1;
        }

        size_t pathSize = (wcslen(L"C:\\Users\\gutro\\Desktop\\job\\FileManager\\dll_test\\dll_test\\Dll_injector.dll") + 1) * sizeof(wchar_t);

        LPVOID remotePath = VirtualAllocEx(hProc, nullptr, pathSize, MEM_COMMIT, PAGE_READWRITE);
        if (!remotePath)
        {
            std::cerr << "Failed to allocate memory in remote process. Error: " << GetLastError() << std::endl;
            return -1;
        }

        DWORD written;
        if (!WriteProcessMemory(hProc, remotePath, "C:\\Users\\gutro\\Desktop\\job\\FileManager\\dll_test\\dll_test\\Dll_injector.dll", pathSize, &written))
        {
            std::cerr << "Failed to write DLL path in remote process. Error: " << GetLastError() << "\nBytes written: " << written << std::endl;
            VirtualFreeEx(it.first, remotePath, 0, MEM_RELEASE);
            return -1;
        }

        HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0,
            reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoadLibraryW),
            remotePath, 0, nullptr);
        if (!hThread)
        {
            std::cerr << "Failed to create remote thread" << std::endl;
            VirtualFreeEx(hProc, remotePath, 0, MEM_RELEASE);
            return -1;
        }

        WaitForSingleObject(hThread, INFINITE);
        DWORD exitCode;
        GetExitCodeThread(hThread, &exitCode);

        cout << "\nExit code: " << exitCode << endl;
        CloseHandle(hThread);
        VirtualFreeEx(hProc, remotePath, 0, MEM_RELEASE);
    }
    system("pause");
    return 0;
}