#include "sysinfofront.h"

SysInfoFront::SysInfoFront(QWidget *parent)
    : QMainWindow(parent)
{
    proc_timer = new QTimer;
    netw_timer = new QTimer;
    OpenSocket();
    ui.setupUi(this);
    connect(ui.disk_button, &QPushButton::clicked, this, &SysInfoFront::DisplayDiskInfo);
    connect(ui.proc_button, &QPushButton::clicked, this, &SysInfoFront::DisplayProcInfo);
    connect(ui.device_button, &QPushButton::clicked, this, &SysInfoFront::DisplayDeviceInfo);
    connect(ui.netw_button, &QPushButton::clicked, this, &SysInfoFront::DisplayNetworkInfo);
    connect(proc_timer, &QTimer::timeout, this, &SysInfoFront::DisplayProcInfo);
    connect(netw_timer, &QTimer::timeout, this, &SysInfoFront::DisplayNetworkInfo);

    model = new QStandardItemModel;
    proxyModel = new NumericSortProxyModel;
}

SysInfoFront::~SysInfoFront()
{
    delete proc_timer;
    delete netw_timer;
    delete model;
    delete proxyModel;

    CloseSocket();
}

void SysInfoFront::DisplayDiskInfo()
{
    if (proc_timer->isActive())
        proc_timer->stop();
    if (netw_timer->isActive())
        netw_timer->stop();
    SendData("#1");

    string json_data;
    RecvData(json_data);
    if (json_data.empty())
        return;
    json disks_info;
    disks_info = json::parse(json_data);

    model->setRowCount(0);
    ui.view_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    model->setColumnCount(3);
    ui.view_table->setShowGrid(true);
    ui.view_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    model->setHorizontalHeaderLabels(QStringList() << "Disk" << "Total size, Mb" << "Free size, Mb");

    int counter = 0;
    for (auto disk : disks_info)
    {
        QList<QStandardItem*> row_items;
        row_items.append(new QStandardItem(QString::fromStdString(disk["DISK"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["TOTAL SIZE"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["FREE SIZE"])));
        model->appendRow(row_items);
    }
    ui.view_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui.view_table->setSortingEnabled(true);

    // Устанавливаем исходную модель таблицы
    proxyModel->setSourceModel(model);

    // Устанавливаем прокси-модель с переопределенной фильтрацией
    ui.view_table->setModel(proxyModel);

    // Сортируем по используемой памяти
    ui.view_table->sortByColumn(1);
    ui.view_table->show();
}

void SysInfoFront::DisplayProcInfo()
{
    if (netw_timer->isActive())
        netw_timer->stop();

    SendData("#2");

    string json_data;
    RecvData(json_data);
    if (json_data.empty())
        return;
    json disks_info;
    disks_info = json::parse(json_data);

    model->setRowCount(0);
    ui.view_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    model->setColumnCount(4);
    ui.view_table->setShowGrid(true);
    ui.view_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    model->setHorizontalHeaderLabels(QStringList() << "Id" << "Name" << "Usage memory, Kb" << "Usage private memory, Kb");

    int counter = 0;
    for (auto disk : disks_info)
    {
        QList<QStandardItem*> row_items;
        row_items.append(new QStandardItem(QString::fromStdString(disk["ID"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["NAME"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["USAGE_MEM"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["PRIVATE MEM"])));
        model->appendRow(row_items);
    }
    ui.view_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui.view_table->setSortingEnabled(true);

    // Устанавливаем исходную модель таблицы
    proxyModel->setSourceModel(model);

    // Устанавливаем прокси-модель с переопределенной фильтрацией
    ui.view_table->setModel(proxyModel);

    // Сортируем по используемой памяти
    ui.view_table->sortByColumn(2);
    ui.view_table->show();

    proc_timer->start(500);
}


void SysInfoFront::DisplayNetworkInfo()
{
    if (proc_timer->isActive())
        proc_timer->stop();
    SendData("#3");

    string json_data;
    RecvData(json_data);
    if (json_data.empty())
        return;
    json disks_info;
    disks_info = json::parse(json_data);

    model->setRowCount(0);
    ui.view_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    model->setColumnCount(6);
    ui.view_table->setShowGrid(true);
    ui.view_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    model->setHorizontalHeaderLabels(QStringList() << "Interface" << "MTU" << "MAC" << "Status" << "Sent, Kb" << "Received, Kb");

    int counter = 0;
    for (auto disk : disks_info)
    {
        QList<QStandardItem*> row_items;
        row_items.append(new QStandardItem(QString::fromStdString(disk["INTERFACE"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["MTU"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["MAC"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["STATUS"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["BYTES SENT"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["BYTES RECV"])));
        model->appendRow(row_items);
    }
    ui.view_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui.view_table->setSortingEnabled(true);

    // Устанавливаем исходную модель таблицы
    proxyModel->setSourceModel(model);

    // Устанавливаем прокси-модель с переопределенной фильтрацией
    ui.view_table->setModel(proxyModel);

    // Сортируем по используемой памяти
    ui.view_table->sortByColumn(1);
    ui.view_table->show();

    netw_timer->start(100);
}

void SysInfoFront::DisplayDeviceInfo()
{
    if (proc_timer->isActive())
        proc_timer->stop();
    if (netw_timer->isActive())
        netw_timer->stop();
    SendData("#4");

    string json_data;
    RecvData(json_data);
    if (json_data.empty())
        return;
    json disks_info;
    disks_info = json::parse(json_data);

    model->setRowCount(0);
    ui.view_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    model->setColumnCount(3);
    ui.view_table->setShowGrid(true);
    ui.view_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    model->setHorizontalHeaderLabels(QStringList() << "Name" << "Vendor id" << "Product id");

    int counter = 0;
    for (auto disk : disks_info)
    {
        QList<QStandardItem*> row_items;
        row_items.append(new QStandardItem(QString::fromStdString(disk["NAME"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["VENDOR ID"])));
        row_items.append(new QStandardItem(QString::fromStdString(disk["PRODUCT ID"])));
        model->appendRow(row_items);
    }
    ui.view_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui.view_table->setSortingEnabled(true);

    // Устанавливаем исходную модель таблицы
    proxyModel->setSourceModel(model);

    // Устанавливаем прокси-модель с переопределенной фильтрацией
    ui.view_table->setModel(proxyModel);

    // Сортируем по используемой памяти
    ui.view_table->sortByColumn(0);
    ui.view_table->show();
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