#include <fstream>

#include "sever.hpp"

using std::fstream;
using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;
using std::vector;
using std::lock_guard;
using std::mutex;
using std::min;
using std::cin;
using std::cout;
using std::endl;
using std::cerr;

int FileManager::StartServer()
{
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0)
    {
        cerr << "Не удалось инициализировать Winsock." << std::endl;
        return INVALID_SOCKET;
    }

    // Создаем сокет
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET)
    {
        cerr << "Не удалось создать сокет." << endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Задаем адрес сервера, к которому будут подключаться клиенты
    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(DEFAULT_PORT);
    server_addr_.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Связываем сокет с адресом сервера
    if (bind(socket_, (struct sockaddr*)&server_addr_, sizeof(server_addr_)) == SOCKET_ERROR)
    {
        std::cerr << "Не удалось связать сокет с адресом сервера.\nError: " << GetLastError() << std::endl;
        closesocket(socket_);
        WSACleanup();
        return INVALID_SOCKET;
    }

    
    return 0;
}

int FileManager::StopServer()
{
    closesocket(socket_);
    WSACleanup();
    cout << "Server stopped." << endl;

    return 0;
}

int FileManager::SendData(SOCKET socket, string data, sockaddr clientAddr)
{
    int total_bytes_send = 0, bytes_to_sent, bytes_sent;

    // Отправляем размер данных
    bytes_to_sent = htonl(data.size());
    sendto(socket, reinterpret_cast<char*>(&bytes_to_sent), sizeof(int), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));

    // Отправляем данные блоками
    while (total_bytes_send < data.size())
    {
        bytes_to_sent = min(data.size() - total_bytes_send, BUFFER_SIZE);
        bytes_sent = sendto(socket, data.data(), data.size(), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));

        if (bytes_sent == SOCKET_ERROR)
        {
            cerr << "Ошибка отправки данных." << endl;
            return -1;
        }

        total_bytes_send += bytes_sent;
    }

    return total_bytes_send;
}

sockaddr FileManager::RecvData(SOCKET socket, string &data)
{
    sockaddr client_addr;
    int client_addr_size = sizeof(client_addr), expected_size = 0, bytes_read = 0;
    char local_buffer[BUFFER_SIZE];
    int bytes_to_receive, total_bytes_received = 0;

    // Получаем размер данных
    bytes_read = recvfrom(socket, reinterpret_cast<char*>(&expected_size), sizeof(expected_size), 0, (struct sockaddr*)&client_addr, &client_addr_size);
    if (bytes_read == SOCKET_ERROR)
    {
        cerr << "Ошибка приема данных.\nError: " << GetLastError() << endl;
        return sockaddr{};
    }
    bytes_read = 0;
    expected_size = ntohl(expected_size);

    // Получаем данные блоками 
    while(total_bytes_received < expected_size)
    {
        bytes_to_receive = min(expected_size - total_bytes_received, BUFFER_SIZE);
        bytes_read = recvfrom(socket, local_buffer, bytes_to_receive, 0, (struct sockaddr*)&client_addr, &client_addr_size);
        if (bytes_read == SOCKET_ERROR)
        {
            cerr << "Ошибка приема данных." << endl;
            return sockaddr{};
        }
        total_bytes_received += bytes_read;

        local_buffer[bytes_read] = '\0';
        data += string{ local_buffer };
    }
    return client_addr;
}

VOID FileManager::ThreadStarter(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
    Args* args = reinterpret_cast<Args*>(Parameter);
    args->ptr->ExecuteCommand(args->data, args->client_addr);
}

int FileManager::run()
{
    HANDLE threadHandle;
    // Инициализируем структуры сокетов
    if (StartServer() == -1)
        return -1;


    InitializeThreadpoolEnvironment(&call_back_environ_);
    // Создаем пул потоков и устанавливаем максимальный и минимальный размер
    pool_ = CreateThreadpool(NULL);
    SetThreadpoolThreadMaximum(pool_, TH_SIZE_MAXIMUM);
    SetThreadpoolThreadMinimum(pool_, TH_SIZE_MINIMUM);

    SetThreadpoolCallbackPool(&call_back_environ_, pool_);

    // Создаем группу очистки и ассоциируем ее спулом
    cleanupgroup_ = CreateThreadpoolCleanupGroup();
    SetThreadpoolCallbackCleanupGroup(&call_back_environ_, cleanupgroup_, NULL);

    workcallback_ = ThreadStarter;

    // Принимаем и отправляем данные в бесконечном цикле
    while (true)
    {
        sockaddr client_addr;
        string recv_data;
        client_addr = RecvData(socket_, recv_data);

        Args args;
        args.ptr = this;
        args.data = recv_data;
        args.client_addr = client_addr;
        work_ = CreateThreadpoolWork(workcallback_, &args, &call_back_environ_);
        // Выполняем работу в потоке
        SubmitThreadpoolWork(work_);
    }

    StopServer();
}

int FileManager::ExecuteCommand(string command, sockaddr client_addr)
{
    stringstream ss(command);
    string arg1, arg2;

    ss >> arg1;
    if (arg1.empty())
        return -1;
    Command cmd = static_cast<Command>(std::stoi(arg1));
    if (cmd == kWrite)
    {
        ss >> arg2;
        if (arg2.empty())
            return -1;
    }

    string answer;
    switch (cmd)
    {
    case kCreateNew:
        MyCreateFile(arg1);
        break;
    case kDelete:
        MyDeleteFile(arg1);
        break;
    case kWrite:
        Write(arg1, arg2);
        break;
    case kRead:
        answer = Read(arg1);
        SendData(socket_, answer, client_addr);
        break;
    case kExit:

        break;
    default:
        break;
    }

    return 0;
}

//---------------------------------------------
// Server commands
//
int FileManager::MyCreateFile(string name)
{
    ifstream check_file(DEF_PATH + name);
    if (!check_file.good())
    {
        ofstream file(DEF_PATH + name);

        check_file.close();
        file.close();
        return 0;
    }
    else
    {
        return -1;
    }
}

int FileManager::MyDeleteFile(string filename)
{
    if (remove(string{ DEF_PATH + filename }.c_str()) != 0)
    {
        cerr << "Ошибка при удалении файла: " << filename << endl;
        return -1;
    }
    return 0;
}

string FileManager::Read(string filename)
{
    fstream file(DEF_PATH + filename);
    if (!file.good())
    {
        cerr << "Ошибка открытия файла для чтения!";
        return string{""};
    }
    
    string file_data;
    while (!file.eof())
    {
        string temp_str;
        file >> temp_str;
        file_data += temp_str;
    }
    
    file.close();
    return file_data;
}

int FileManager::Write(string filename, string data)
{
    fstream file(DEF_PATH + filename);
    if (!file.good())
    {
        cerr << "Ошибка открытия файла для записи!";
        return -1;
    }

    file << data;

    return 0;
}

int FileManager::Exit()
{
    return 0;
}