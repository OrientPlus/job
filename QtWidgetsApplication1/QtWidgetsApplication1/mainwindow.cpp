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
    // Инициализация Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cout << "Ошибка при инициализации Winsock." << endl;
        return -1;
    }

    // Создание сокета
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == INVALID_SOCKET)
    {
        cout << "Ошибка при создании сокета." << endl;
        WSACleanup();
        return -2;
    }

    // Разрешение имени хоста
    addrinfo* result = nullptr, hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (ip_address_.empty())
        ip_address_ = "localhost";
    if (getaddrinfo(ip_address_.c_str(), port_.c_str(), &hints, &result) != 0)
    {
        cout << "Ошибка при разрешении имени хоста." << endl;
        closesocket(socket_);
        WSACleanup();
        return -3;
    }

    // Подключение к серверу
    if (::connect(socket_, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR)
    {
        cout << "Ошибка при подключении к серверу." << endl << GetLastError() << "  " << std::system_category().message(GetLastError()) << endl;
        freeaddrinfo(result);
        closesocket(socket_);
        WSACleanup();
        return -4;
    }

    // Освобождение ресурсов
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

    // 2. Получаем публичный ключ сессии
    public_key_ = RecvData();

    // 3. Получаем от клиента логин и пароль
    // Создаем основное окно
    QWidget* window = new QWidget();
    window->setWindowTitle("Authorization");

    // Создаем поля для ввода данных и кнопку
    QLineEdit* login_edit = new QLineEdit();
    QLineEdit* password_edit = new QLineEdit();
    password_edit->setEchoMode(QLineEdit::Password); // Скрываем вводимые символы
    QPushButton* send_button = new QPushButton("Send");

    // Создаем макет и добавляем наши виджеты в него
    QFormLayout* layout = new QFormLayout();
    layout->addRow("Login:", login_edit);
    layout->addRow("Password:", password_edit);
    layout->addWidget(send_button);

    window->setLayout(layout);

    bool auth = false;
    QEventLoop loop;
    // Связываем кнопку с обработкой введенных данных
    QObject::connect(send_button, &QPushButton::clicked, &loop, [&]() {
        QString login = login_edit->text();
        QString password = password_edit->text();
        
        login_ = login.toStdString();
        password_ = password.toStdString();

        // 4. Шифруем публичным ключом данные авторизации
        string encrypted_data = crypt_.RsaEncrypt(string{ login_ + " " + password_ }, public_key_);

        // 5. Отправляем зашифрованные данные серверу
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
    // Создаем диалоговое окно
    QDialog dialog;
    dialog.setWindowTitle("Error message");

    // Создаем label и устанавливаем текст
    QLabel label;
    label.setText(QString::fromStdString(msg));

    // Создаем вертикальный макет и добавляем label в него
    QVBoxLayout layout;
    layout.addWidget(&label);

    // Устанавливаем макет в окно
    dialog.setLayout(&layout);

    // Показываем окно
    dialog.exec();
    return 0;
}