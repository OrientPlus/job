// dllmain.cpp : Определяет точку входа для приложения DLL.
#include "pch.h"
#include "dll_injector.hpp"

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        std::vector<std::pair<HANDLE, DWORD64>> ssl_processes = FindOpensslProcesses();
        for (auto it : ssl_processes)
        {
            Inject(it.first);
        }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

