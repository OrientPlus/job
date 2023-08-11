#include "sys_info.hpp"


int SysInfo::run()
{
    OpenSocket();

    string proc_list, device_info, disk_info, netw_act;
    string temp;
    while (true)
    {
        string request, info;
        RecvData(request);
        if (request == "#1")
            GetDiskInfo(info);
        else if (request == "#2")
            GetProcessList(info);
        else if (request == "#3")
            GetNetworkActivity(info);
        else if (request == "#4")
            GetDeviceInfo(info);
        else
            info = "Invalid request";

        SendData(info);
    }

    CloseSocket();
    return 0;
}

int SysInfo::OpenSocket()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0)
    {
        cerr << "Не удалось инициализировать Winsock. Error: " << GetLastError() << std::endl;
        return INVALID_SOCKET;
    }

    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET)
    {
        cerr << "Не удалось создать сокет. Error: " << GetLastError() << endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(DEFAULT_PORT);
    addr_.sin_addr.s_addr = inet_addr(DEFAULT_IP);

    if (bind(socket_, (struct sockaddr*)&addr_, sizeof(addr_)) == SOCKET_ERROR)
    {
        std::cerr << "Не удалось связать сокет с адресом сервера.\nError: " << GetLastError() << std::endl;
        CloseSocket();
        return INVALID_SOCKET;
    }


    return 0;
}

int SysInfo::CloseSocket()
{
    closesocket(socket_);
    WSACleanup();

    cout << "Socket closed" << endl;
    return 0;
}

int SysInfo::SendData(string data)
{
    int total_bytes_send = 0, bytes_to_sent, bytes_sent;

    // Отправляем размер данных
    bytes_to_sent = htonl(data.size());
    sendto(socket_, reinterpret_cast<char*>(&bytes_to_sent), sizeof(int), 0, (struct sockaddr*)&gui_addr_, sizeof(gui_addr_));

    // Отправляем данные блоками
    while (total_bytes_send < data.size())
    {
        bytes_to_sent = min(data.size() - total_bytes_send, BUFFER_SIZE);
        bytes_sent = sendto(socket_, data.data() + total_bytes_send, bytes_to_sent, 0, (struct sockaddr*)&gui_addr_, sizeof(gui_addr_));

        if (bytes_sent == SOCKET_ERROR)
        {
            cerr << "Ошибка отправки данных. Error: " << GetLastError() << endl;
            return -1;
        }

        total_bytes_send += bytes_sent;
    }
    // Отправляем контрольную сумму
    unsigned checksum = GetCRC32(data);
    checksum = htonl(checksum);
    bytes_sent = sendto(socket_, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0, (struct sockaddr*)&gui_addr_, sizeof(gui_addr_));
    if (bytes_sent == SOCKET_ERROR)
    {
        cerr << "Ошибка отправки данных.\nError: " << GetLastError() << endl;
        return -1;
    }

    return total_bytes_send;
}

int SysInfo::RecvData(string& data)
{
    int client_addr_size = sizeof(gui_addr_), expected_size = 0, bytes_read = 0;
    char local_buffer[BUFFER_SIZE + 1];
    int bytes_to_receive, total_bytes_received = 0;

    // Getting the size of the data
    bytes_read = recvfrom(socket_, reinterpret_cast<char*>(&expected_size), sizeof(expected_size), 0, (struct sockaddr*)&gui_addr_, &client_addr_size);
    if (bytes_read == SOCKET_ERROR)
    {
        cerr << "Ошибка приема данных.\nError: " << GetLastError() << endl;
        return -1;
    }
    bytes_read = 0;
    expected_size = ntohl(expected_size);

    // Getting data in blocks
    while (total_bytes_received < expected_size)
    {
        bytes_to_receive = min(expected_size - total_bytes_received, BUFFER_SIZE);
        bytes_read = recvfrom(socket_, local_buffer, bytes_to_receive, 0, (struct sockaddr*)&gui_addr_, &client_addr_size);
        if (bytes_read == SOCKET_ERROR)
        {
            cerr << "Ошибка приема данных.\nError: " << GetLastError() << endl;
            return -1;
        }
        total_bytes_received += bytes_read;

        local_buffer[bytes_read] = '\0';
        data += string{ local_buffer };
    }

    // Getting the checksum of the data
    unsigned checksum;
    bytes_read = recvfrom(socket_, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0, (struct sockaddr*)&gui_addr_, &client_addr_size);
    if (bytes_read == SOCKET_ERROR)
    {
        cerr << "Ошибка приема данных.\nError: " << GetLastError() << endl;
        return -1;
    }
    checksum = ntohl(checksum);

    // Check that the checksum matches
    unsigned calculated_checksum = GetCRC32(data);
    if (calculated_checksum != checksum)
    {
        cout << "The checksum does not match!" << endl;
        return -1;
    }

    return 0;
}

unsigned SysInfo::GetCRC32(string data)
{
    unsigned int crc = crc32(0L, Z_NULL, 0);

    crc = crc32(crc, reinterpret_cast<const Bytef*>(data.c_str()), data.length());

    return crc;
}



//-------------------------------------------------
//          Методы сбора информации
//
int SysInfo::GetDiskInfo(string& data)
{
    DWORD drives = GetLogicalDrives();
    wchar_t driveLetter = 'A';

    json di_json;
    while (drives)
    {
        if (drives & 1) {
            wstring drivePath = wstring(1, driveLetter) + L":\\";

            ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
            if (GetDiskFreeSpaceEx(drivePath.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes))
            {
                string path(wstringToString(wstring(drivePath))),
                    total_bytes(std::to_string(totalBytes.QuadPart /1024 /1024)),
                    free_bytes(std::to_string(freeBytesAvailable.QuadPart /1024 /1024));

                json temp_json;
                temp_json["DISK"] = path;
                temp_json["TOTAL SIZE"] = total_bytes;
                temp_json["FREE SIZE"] = free_bytes;

                di_json.push_back(temp_json);
            }
        }

        drives >>= 1;
        driveLetter++;
    }

    data = di_json.dump();
    return 0;
}

int SysInfo::GetProcessList(string& data)
{
    // Получаем список процессов
    DWORD aProcesses[1024], cbNeeded, cProcesses;
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
    {
        cout << "Error getting the list of running processes. Error: " << GetLastError() << endl;
        return -1;
    }

    json pl_json;
    cProcesses = cbNeeded / sizeof(DWORD);
    for (DWORD i = 0; i < cProcesses; i++)
    {
        if (aProcesses[i] != 0)
        {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
            if (hProcess)
            {
                TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
                HMODULE hMod;
                DWORD cbNeeded;

                if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
                    GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));

                GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
                string proc_id(std::to_string(aProcesses[i])),
                    proc_name(wstringToString(wstring(szProcessName))),
                    proc_mem(std::to_string(pmc.WorkingSetSize/1024)),
                    proc_pr_mem(std::to_string(pmc.PrivateUsage/1024));

                json temp_json;
                temp_json["ID"] = proc_id;
                temp_json["NAME"] = proc_name;
                temp_json["USAGE_MEM"] = proc_mem;
                temp_json["PRIVATE MEM"] = proc_pr_mem;

                pl_json.push_back(temp_json);

                CloseHandle(hProcess);
            }
        }
    }

    data = pl_json.dump();
    return 0;
}

int SysInfo::GetNetworkActivity(string& data)
{
    auto MacAddressToString = [](const BYTE* macAddr, DWORD macAddrLen) -> std::string {
        char macStr[18]; // 17 символов для адреса + 1 символ для нуль-терминатора
        sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
            macAddr[0], macAddr[1], macAddr[2],
            macAddr[3], macAddr[4], macAddr[5]);
        return macStr;
    };

    auto InterfaceStatus = [](const DWORD status) -> std::string {
        
        if (status == IF_OPER_STATUS_NON_OPERATIONAL)
            return "OPERATIONAL";
        else if (status == IF_OPER_STATUS_UNREACHABLE)
            return "UNREACHABLE";
        else if (status == IF_OPER_STATUS_DISCONNECTED)
            return "DISCONNECTED";
        else if (status == IF_OPER_STATUS_CONNECTING)
            return "CONNECTING";
        else if (status == IF_OPER_STATUS_CONNECTED)
            return "CONNECTED";
        else
            return "NONE";
    };

    ULONG ulOutBufLen = 0;
    MIB_IFTABLE* ifTable = NULL;

    json na_json;
    DWORD result = GetIfTable(ifTable, &ulOutBufLen, FALSE);
    ifTable = (MIB_IFTABLE*)malloc(ulOutBufLen);
    result = GetIfTable(ifTable, &ulOutBufLen, TRUE);
    if (result == NO_ERROR)
    {
        for (DWORD i = 0; i < ifTable->dwNumEntries; i++)
        {
            MIB_IFROW ifRow = ifTable->table[i];

            json temp_json;


            temp_json["INTERFACE"] = ConvertToMultibyte(ifRow.wszName);
            temp_json["MTU"] = std::to_string(ifRow.dwMtu);
            temp_json["MAC"] = MacAddressToString(ifRow.bPhysAddr, ifRow.dwPhysAddrLen);
            temp_json["STATUS"] = InterfaceStatus(ifRow.dwOperStatus);
            temp_json["BYTES SENT"] = std::to_string(ifRow.dwOutOctets/1024);
            temp_json["BYTES RECV"] = std::to_string(ifRow.dwInOctets/1024);

            na_json.push_back(temp_json);
        }
    }
    else
        std::cerr << "Failed to get network activity information. Error: " << GetLastError() << std::endl;

    free(ifTable);

    data = na_json.dump();
    return 0;
}

int SysInfo::GetDeviceInfo(string& data)
{
    HDEVINFO hDevInfo = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE)
    {
        cerr << "Failed to get device information. Error: " << GetLastError() << std::endl;
        return -1;
    }

    SP_DEVINFO_DATA deviceInfoData;
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    json device_json;
    for (DWORD i = 0; SetupDiEnumDeviceInfo(hDevInfo, i, &deviceInfoData); i++)
    {
        WCHAR deviceName[200];
        ZeroMemory(&deviceName, 200);
        DWORD size = sizeof(deviceName) / sizeof(deviceName[0]);

        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &deviceInfoData, SPDRP_DEVICEDESC, NULL, (PBYTE)deviceName, size, NULL))
        {
            DEVPROPTYPE propType;
            WCHAR vid[10], pid[10];
            ZeroMemory(&vid, 10);
            ZeroMemory(&pid, 10);

            SetupDiGetDeviceRegistryPropertyW(hDevInfo, &deviceInfoData, SPDRP_HARDWAREID, NULL, (PBYTE)vid, sizeof(vid), NULL);

            SetupDiGetDeviceRegistryPropertyW(hDevInfo, &deviceInfoData, SPDRP_COMPATIBLEIDS, NULL, (PBYTE)pid, sizeof(pid), NULL);

            string name = ConvertToMultibyte(deviceName),
                vendor = ConvertToMultibyte(vid),
                product = ConvertToMultibyte(pid);

            if (vendor.empty())
                vendor = "unknown";
            if (product.empty())
                product = "unknown";

            json temp_json;
            try
            {
                temp_json["NAME"] = name;
                temp_json["VENDOR ID"] = vendor;
                temp_json["PRODUCT ID"] = product;
            }
            catch (const std::exception &ex)
            {
                cout << "JSON throw exception!" << endl;
                vendor = "unknown";
                product = "unknown";

                temp_json["NAME"] = name;
                temp_json["VENDOR ID"] = vendor;
                temp_json["PRODUCT ID"] = product;
            }

            device_json.push_back(temp_json);
        }
    }

    SetupDiDestroyDeviceInfoList(hDevInfo);

    data = device_json.dump(0, ' ', true, nlohmann::json::error_handler_t::replace);
    //data = device_json.dump();
    return 0;
}


string SysInfo::wstringToString(const wstring& wstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    string str;

    for (wchar_t wc : wstr) {
        try {
            str += converter.to_bytes(wc);
        }
        catch (const std::range_error& e) {
            str += "?";
        }
    }
    return str;
}

string SysInfo::ConvertToMultibyte(WCHAR* wstr)
{
    // Конвертируем WCHAR в wstring
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    wstring wtemp(wstr);

    // Конвертируем wstring в string
    string str = converter.to_bytes(wtemp);

    return str;
}