#include "redirector.hpp"

// Устанавливаем наш модуль
bool Redirector::setModule(HMODULE hModule)
{
	if (!hModule)
		return false;

	m_hModule = hModule;

	return true;
}

bool Redirector::setModule(LPCSTR lpModuleName)
{
	return setModule(GetModuleHandleA(lpModuleName));
}

// Выбираем модуль, функции которого будем подменивать
bool Redirector::setApiModule(HMODULE hApiModule)
{
	if (!hApiModule)
		return false;

	m_hApiModule = hApiModule;

	if (!m_hModule)
		return false;

	// Проверка, dll/exe это или нет
	if (PIMAGE_DOS_HEADER(m_hModule)->e_magic != IMAGE_DOS_SIGNATURE)
		return false;

	// Точно ли это исполняемый файл?
	PIMAGE_NT_HEADERS pNTHeader = PIMAGE_NT_HEADERS(PBYTE(m_hModule) + PIMAGE_DOS_HEADER(m_hModule)->e_lfanew);
	if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
		return false;

	// Получаем адрес таблицы импорта
	DWORD dwImportRVA = pNTHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
	PIMAGE_IMPORT_DESCRIPTOR pImportDesc = (PIMAGE_IMPORT_DESCRIPTOR)((ULONG_PTR)(m_hModule)+(ULONG_PTR)(dwImportRVA));

	// Ищем нашу таблицу импорта
	while (pImportDesc->FirstThunk)
	{
		if (GetModuleHandleA((PSTR)((ULONG_PTR)(m_hModule)+(ULONG_PTR)(pImportDesc->Name))) == hApiModule)
			break;

		pImportDesc++;
	}

	if (!pImportDesc->FirstThunk)
		return false;

	// Сохраняем адрес нашей таблицы импорта
	m_pThunk = (PIMAGE_THUNK_DATA)((PBYTE)m_hModule + (DWORD)pImportDesc->FirstThunk);

	return true;
}

bool Redirector::setApiModule(LPCSTR lpApiModuleName)
{
	return setApiModule(GetModuleHandleA(lpApiModuleName));
}

// Меняем адрес в таблице импорта на наш
bool Redirector::redirect(LPCSTR lpProcName, LPVOID lpNewProcAddress)
{
	// Получаем адреса функций
	ULONG_PTR uNewProcAddress = (ULONG_PTR)lpNewProcAddress;
	ULONG_PTR uRoutineAddress = (ULONG_PTR)GetProcAddress(m_hApiModule, lpProcName);
	PIMAGE_THUNK_DATA pThunk = m_pThunk;

	if (!uRoutineAddress)
		return false;

	if (!pThunk)
		return false;

	// Ищем нашу функцию
	while (pThunk->u1.Function)
	{
		// Получаем адрес
		ULONG_PTR* uAddress = (ULONG_PTR*)&pThunk->u1.Function;
		if (*uAddress == uRoutineAddress)
		{
			DWORD dwOldProtect = NULL;

			// Подмениваем адрес
			VirtualProtect((LPVOID)uAddress, 4, PAGE_WRITECOPY, &dwOldProtect);
			*uAddress = uNewProcAddress;
			VirtualProtect((LPVOID)uAddress, 4, dwOldProtect, NULL);

			return true;
		}

		pThunk++;
	}

	return false;
}

bool Redirector::redirect(int nProcOrdinal, LPVOID lpNewProcAddress)
{
	return redirect((LPCSTR)nProcOrdinal, lpNewProcAddress);
}