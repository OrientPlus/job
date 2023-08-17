#include "sysinfofront.h"

SysInfoFront::SysInfoFront(QWidget* parent)
    : QMainWindow(parent)
{
    channel = grpc::CreateChannel("127.0.0.1:8080", grpc::InsecureChannelCredentials());
    stub_ = channel::ChannelService::NewStub(channel);

    utf8Codec = QTextCodec::codecForName("UTF-8");
    backend_proc = new QProcess;
    backend_proc->start("SysInfoBack.exe");
    proc_timer = new QTimer;
    netw_timer = new QTimer;
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
    backend_proc->kill();
    delete backend_proc;
    delete proc_timer;
    delete netw_timer;
    delete model;
    delete proxyModel;
}

void SysInfoFront::DisplayDiskInfo()
{
    if (proc_timer->isActive())
        proc_timer->stop();
    if (netw_timer->isActive())
        netw_timer->stop();

    string json_data;
    
    google::protobuf::Empty empty_request;
    channel::Response response;
    grpc::ClientContext context;

    grpc::Status status = stub_->DiskInfo(&context, empty_request, &response);
    if (status.ok()) {
        json_data = response.result();
    }
    else {
        json_data = "RPC failed with error: " + status.error_message();
        return;
    }

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
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["DISK"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["TOTAL SIZE"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["FREE SIZE"]).c_str())));
        model->appendRow(row_items);
    }
    ui.view_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui.view_table->setSortingEnabled(true);

    // Setting the initial model of the table
    proxyModel->setSourceModel(model);

    // Installing a proxy model with redefined filtering
    ui.view_table->setModel(proxyModel);

    // Sort by 1 column
    ui.view_table->sortByColumn(1);
    ui.view_table->show();
}

void SysInfoFront::DisplayProcInfo()
{
    if (netw_timer->isActive())
        netw_timer->stop();

    string json_data;
    google::protobuf::Empty empty_request;
    channel::Response response;
    grpc::ClientContext context;

    grpc::Status status = stub_->ProcessList(&context, empty_request, &response);
    if (status.ok()) {
        json_data = response.result();
    }
    else {
        json_data = "RPC failed with error: " + status.error_message();
        return;
    }

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
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["ID"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["NAME"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["USAGE_MEM"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["PRIVATE MEM"]).c_str())));
        model->appendRow(row_items);
    }
    ui.view_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui.view_table->setSortingEnabled(true);

    // Setting the initial model of the table
    proxyModel->setSourceModel(model);

    // Installing a proxy model with redefined filtering
    ui.view_table->setModel(proxyModel);

    // Sort by 1 column
    ui.view_table->sortByColumn(2);
    ui.view_table->show();

    proc_timer->start(500);
}

void SysInfoFront::DisplayNetworkInfo()
{
    if (proc_timer->isActive())
        proc_timer->stop();

    string json_data;
    google::protobuf::Empty empty_request;
    channel::Response response;
    grpc::ClientContext context;

    grpc::Status status = stub_->NetworkActivity(&context, empty_request, &response);
    if (status.ok()) {
        json_data = response.result();
    }
    else {
        json_data = "RPC failed with error: " + status.error_message();
        return;
    }

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
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["INTERFACE"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["MTU"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["MAC"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["STATUS"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["BYTES SENT"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["BYTES RECV"]).c_str())));
        model->appendRow(row_items);
    }
    ui.view_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui.view_table->setSortingEnabled(true);

    // Setting the initial model of the table
    proxyModel->setSourceModel(model);

    // Installing a proxy model with redefined filtering
    ui.view_table->setModel(proxyModel);

    // Sort by 1 column
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

    string json_data;
    google::protobuf::Empty empty_request;
    channel::Response response;
    grpc::ClientContext context;

    grpc::Status status = stub_->DeviceInfo(&context, empty_request, &response);
    if (status.ok()) {
        json_data = response.result();
    }
    else {
        json_data = "RPC failed with error: " + status.error_message();
        return;
    }

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
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["NAME"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["VENDOR ID"]).c_str())));
        row_items.append(new QStandardItem(utf8Codec->toUnicode(string(disk["PRODUCT ID"]).c_str())));
        model->appendRow(row_items);
    }
    ui.view_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui.view_table->setSortingEnabled(true);

    // Setting the initial model of the table
    proxyModel->setSourceModel(model);

    // Installing a proxy model with redefined filtering
    ui.view_table->setModel(proxyModel);

    // Sort by 1 column
    ui.view_table->sortByColumn(0);
    ui.view_table->show();
}
