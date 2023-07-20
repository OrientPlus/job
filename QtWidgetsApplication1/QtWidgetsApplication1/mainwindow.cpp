#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    connect(ui.LoadAllNotes, &QPushButton::clicked, this, &MainWindow::LoadAllNotes);
    connect(ui.SaveAllNotes, &QPushButton::clicked, this, &MainWindow::SaveAllNotes);
}

MainWindow::~MainWindow()
{}

void MainWindow::run()
{
    /*if (StartClient() != 0)
        Message("Error connecting to server!");*/
    StartClient();

    Cryptographer crypt;

    while (true)
    {
        if (Authorization())
            break;
        /*else
            Message(error_message_);*/
    }


}

int MainWindow::StartClient()
{
    // ������������� Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "������ ��� ������������� Winsock." << endl;
        return -1;
    }

    // �������� ������
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == INVALID_SOCKET)
    {
        cout << "������ ��� �������� ������." << endl;
        WSACleanup();
        return -2;
    }

    // ���������� ����� �����
    addrinfo* result = nullptr, hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (ip_address_.empty())
        ip_address_ = "localhost";
    if (getaddrinfo(ip_address_.c_str(), port_.c_str(), &hints, &result) != 0)
    {
        cout << "������ ��� ���������� ����� �����." << endl;
        closesocket(socket_);
        WSACleanup();
        return -3;
    }

    // ����������� � �������
    if (::connect(socket_, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR)
    {
        cout << "������ ��� ����������� � �������." << endl << GetLastError() << "  " << std::system_category().message(GetLastError()) << endl;
        freeaddrinfo(result);
        closesocket(socket_);
        WSACleanup();
        return -4;
    }

    // ������������ ��������
    freeaddrinfo(result);

    return 0;
}

int MainWindow::StopClient()
{
    closesocket(socket_);
    WSACleanup();
    cout << "Client stopped." << endl;

    return 0;
}

int MainWindow::SendData(string data)
{
    auto ret = send(socket_, data.c_str(), data.size(), 0);
    if (ret == SOCKET_ERROR)
    {
        cout << "Failed to send data." << endl;
        return -1;
    }
    return ret;
}

string MainWindow::RecvData()
{
    char buffer[RECV_BUFFER_SIZE];
    int bytesReceived = recv(socket_, buffer, RECV_BUFFER_SIZE, 0);
    if (bytesReceived <= 0)
    {
        cout << "Failed to receive data." << endl;
        return string{ "Error" };
    }

    string recv{ &buffer[0], static_cast<size_t>(bytesReceived) };
    return recv;
}

bool MainWindow::Authorization()
{
    string request, answer, tmp_str, cont;

    // 2. �������� ��������� ���� ������
    public_key_ = RecvData();

    // 3. �������� �� ������� ����� � ������
    // ������� �������� ����
    QWidget* window = new QWidget();
    window->setWindowTitle("Authorization");

    // ������� ���� ��� ����� ������ � ������
    QLineEdit* login_edit = new QLineEdit();
    QLineEdit* password_edit = new QLineEdit();
    password_edit->setEchoMode(QLineEdit::Password); // �������� �������� �������
    QPushButton* send_button = new QPushButton("Send");

    // ������� ����� � ��������� ���� ������� � ����
    QFormLayout* layout = new QFormLayout();
    layout->addRow("Login:", login_edit);
    layout->addRow("Password:", password_edit);
    layout->addWidget(send_button);

    window->setLayout(layout);

    bool auth = false;
    QEventLoop loop;
    // ��������� ������ � ���������� ��������� ������
    QObject::connect(send_button, &QPushButton::clicked, &loop, [&]() {
        QString login = login_edit->text();
        QString password = password_edit->text();
        
        login_ = login.toStdString();
        password_ = password.toStdString();

        // 4. ������� ��������� ������ ������ �����������
        string encrypted_data = crypt_.RsaEncrypt(string{ login_ + " " + password_ }, public_key_);

        // 5. ���������� ������������� ������ �������
        SendData(encrypted_data);
        encrypted_data = RecvData();
        encrypted_data = crypt_.AesDecryptData(encrypted_data, password_);
        if (encrypted_data == "Authorization is successful!")
            auth = true;
        else
        {
            error_message_ = encrypted_data;
            auth = false;
        }

        window->close();
        });

    window->show();
    loop.exec();
    return true;
}

void MainWindow::SaveAllNotes()
{

}

void MainWindow::LoadAllNotes()
{

}

int MainWindow::Message(string msg)
{
    // ������� ���������� ����
    QDialog dialog;
    dialog.setWindowTitle("Error message");

    // ������� label � ������������� �����
    QLabel label;
    label.setText(QString::fromStdString(msg));

    // ������� ������������ ����� � ��������� label � ����
    QVBoxLayout layout;
    layout.addWidget(&label);

    // ������������� ����� � ����
    dialog.setLayout(&layout);

    // ���������� ����
    dialog.exec();
    return 0;
}