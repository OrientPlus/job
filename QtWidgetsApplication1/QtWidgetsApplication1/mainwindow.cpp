#include "mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    connect(ui.LoadAllNotes, &QPushButton::clicked, this, &MainWindow::LoadAllNotes);
    connect(ui.SaveAllNotes, &QPushButton::clicked, this, &MainWindow::SaveAllNotes);
    connect(ui.Logout, &QPushButton::clicked, this, &MainWindow::Logout);
    connect(ui.actionCreate_new_note, &QAction::triggered, this, &MainWindow::CreateNewNote);
    connect(ui.listWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::OpenNote);
    ui.listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui.listWidget, &QListWidget::customContextMenuRequested, this, &MainWindow::ShowNoteMenu);
    connect(this, SIGNAL(closeEvent), this, SLOT(Logout));

    port_ = "8888";
    ip_address_ = "localhost";
}

MainWindow::~MainWindow() {}

int MainWindow::StartClient()
{
    // Инициализация Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        Message("Ошибка при инициализации Winsock.");
        return -1;
    }

    // Создание сокета
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ == INVALID_SOCKET)
    {
        Message("Ошибка при создании сокета.");
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
        Message("Ошибка при разрешении имени хоста.");
        closesocket(socket_);
        WSACleanup();
        return -3;
    }

    // Подключение к серверу
    if (::connect(socket_, result->ai_addr, static_cast<int>(result->ai_addrlen)) == SOCKET_ERROR)
    {
        string error = "Ошибка при подключении к серверу. " + std::system_category().message(GetLastError());
        Message(error);
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

    return 0;
}

int MainWindow::SendData(string data)
{
    int bytesSent = send(socket_, data.c_str(), static_cast<int>(data.length()), 0);
    if (bytesSent == SOCKET_ERROR) 
    {
        std::cerr << "Ошибка при отправке данных: " << WSAGetLastError() << std::endl;
        return 0;
    }
    return bytesSent;
}

string MainWindow::RecvData()
{
    char buffer[BUFFER_SIZE];

    int bytes_received = recv(socket_, buffer, BUFFER_SIZE, 0);
    if (bytes_received == SOCKET_ERROR)
    {
        std::cerr << "Ошибка при получении данных: " << WSAGetLastError() << std::endl;
        return string{"Error"};
    }
    return string{ buffer, static_cast<size_t>(bytes_received) };
}


void MainWindow::run()
{
    if (StartClient() != 0)
    {
        Message("Error connecting to server!");
        this->close();
    }

    while (true)
    {
        if (Authorization())
            break;
        else
            Message(error_message_);
    }
}

bool MainWindow::Authorization()
{
    string request, answer, tmp_str, cont;

    // 2. Получаем публичный ключ сессии
    public_key_ = RecvData();

    // 3. Получаем от клиента логин и пароль
    // Создаем основное окно
    window_ = new QWidget();
    window_->setWindowTitle("Authorization");

    // Создаем поля для ввода данных и кнопку
    line_edit1_ = new QLineEdit();
    line_edit2_ = new QLineEdit();
    line_edit2_->setEchoMode(QLineEdit::Password); // Скрываем вводимые символы
    button_ = new QPushButton("Send");

    // Создаем макет и добавляем наши виджеты в него
    layout_ = new QFormLayout();
    layout_->addRow("Login:", line_edit1_);
    layout_->addRow("Password:", line_edit2_);
    layout_->addWidget(button_);

    window_->setLayout(layout_);

    bool auth = false;
    QEventLoop loop;
    QObject::connect(button_, &QPushButton::clicked, &loop, [&]() {
        QString login = line_edit1_->text();
        QString password = line_edit2_->text();

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

        window_->close();
        loop.exit();
        });


    window_->show();
    loop.exec();


    delete line_edit1_;
    delete line_edit2_;
    delete button_;
    delete layout_;
    delete window_;

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
    string command = std::to_string(kLoadAll);
    command = crypt_.AesEncryptData(command, password_);
    SendData(command);
    command = RecvData();
    command = crypt_.AesDecryptData(command, password_);

    std::vector<string> note_names;
    std::stringstream ss(command);
    while (ss >> command)
        note_names.push_back(command);

    ui.listWidget->clear();
    for (auto it : note_names)
        ui.listWidget->addItem(QString::fromStdString(it));

    ui.listWidget->update();

}

void MainWindow::CreateNewNote()
{
    string command;

    window_ = new QWidget();
    window_->setWindowTitle("Creating a note");

    line_edit1_ = new QLineEdit();
    line_edit2_ = new QLineEdit();
    line_edit2_->setEchoMode(QLineEdit::Password);
    combo_box_ = new QComboBox();
    combo_box_->addItem(QString{ "Shared" }, QString{ "0" });
    combo_box_->addItem(QString{ "Def. Encrypt" }, QString{ "1" });
    combo_box_->addItem(QString{ "Spec. Encrypt" }, QString{ "2" });
    button_ = new QPushButton("Create");

    layout_ = new QFormLayout();
    layout_->addRow("Name:", line_edit1_);
    layout_->addWidget(combo_box_);
    layout_->addRow("Special password:", line_edit2_);
    layout_->addWidget(button_);

    window_->setLayout(layout_);

    // "create" button
    QEventLoop loop;
    connect(button_, &QPushButton::clicked, &loop, [&]() {
        command = std::to_string(kCreateNew);
        string note_name = line_edit1_->text().toStdString();
        NoteType type = static_cast<NoteType>(combo_box_->currentIndex());

        string send_command = command + " " + note_name + " " + std::to_string(type);
        if (type == kSpecialEncrypted)
        {
            send_command += " " + line_edit2_->text().toStdString();
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
        window_->close();
        loop.exit();
        });
    window_->show();
    loop.exec();

    delete line_edit1_;
    delete line_edit2_;
    delete combo_box_;
    delete layout_;
    delete window_;

    return;
}

void MainWindow::DeleteNote(string note_name)
{
    string command = std::to_string(kDelete) + " " + note_name,
        key;
    NoteType type = GetNoteTypeInfo(note_name);

    if (type == kSpecialEncrypted)
    {
        window_ = new QWidget();
        window_->setWindowTitle("Special password");

        line_edit1_ = new QLineEdit();
        line_edit1_->setEchoMode(QLineEdit::Password);
        button_ = new QPushButton("Send");

        layout_ = new QFormLayout();
        layout_->addRow("Password:", line_edit1_);
        layout_->addWidget(button_);

        window_->setLayout(layout_);
        QEventLoop loop;
        QObject::connect(button_, &QPushButton::clicked, &loop, [&]() {
            key = line_edit1_->text().toStdString();
            window_->close();
            loop.exit();
            });

        window_->show();
        loop.exec();

        if (key.empty())
            return;
        command += " " + key;

        delete line_edit1_;
        delete line_edit2_;
        delete button_;
        delete layout_;
        delete window_;
    }
    
    command = crypt_.AesEncryptData(command, password_);
    SendData(command);
    command = RecvData();
    command = crypt_.AesDecryptData(command, password_);
    if (command != "0")
        Message(command);

    UpdateNoteList();
}

void MainWindow::ChangeType(string note_name)
{
    string command = std::to_string(kChangeType) + " " + note_name,
        key;
    NoteType type = GetNoteTypeInfo(note_name);

    if (type == kSpecialEncrypted)
    {
        // Создаем основное окно
        window_ = new QWidget();
        window_->setWindowTitle("Special password");

        // Создаем поля для ввода данных и кнопку
        line_edit1_ = new QLineEdit();
        line_edit1_->setEchoMode(QLineEdit::Password); // Скрываем вводимые символы
        button_ = new QPushButton("Send");

        // Создаем макет и добавляем наши виджеты в него
        layout_ = new QFormLayout();
        layout_->addRow("Password:", line_edit1_);
        layout_->addWidget(button_);

        window_->setLayout(layout_);
        QEventLoop loop;
        // Связываем кнопку с обработкой введенных данных
        QObject::connect(button_, &QPushButton::clicked, &loop, [&]() {
            key = line_edit1_->text().toStdString();
            window_->close();
            loop.exit();
            });

        window_->show();
        loop.exec();

        delete line_edit1_;
        delete button_;
        delete layout_;
        delete window_;

        if (key.empty())
            return;
    }

    window_ = new QWidget();
    window_->setWindowTitle("Change note type");

    label_ = new QLabel(QString::fromStdString(note_name));
    line_edit1_ = new QLineEdit();
    line_edit1_->setEchoMode(QLineEdit::Password);
    combo_box_ = new QComboBox();
    combo_box_->addItem(QString{ "Shared" }, QString{ "0" });
    combo_box_->addItem(QString{ "Def. Encrypt" }, QString{ "1" });
    combo_box_->addItem(QString{ "Spec. Encrypt" }, QString{ "2" });
    button_ = new QPushButton("Send");

    layout_ = new QFormLayout();
    layout_->addWidget(label_);
    layout_->addWidget(combo_box_);
    layout_->addRow("Special password:", line_edit1_);
    layout_->addWidget(button_);
    window_->setLayout(layout_);

    // "send" button
    QEventLoop loop;
    QObject::connect(button_, &QPushButton::clicked, &loop, [&]() {
        command = std::to_string(kChangeType);
        NoteType new_type = static_cast<NoteType>(combo_box_->currentIndex());

        string send_command = command + " " + note_name + " " + std::to_string(new_type);
        if (new_type == kSpecialEncrypted)
            send_command += " " + line_edit1_->text().toStdString();
        if (type == kSpecialEncrypted)
            send_command += " " + key;

        // Шифруем данные, отправляем на сервер и получаем ответ
        string encrypted_data = crypt_.AesEncryptData(send_command, password_);
        SendData(encrypted_data);
        encrypted_data = RecvData();

        encrypted_data = crypt_.AesDecryptData(encrypted_data, password_);
        if (encrypted_data != "0")
            Message(encrypted_data);

        window_->close();
        loop.exit();
        });

    window_->show();
    loop.exec();

    delete label_;
    delete line_edit1_;
    delete combo_box_;
    delete button_;
    delete layout_;
    delete window_;
}

void MainWindow::ShowNoteMenu(const QPoint& pos)
{
    QListWidgetItem* item = ui.listWidget->itemAt(pos);
    if (!item)
        return;

    QPoint globalPos = ui.listWidget->mapToGlobal(pos);

    QMenu contextMenu;
    contextMenu.addAction("Delete note", std::bind(&MainWindow::DeleteNote, this, item->text().toStdString()));
    contextMenu.addAction("Change type", std::bind(&MainWindow::ChangeType, this, item->text().toStdString()));

    contextMenu.exec(globalPos);
}

void MainWindow::UpdateNoteList()
{
    string command = std::to_string(kGetActualNoteList);

    command = crypt_.AesEncryptData(command, password_);

    SendData(command);

    command = RecvData();
    command = crypt_.AesDecryptData(command, password_);

    std::vector<string> note_names;
    std::stringstream ss(command);
    while (ss >> command)
        note_names.push_back(command);

    ui.listWidget->clear();

    for(auto it : note_names)
        ui.listWidget->addItem(QString::fromStdString(it));

    ui.listWidget->update();
}

void MainWindow::OpenNote(QListWidgetItem* item)
{
    NoteType type = GetNoteTypeInfo(item->text().toStdString());

    string special_password;
    if (type == kSpecialEncrypted)
    {
        // Создаем окно для запроса специального пароля
        window_ = new QWidget();
        window_->setWindowTitle("Creating a note");

        // Создаем поля для ввода данных и кнопку
        line_edit1_ = new QLineEdit();
        line_edit1_->setEchoMode(QLineEdit::Password);

        button_ = new QPushButton("Send");

        // Создаем макет и добавляем наши виджеты в него
        layout_ = new QFormLayout();
        layout_->addRow("Special password:", line_edit1_);
        layout_->addWidget(button_);

        window_->setLayout(layout_);

        QEventLoop loop;
        QObject::connect(button_, &QPushButton::clicked, &loop, [&]() {
            special_password = line_edit1_->text().toStdString();
            window_->close();
            loop.exit();
            });

        window_->show();
        loop.exec();

        delete line_edit1_;
        delete button_;
        delete layout_;
        delete window_;

        if (special_password.empty())
            Message("Empty special passwod!");
    }

    string command = std::to_string(kRead) + " " + item->text().toStdString();
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

    window_ = new QWidget();
    window_->resize(300, 500);
    text_edit_ = new QTextEdit();
    button_ = new QPushButton("Write");

    text_edit_->setPlainText(QString::fromStdString(command));
    layout_ = new QFormLayout();
    layout_->addRow(text_edit_);
    layout_->addWidget(button_);

    window_->setLayout(layout_);

    // "Write" button
    QEventLoop loop;
    QObject::connect(button_, &QPushButton::clicked, &loop, [&]() {
        command = std::to_string(kWrite);
        string note_data = text_edit_->toPlainText().toStdString(),
            send_command = command + " " + item->text().toStdString();

        send_command += " " + special_password;

        send_command += " " + note_data;

        string encrypted_data = crypt_.AesEncryptData(send_command, password_);
        SendData(encrypted_data);
        encrypted_data = RecvData();

        encrypted_data = crypt_.AesDecryptData(encrypted_data, password_);
        if (encrypted_data != "0")
            Message(encrypted_data);
        window_->close();
        loop.exit();
        });

    window_->show();
    loop.exec();

    delete text_edit_;
    delete button_;
    delete layout_;
    delete window_;
    
    return;
}

NoteType MainWindow::GetNoteTypeInfo(string note_name)
{
    string command = std::to_string(kGetNoteTypeInfo);
    command += " " + note_name;
    command = crypt_.AesEncryptData(command, password_);
    SendData(command);
    command = RecvData();
    string type_str = crypt_.AesDecryptData(command, password_);

    return static_cast<NoteType>(std::stoi(type_str));
}

void MainWindow::Logout()
{
    string command = std::to_string(kLogout);
    command = crypt_.AesEncryptData(command, password_);
    SendData(command);
    StopClient();
    this->close();
    return;
}

int MainWindow::Message(string msg)
{
    QDialog dialog;
    dialog.setWindowTitle("Error message");

    QLabel label;
    label.setText(QString::fromStdString(msg));

    QVBoxLayout layout;
    layout.addWidget(&label);

    dialog.setLayout(&layout);

    dialog.exec();
    return 0;
}