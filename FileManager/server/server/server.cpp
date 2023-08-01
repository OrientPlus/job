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

    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET)
    {
        cerr << "Не удалось создать сокет." << endl;
        WSACleanup();
        return INVALID_SOCKET;
    }

    server_addr_.sin_family = AF_INET;
    server_addr_.sin_port = htons(DEFAULT_PORT);
    server_addr_.sin_addr.s_addr = inet_addr(DEFAULT_IP);

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

int FileManager::SendData(string data, sockaddr clientAddr) 
{
    int total_bytes_send = 0, bytes_to_sent, bytes_sent;

    // Отправляем размер данных
    bytes_to_sent = htonl(data.size());
    sendto(socket_, reinterpret_cast<char*>(&bytes_to_sent), sizeof(int), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));

    // Отправляем данные блоками
    while (total_bytes_send < data.size())
    {
        bytes_to_sent = min(data.size() - total_bytes_send, BUFFER_SIZE);
        bytes_sent = sendto(socket_, data.data() + total_bytes_send, bytes_to_sent, 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));

        if (bytes_sent == SOCKET_ERROR)
        {
            cerr << "Ошибка отправки данных." << endl;
            return -1;
        }

        total_bytes_send += bytes_sent;
    }
    // Отправляем контрольную сумму
    unsigned checksum = GetCRC32(data);
    checksum = htonl(checksum);
    bytes_sent = sendto(socket_, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0, (struct sockaddr*)&clientAddr, sizeof(clientAddr));
    if (bytes_sent == SOCKET_ERROR)
    {
        cerr << "Ошибка отправки данных.\nError: " << GetLastError() << endl;
        return -1;
    }

    return total_bytes_send;
}

int FileManager::RecvData(string &data, sockaddr &client_addr)
{
    int client_addr_size = sizeof(client_addr), expected_size = 0, bytes_read = 0;
    char local_buffer[BUFFER_SIZE + 1];
    int bytes_to_receive, total_bytes_received = 0;

    // Getting the size of the data
    bytes_read = recvfrom(socket_, reinterpret_cast<char*>(&expected_size), sizeof(expected_size), 0, (struct sockaddr*)&client_addr, &client_addr_size);
    if (bytes_read == SOCKET_ERROR)
    {
        cerr << "Ошибка приема данных.\nError: " << GetLastError() << endl;
        last_error_ = "Data acquisition error!";
        return -1;
    }
    bytes_read = 0;
    expected_size = ntohl(expected_size);

    // Getting data in blocks
    while(total_bytes_received < expected_size)
    {
        bytes_to_receive = min(expected_size - total_bytes_received, BUFFER_SIZE);
        bytes_read = recvfrom(socket_, local_buffer, bytes_to_receive, 0, (struct sockaddr*)&client_addr, &client_addr_size);
        if (bytes_read == SOCKET_ERROR)
        {
            cerr << "Ошибка приема данных.\nError: " << GetLastError() << endl;
            last_error_ = "Data acquisition error!";
            return -1;
        }
        total_bytes_received += bytes_read;

        local_buffer[bytes_read] = '\0';
        data += string{ local_buffer };
    }

    // Getting the checksum of the data
    unsigned checksum;
    bytes_read = recvfrom(socket_, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0, (struct sockaddr*)&client_addr, &client_addr_size);
    if (bytes_read == SOCKET_ERROR)
    {
        cerr << "Ошибка приема данных.\nError: " << GetLastError() << endl;
        last_error_ = "Data acquisition error!";
        return -1;
    }
    checksum = ntohl(checksum); 

    // Check that the checksum matches
    unsigned calculated_checksum = GetCRC32(data);
    if (calculated_checksum != checksum)
    {
        last_error_ = "The checksum does not match!";
        return -1;
    }

    return 0;
}

unsigned FileManager::GetCRC32(std::string data)
{
    unsigned int crc = crc32(0L, Z_NULL, 0);

    crc = crc32(crc, reinterpret_cast<const Bytef*>(data.c_str()), data.length());

    return crc;
}

VOID FileManager::ThreadStarter(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
    Args* args = reinterpret_cast<Args*>(Parameter);
    args->ptr_->ExecuteCommand(args->data_, args->client_addr_);
}

int FileManager::run()
{
    HANDLE threadHandle;
    if (StartServer() == -1)
        return -1;


    InitializeThreadpoolEnvironment(&call_back_environ_);
    pool_ = CreateThreadpool(NULL);
    SetThreadpoolThreadMaximum(pool_, TH_SIZE_MAXIMUM);
    SetThreadpoolThreadMinimum(pool_, TH_SIZE_MINIMUM);

    SetThreadpoolCallbackPool(&call_back_environ_, pool_);

    cleanupgroup_ = CreateThreadpoolCleanupGroup();
    SetThreadpoolCallbackCleanupGroup(&call_back_environ_, cleanupgroup_, NULL);

    workcallback_ = ThreadStarter;
    
    Args* args = new Args;
    while (true)
    {
        sockaddr client_addr;
        string recv_data;
        if (RecvData(recv_data, client_addr) == -1)
        {
            SendData(last_error_, client_addr);
            continue;
        }

        args->ptr_ = this;
        args->data_ = recv_data;
        args->client_addr_ = client_addr;
        work_ = CreateThreadpoolWork(workcallback_, args, &call_back_environ_);
        SubmitThreadpoolWork(work_);
    }

    StopServer();
    delete args;
    return 0;
}

int FileManager::ExecuteCommand(string command, sockaddr client_addr)
{
    stringstream ss(command);
    string arg1, arg2;

    ss >> command >> arg1;
    if (command.empty())
    {
        cerr << "Invalid arguments" << endl;
        last_error_ = "Invalid arguments";
        SendData(last_error_, client_addr);
        return 0;
    }
    Command cmd = static_cast<Command>(std::stoi(command));
    if (cmd != kList and arg1.empty())
    {
        cerr << "Invalid arguments" << endl;
        last_error_ = "Invalid arguments";
        SendData(last_error_, client_addr);
        return 0;
    }
    if (cmd == kWrite or cmd == kAppend)
    {
        std::getline(ss, arg2);
        if (arg2.empty())
        {
            cerr << "Invalid arguments" << endl;
            return -1;
        }
        arg2.erase(0, 1);
    }

    int ret_value;
    string data;
    switch (cmd)
    {
    case kCreateNew:
        ret_value = MyCreateFile(arg1);
        break;
    case kDelete:
        ret_value = MyDeleteFile(arg1);
        break;
    case kAppend:
    case kWrite:
        ret_value = Write(arg1, arg2, cmd);
        break;
    case kRead:
        ret_value = Read(arg1, data);
        break;
    case kList:
        ret_value = GetFileList(data);
        break;
    default:
        last_error_ = "Undefined command!";
        break;
    }

    if (ret_value == -1)
        SendData(last_error_, client_addr);
    else
    {
        data.empty() ? data = "0" : "";
        SendData(data, client_addr);
    }
    return 0;
}

//---------------------------------------------
//              Server commands
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
        cerr << "File creation error!" << endl;
        last_error_ = "File creation error. Such a file already exists.";
        return -1;
    }
}

int FileManager::MyDeleteFile(string filename)
{
    if (remove(string{ DEF_PATH + filename }.c_str()) != 0)
    {
        cerr << "File deletion error: " << filename << endl;
        last_error_ = "File deletion error";
        return -1;
    }
    return 0;
}

int FileManager::Read(string filename, string &file_data)
{
    fstream file(DEF_PATH + filename);
    if (!file.good())
    {
        cerr << "Error opening a file for reading!";
        last_error_ = "File opening error. There is no such file";
        return -1;
    }
    std::getline(file, file_data);
    
    file.close();
    return 0;
}

int FileManager::Write(string filename, string data, Command cmd)
{
    ofstream file;
    
    if (cmd == kWrite)
        file.open(DEF_PATH + filename, std::ios::trunc);
    else
        file.open(DEF_PATH + filename, std::ios::app);

    if (!file.good())
    {
        cerr << "Error opening a file for writing!";
        last_error_ = "File opening error. There is no such file";
        return -1;
    }

    file << data;
    file.close();
    return 0;
}

int FileManager::GetFileList(string& file_list)
{
    if (std::filesystem::exists(DEF_PATH)) 
    {
        for (const auto& entry : std::filesystem::directory_iterator(DEF_PATH)) 
        {
            if (std::filesystem::is_regular_file(entry))
                file_list += entry.path().string() + "\n";
        }
    }

    return 0;
}