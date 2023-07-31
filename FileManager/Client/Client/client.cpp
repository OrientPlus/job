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

    // Отправляем размер данных
    bytes_to_sent = htonl(data.length());
    bytes_sent = sendto(socket_, reinterpret_cast<char*>(&bytes_to_sent), sizeof(int), 0, (struct sockaddr*)&server_address_, sizeof(server_address_));
    if (bytes_sent == SOCKET_ERROR)
    {
        cerr << "Ошибка отправки данных." << endl;
        return -1;
    }

    // Отправляем данные блоками
    while (total_bytes_send < data.size())
    {
        bytes_to_sent = min(data.size() - total_bytes_send, BUFFER_SIZE);
        bytes_sent = sendto(socket_, data.data(), data.length(), 0, (struct sockaddr*)&server_address_, sizeof(server_address_));
        if (bytes_sent == SOCKET_ERROR)
        {
            cerr << "Ошибка отправки данных." << endl;
            return -1;
        }

        total_bytes_send += bytes_sent;
    }

    return total_bytes_send;
}

int FileManager::RecvData(string &data)
{
    int client_addr_size = sizeof(server_address_), expected_size = 0, bytes_read = 0;
    char local_buffer[512];
    int bytes_to_receive, total_bytes_received = 0;

    // Получаем размер данных
    bytes_read = recvfrom(socket_, reinterpret_cast<char*>(expected_size), sizeof(expected_size), 0, (struct sockaddr*)&server_address_, &client_addr_size);
    if (bytes_read == SOCKET_ERROR)
    {
        cerr << "Ошибка приема данных." << endl;
        return -1;
    }
    bytes_read = 0;
    expected_size = ntohl(expected_size);

    // Получаем данные блоками 
    while (total_bytes_received < expected_size)
    {
        bytes_to_receive = min(expected_size - total_bytes_received, BUFFER_SIZE);
        bytes_read = recvfrom(socket_, local_buffer, bytes_to_receive, 0, (struct sockaddr*)&server_address_, &client_addr_size);

        if (bytes_read == SOCKET_ERROR)
        {
            cerr << "Ошибка приема данных." << endl;
            return -1;
        }
        total_bytes_received += bytes_read;
        data += string{ local_buffer };
    }

    return total_bytes_received;
}

string FileManager::ParsCommand()
{
    string command, filename, result, data;

    cout << "cmd: ";
    cin >> command;
    cout << "filename: ";
    cin >> filename;
    if (command == "read" or command == "Read" or command == "READ")
    {
        result = std::to_string(kRead);
        result += " " + filename;
    }
    else if (command == "write" or command == "Write" or command == "WRITE")
    {
        result = std::to_string(kWrite);
        cout << "data: ";
        cin >> data;
        result += " " + filename + " " + data;
    }
    else if (command == "create" or command == "Create" or command == "CREATE")
    {
        result = std::to_string(kCreateNew);
        result += " " + filename;
    }
    else if (command == "delete" or command == "Delete" or command == "del" or command == "DELETE")
    {
        result = std::to_string(kDelete);
        result += " " + filename;
    }


    return result;
}

int FileManager::run()
{
    StartClient();

    string cmd;

    while (true)
    {
        cmd = ParsCommand();

        SendData(cmd);
        RecvData(cmd);
        if (cmd != "ok")
            cout << cmd;
    }
}