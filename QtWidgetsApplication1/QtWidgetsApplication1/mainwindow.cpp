#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    connect(ui.LoadAllNotes, &QPushButton::clicked, this, &MainWindow::LoadAllNotes);
    connect(ui.SaveAllNotes, &QPushButton::clicked, this, &MainWindow::SaveAllNotes);
    connect(ui.actionCreate_new_note, &QAction::triggered, this, &MainWindow::CreateNewNote);
    connect(ui.listWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::OpenNote);

    port_ = "8888";
    ip_address_ = "localhost";
}

MainWindow::~MainWindow()
{}

void MainWindow::run()
{
    if (StartClient() != 0)
        Message("Error connecting to server!");

    while (true)
    {
        if (Authorization())
            break;
        else
            Message(error_message_);
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

    return auth;
}

void MainWindow::SaveAllNotes()
{
    string command = std::to_string(kSaveAll);
    command = crypt_.AesEncryptData(command, password_);
    SendData(command);

    command = RecvData();
    command = crypt_.AesDecryptData(command, password_);
    if (command != "0")
    {
        Message("Error saving notes!");
    }
    else
        Message("Successfully saved.");

    return;
}

void MainWindow::LoadAllNotes()
{

}

void MainWindow::CreateNewNote()
{
    string command = std::to_string(kCreateNew);

    // Создаем окно для ввода параметров заметки
    QWidget* window = new QWidget();
    window->setWindowTitle("Creating a note");

    // Создаем поля для ввода данных и кнопку
    QLineEdit* name_edit = new QLineEdit();
    QLineEdit* special_password_edit = new QLineEdit();
    QComboBox* type_box = new QComboBox();
    type_box->addItem(QString{ "Shared" }, QString{ "0" });
    type_box->addItem(QString{ "Def. Encrypt" }, QString{ "1" });
    type_box->addItem(QString{ "Spec. Encrypt" }, QString{ "2" });
    QPushButton* create_button = new QPushButton("Create");

    // Создаем макет и добавляем наши виджеты в него
    QFormLayout* layout = new QFormLayout();
    layout->addRow("Name:", name_edit);
    layout->addWidget(type_box);
    layout->addRow("Special password:", special_password_edit);
    layout->addWidget(create_button);

    window->setLayout(layout);

    // Кнопка "create"
    QEventLoop loop;
    QObject::connect(create_button, &QPushButton::clicked, &loop, [&]() {
        QString name = name_edit->text();
        NoteType type = static_cast<NoteType>(type_box->currentIndex());

        string note_name = name.toStdString(),
            send_command = command + " " + note_name + " " + std::to_string(type);
        if (type == kSpecialEncrypted)
        {
            send_command += " " + special_password_edit->text().toStdString();
        }

        // Шифруем данные, отправляем на сервер и получаем ответ
        string encrypted_data = crypt_.AesEncryptData(send_command, password_);
        SendData(encrypted_data);
        encrypted_data = RecvData();

        encrypted_data = crypt_.AesDecryptData(encrypted_data, password_);
        if (encrypted_data != "0")
            Message("Error creation note! Try again.");

        // Обновляем отображаемый список заметок
        UpdateNoteList();
        window->close();
        });

    window->show();
    loop.exec();
}

void MainWindow::ReadNote()
{

}

void MainWindow::WriteNote()
{

}

void MainWindow::DeleteNote()
{

}

void MainWindow::UpdateNoteList()
{
    string command = std::to_string(kGetActualNoteList);

    command = crypt_.AesEncryptData(command, password_);

    SendData(command);

    command = RecvData();
    command = crypt_.AesDecryptData(command, password_);

    // Парсим полученный список заметок
    std::vector<string> note_names;
    std::stringstream ss(command);
    while (ss >> command)
        note_names.push_back(command);

    // Добавляем записи в виджет заметок
    for(auto it : note_names)
        ui.listWidget->addItem(QString::fromStdString(it));

    ui.listWidget->update();
}

void MainWindow::OpenNote(QListWidgetItem* item)
{
    // Получаем тип заметки
    string command = std::to_string(kGetNoteTypeInfo);
    command = crypt_.AesEncryptData(command, password_);
    SendData(command);
    command = RecvData();
    string type_str = crypt_.AesDecryptData(command, password_);
    NoteType type = static_cast<NoteType>(std::stoi(type_str));

    string special_password;
    if (type == kSpecialEncrypted)
    {
        // Создаем окно для запроса специального пароля
        QWidget* window = new QWidget();
        window->setWindowTitle("Creating a note");

        // Создаем поля для ввода данных и кнопку
        QLineEdit* special_password_edit = new QLineEdit();
        special_password_edit->setEchoMode(QLineEdit::PasswordEchoOnEdit);

        QPushButton* send_button = new QPushButton("Send");

        // Создаем макет и добавляем наши виджеты в него
        QFormLayout* layout = new QFormLayout();
        layout->addRow("Special password:", special_password_edit);
        layout->addWidget(send_button);

        window->setLayout(layout);

        QEventLoop loop;
        QObject::connect(send_button, &QPushButton::clicked, &loop, [&]() {
            special_password = special_password_edit->text().toStdString();
            window->close();
            });

        window->show();
        loop.exec();
    }

    // Формируем запрос к серверу, отправляем, получаем ответ
    command = std::to_string(kRead) + " " + item->text().toStdString();
    type == kSpecialEncrypted ? command += " " + special_password : command;
    
    command = crypt_.AesEncryptData(command, password_);
    SendData(command);
    command = RecvData();
    command = crypt_.AesDecryptData(command, password_);
    if (command == "Access denied!")
    {
        Message(command);
        return;
    }

    // Если доступ разрешен, создаем окно заметки
    QWidget* note_window = new QWidget();
    note_window->resize(200, 800);
    QTextEdit* text_edit = new QTextEdit();
    QPushButton* save_button = new QPushButton("Save"),
        *clear_button = new QPushButton("Clear");

    text_edit->setPlainText(QString::fromStdString(command));
    setCentralWidget(text_edit);
    QFormLayout* layout = new QFormLayout();
    layout->addRow(text_edit);
    layout->addWidget(save_button);
    layout->addWidget(clear_button);

    note_window->setLayout(layout);

    // Кнопка "save"
    QObject::connect(save_button, &QPushButton::clicked, [&]() {
        command = std::to_string(kWrite);
        string note_data = text_edit->toPlainText().toStdString(),
            send_command = command + " " + item->text().toStdString();

        if (type == kSpecialEncrypted)
            send_command += " " + special_password;

        send_command += " " + note_data;

        // Шифруем данные, отправляем на сервер и получаем ответ
        string encrypted_data = crypt_.AesEncryptData(send_command, password_);
        SendData(encrypted_data);
        encrypted_data = RecvData();

        encrypted_data = crypt_.AesDecryptData(encrypted_data, password_);
        if (encrypted_data != "0")
            Message(encrypted_data);

        //note_window->close();
        });

    // Кнопка "clear"
    QObject::connect(save_button, &QPushButton::clicked, [&]() {
        text_edit->clear();
        });

    note_window->show();

    return;
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