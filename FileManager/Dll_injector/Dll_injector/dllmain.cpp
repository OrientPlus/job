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
        std::cout << "Loading dll" << std::endl;
        Inject();
    case DLL_THREAD_ATTACH:
        std::cout << "Loading dll" << std::endl;
        Inject();
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

