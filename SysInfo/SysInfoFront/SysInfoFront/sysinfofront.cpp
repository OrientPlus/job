#include "sysinfofront.h"

SysInfoFront::SysInfoFront(QWidget *parent)
    : QMainWindow(parent)
{
    OpenSocket();
    ui.setupUi(this);
    connect(ui.disk_button, &QPushButton::clicked, this, &SysInfoFront::DisplayDiskInfo);
    connect(ui.proc_button, &QPushButton::clicked, this, &SysInfoFront::DisplayProcInfo);
    connect(ui.device_button, &QPushButton::clicked, this, &SysInfoFront::DisplayDeviceInfo);
    connect(ui.netw_button, &QPushButton::clicked, this, &SysInfoFront::DisplayNetworkInfo);
}

SysInfoFront::~SysInfoFront()
{
    CloseSocket();
}


void SysInfoFront::DisplayDiskInfo()
{
    SendData("#1");

    string json_data;
    RecvData(json_data);
    if (json_data.empty())
        return;
    json disks_info;
    disks_info = json::parse(json_data);

    QStandardItemModel model;

    // Создаем заголовки для столбцов
    model.setHorizontalHeaderLabels(QStringList() << "Имя" << "Размер" << "Свободное место");

    // Пример данных о дисках (можете добавить свои данные)
    // Пример данных о дисках (можете добавить свои данные)
    QList<QList<QStandardItem*>> data;
    for (auto disk : disks_info)
    {
        data << QList<QStandardItem*>{new QStandardItem(QString::fromStdString(disk["DISK"])), new QStandardItem(QString::fromStdString(disk["TOTAL SIZE"])), new QStandardItem(QString::fromStdString(disk["FREE SIZE"]))};
    }

    // Добавляем данные в модель
    for (const QList<QStandardItem*> &row : data) {
        model.appendRow(row);
    }


    ui.tableView->setModel(&model);
    ui.tableView->resizeColumnsToContents();
   
    ui.tableView->update();
    ui.tableView->show();
}

void SysInfoFront::DisplayProcInfo()
{

}

void SysInfoFront::DisplayNetworkInfo()
{

}

void SysInfoFront::DisplayDeviceInfo()
{

}

int SysInfoFront::OpenSocket()
{
    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data_) != 0)
    {
        return INVALID_SOCKET;
    }

    // Создание UDP сокета
    socket_ = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (socket_ == INVALID_SOCKET)
    {
        WSACleanup();
        return INVALID_SOCKET;
    }

    // Задание адреса сервера
    server_address_.sin_family = AF_INET;
    server_address_.sin_port = htons(DEFAULT_PORT);
    server_address_.sin_addr.s_addr = inet_addr("127.0.0.1");

    int res = inet_pton(AF_INET, DEFAULT_IP, &(server_address_.sin_addr));


    return 0;
}

int SysInfoFront::CloseSocket()
{
    closesocket(socket_);
    WSACleanup();

    return 0;
}

int SysInfoFront::RecvData(string& data)
{
    int server_addr_size = sizeof(server_address_), expected_size = 0, bytes_read = 0;
    char local_buffer[BUFFER_SIZE + 1];
    int bytes_to_receive, total_bytes_received = 0;

    // Getting the size of the data
    bytes_read = recvfrom(socket_, reinterpret_cast<char*>(&expected_size), sizeof(expected_size), 0, (struct sockaddr*)&server_address_, &server_addr_size);
    if (bytes_read == SOCKET_ERROR)
        return -1;
    bytes_read = 0;
    expected_size = ntohl(expected_size);

    // Getting data in blocks
    while (total_bytes_received < expected_size)
    {
        bytes_to_receive = min(expected_size - total_bytes_received, BUFFER_SIZE);
        bytes_read = recvfrom(socket_, local_buffer, bytes_to_receive, 0, (struct sockaddr*)&server_address_, &server_addr_size);
        if (bytes_read == SOCKET_ERROR)
            return -1;
        total_bytes_received += bytes_read;

        local_buffer[bytes_read] = '\0';
        data += string{ local_buffer };
    }

    // Getting the checksum of the data
    unsigned checksum;
    bytes_read = recvfrom(socket_, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0, (struct sockaddr*)&server_address_, &server_addr_size);
    if (bytes_read == SOCKET_ERROR)
        return -1;
    checksum = ntohl(checksum);

    // Check that the checksum matches
    unsigned calculated_checksum = GetCRC32(data);
    if (calculated_checksum != checksum)
        return -1;

    return 0;
}

int SysInfoFront::SendData(string data)
{
    int total_bytes_send = 0, bytes_to_sent, bytes_sent;

    // Отправляем размер данных
    bytes_to_sent = htonl(data.size());
    bytes_sent = sendto(socket_, reinterpret_cast<char*>(&bytes_to_sent), sizeof(bytes_to_sent), 0, (struct sockaddr*)&server_address_, sizeof(server_address_));
    if (bytes_sent == -1)
        qDebug() << "Error: " << QString::fromStdString(std::to_string(GetLastError()));
    // Отправляем данные блоками
    while (total_bytes_send < data.size())
    {
        bytes_to_sent = min(data.size() - total_bytes_send, BUFFER_SIZE);
        bytes_sent = sendto(socket_, data.data() + total_bytes_send, bytes_to_sent, 0, (struct sockaddr*)&server_address_, sizeof(server_address_));

        if (bytes_sent == SOCKET_ERROR)
            return -1;

        total_bytes_send += bytes_sent;
    }
    // Отправляем контрольную сумму
    unsigned checksum = GetCRC32(data);
    checksum = htonl(checksum);
    bytes_sent = sendto(socket_, reinterpret_cast<char*>(&checksum), sizeof(checksum), 0, (struct sockaddr*)&server_address_, sizeof(server_address_));
    if (bytes_sent == SOCKET_ERROR)
        return -1;

    return total_bytes_send;
}

unsigned SysInfoFront::GetCRC32(string data)
{
    unsigned int crc = crc32(0L, Z_NULL, 0);

    crc = crc32(crc, reinterpret_cast<const Bytef*>(data.c_str()), data.length());

    return crc;
}