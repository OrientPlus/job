#pragma once

#include <Windows.h>

class Redirector
{
private:
	HMODULE m_hModule = NULL;
	HMODULE m_hApiModule = NULL;
	PIMAGE_THUNK_DATA m_pThunk = NULL;
public:
	Redirector() {};
	bool setModule(HMODULE hModule);
	bool setModule(LPCSTR lpModuleName);

	bool setApiModule(HMODULE hApiModule);
	bool setApiModule(LPCSTR lpApiModuleName);

	bool redirect(LPCSTR lpProcName, LPVOID lpNewProcAddress);
	bool redirect(int nProcOrdinal, LPVOID lpNewProcAddress);
};