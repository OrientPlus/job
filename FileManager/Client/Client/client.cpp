#include "client.hpp"

using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::string; 
using std::stringstream;

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
    bytes_to_sent = htonl(data.length());
    bytes_sent = sendto(socket_, reinterpret_cast<char*>(&bytes_to_sent), sizeof(int), 0, (struct sockaddr*)&server_address_, sizeof(server_address_));
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

int FileManager::run()
{
    StartClient();

    string cmd;

    while (true)
    {
        cmd = ParsCommand();

        SendData(cmd);
        cmd.clear();
        RecvData(cmd);
        if (cmd != "0")
            cout << cmd << endl;
    }
}