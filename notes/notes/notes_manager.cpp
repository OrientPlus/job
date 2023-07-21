#include "notes_manager.hpp"

int NotesManager::StartServer()
{
    // Инициализируем слушающий сокет
    // Заоплняем поля структуры 
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Failed to initialize Winsock." << endl;
        return -1;
    }

    SOCKADDR_IN serverAddress, clientAddress;

    struct addrinfo* result = nullptr;
    struct addrinfo hints {};

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(nullptr, DEFAULT_PORT, &hints, &result) != 0) {
        cout << "Failed to get address info." << endl;
        WSACleanup();
        return -1;
    }

    listen_socket_ = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (listen_socket_ == INVALID_SOCKET) {
        cout << "Failed to create socket." << endl;
        freeaddrinfo(result);
        WSACleanup();
        return -1;
    }

    bind(listen_socket_, result->ai_addr, static_cast<int>(result->ai_addrlen));

    freeaddrinfo(result);

    if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR) {
        cout << "Failed to listen on the socket." << endl << GetLastError() << endl;
        closesocket(listen_socket_);
        WSACleanup();
        return -1;
    }

    cout << "\nWaiting for client connection...";

    return 0;
}

int NotesManager::StopServer()
{
    closesocket(client_socket_);
    closesocket(listen_socket_);
    WSACleanup();
    cout << "Server stopped." << endl;

    return 0;
}

int NotesManager::SendData(SOCKET socket, string data)
{
    auto ret = send(socket, data.c_str(), data.size(), 0);
    if (ret == SOCKET_ERROR)
    {
        cout << "Failed to send data." << endl;
        return -1;
    }
    cout << "\nSEND DATA:\n" << data << endl;
    return ret;
}

int NotesManager::RecvData(SOCKET socket, string &data)
{
    char buffer[RECV_BUFFER_SIZE];
    int bytesReceived = recv(socket, buffer, RECV_BUFFER_SIZE, 0);
    if (bytesReceived <= 0)
    {
        cout << "Failed to receive data." << endl;
        return -1;
    }

    data = string{ &buffer[0], static_cast<size_t>(bytesReceived) };

    cout << "\nRECV DATA:\n" << data << endl;

    return bytesReceived;
}

int NotesManager::run()
{
    HANDLE threadHandle;
    // Инициализируем структуры сокетов
    if (StartServer() == -1)
        return -1;


    InitializeThreadpoolEnvironment(&call_back_environ_);
    // Создаем пул потоков и устанавливаем максимальный и минимальный размер
    pool_ = CreateThreadpool(NULL);
    SetThreadpoolThreadMaximum(pool_, TH_SIZE_MAXIMUM);
    SetThreadpoolThreadMinimum(pool_, TH_SIZE_MINIMUM);

    SetThreadpoolCallbackPool(&call_back_environ_, pool_);

    // Создаем группу очистки и ассоциируем ее спулом
    cleanupgroup_ = CreateThreadpoolCleanupGroup();
    SetThreadpoolCallbackCleanupGroup(&call_back_environ_, cleanupgroup_, NULL);

    workcallback_ = ThreadStarter;
    work_ = CreateThreadpoolWork(workcallback_, this, &call_back_environ_);

    // В цикле устанавливаем соединения с клиентами и выделяем их в новый поток
    while (true)
    {
        // Устанавливаем соединение с новым клиентом
        client_socket_ = accept(listen_socket_, nullptr, nullptr);
        if (client_socket_ == INVALID_SOCKET) {
            cout << "Failed to accept client connection." << endl;
            closesocket(listen_socket_);
            WSACleanup();
            return -1;
        }

        cout << "Client connected - " << client_socket_ << endl;

        // Выполняем работу в потоке
        SubmitThreadpoolWork(work_);
    }
}

VOID NotesManager::ThreadStarter(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
    NotesManager* ptr = reinterpret_cast<NotesManager*>(Parameter);
    
    ptr->ExecuteCommand();
}

int NotesManager::ExecuteCommand()
{
    SOCKET current_socket = client_socket_;
    User user;

    // Авторизуем пользователя
    while (true)
    {
        if (IdentificationClient(user, current_socket))
            break;
    }

    string requested_command;
    // Выполняем команды в цикле
    while (true)
    {
        // Получаем и дешфируем данные от юзера
        RecvData(current_socket, requested_command);

        {
            lock_guard<mutex> lock(mt_);
            requested_command = crypt_.AesDecryptData(requested_command, user.password_);
        }
        
        // Парсим и выполняем команду
        ParsCommand(user, requested_command);

        {
            lock_guard<mutex> lock(mt_);
            requested_command = crypt_.AesEncryptData(requested_command, user.password_);
        }

        SendData(current_socket, requested_command);
    }

    cout << "\nAuth success!";

    return 0;
}

int NotesManager::ParsCommand(User user, string &data)
{
    stringstream ss(data);
    string command, arg1, arg2, key;
    ss >> command;
    
    Note note;
    NoteType nt;
    int switch_on = std::stoi(command);
    switch (switch_on)
    {
    // CREATE_NOTE
    case kCreateNew:
        // Парсим команду
        ss >> arg1 >> arg2;
        if (arg1.empty() or arg2.empty())
            return -1;

        nt = static_cast<NoteType>(stoi(arg2));
        if (nt == kSpecialEncrypted)
        {
            ss >> key;
            if (key.empty())
                return -1;
        }
        else
            key = user.password_;

        // Создаем заметку
        note = CreateNote(arg1, nt, key, user.login_);

        data = "0";
        return 0;

    // DELETE_NOTE
    case kDelete:
        ss >> arg1;
        if (arg1.empty())
            return -1;
        {
            auto note_it = access_rights_.FindNote(arg1);
            if (note_it == access_rights_.access_table_.end())
                return -1;
            note = *note_it;
        }
        key.clear();
        if (note.type_ == kSpecialEncrypted)
        {
            ss >> key;
            if (key.empty())
                return -1;
        }
        else if (note.type_ == kEncrypted)
            key = user.password_;

        if (access_rights_.CheckRights(note, key))
        {
            DeleteNote(note);
            data = "0";
        }
        else
            data = "Access denied!";
        return 0;

    // WRITE_NOTE
    case kWrite:
        ss >> arg1;
        if (arg1.empty())
            return -1;
        {
            auto note_it = access_rights_.FindNote(arg1);
            if (note_it == access_rights_.access_table_.end())
                return -1;
            note = *note_it;
        }

        key.clear();
        if (note.type_ == kSpecialEncrypted)
        {
            ss >> key;
            if (key.empty())
                return -1;
        }
        else if (note.type_ == kEncrypted)
            key = user.password_;
        
        ss >> arg2;
        if (arg2.empty())
            return -1;

        if (access_rights_.CheckRights(note, key))
        {
            WriteNote(note, arg2);
            data = "0";
        }
        else
            data = "Access denied!";
        return 0;

    // READ_NOTE
    case kRead:
        ss >> arg1;
        if (arg1.empty())
            return -1;
        {
            auto note_it = access_rights_.FindNote(arg1);
            if (note_it == access_rights_.access_table_.end())
                return -1;
            note = *note_it;
        }
        key.clear();
        if (note.type_ == kSpecialEncrypted)
        {
            ss >> key;
            if (key.empty())
                return -1;
        }
        else if (note.type_ == kEncrypted)
            key = user.password_;

        if (access_rights_.CheckRights(note, key))
            data = ReadNote(note);
        else
            data = "Access denied!";
        return 0;

    // SAVE_ALL
    case kSaveAll:
        SaveAllNotes();
        data = "0";
        return 0;

    // LOAD_ALL
    case kLoadAll:
        data = LoadAllNotes();
        return 0;
    case kGetActualNoteList:
        data = GetNoteList();
        return 0;
    case kGetNoteTypeInfo:
        ss >> arg1;
        if (arg1.empty())
            return -1;
        
        data = GetNoteTypeInfo(arg1);
        return 0;

    default:
        data = "Unsupported command!";
        return 0;
    }

    return 0;
}

bool NotesManager::IdentificationClient(User &user, SOCKET user_socket)
{
    // 1. Генерируем два ключа (секретный и публичный)
    crypt_.GenRsaKey();

    // 2. Отправляем публичный ключ клиенту
    if (SendData(user_socket, crypt_.rsa_public_key_string_) == 0)
    {
        cout << "Error sending a message to the client " << user_socket << endl;
        return false;
    }

    string data;
    // 3. Получаем данные авторизации
    if (RecvData(user_socket, data) == 0)
    {
        cout << "Error receiving data from the client " << user_socket;
        return false;
    }

    // 4. Дешифруем приватным ключом
    string decrypted_text = crypt_.RsaDecrypt(data);
    cout << "\nDecr user data = " << decrypted_text << endl;

    // 5. Авторизуем/регистрируем клиента
    user.login_ = decrypted_text.substr(0, decrypted_text.find(' '));
    user.password_ = decrypted_text.substr(decrypted_text.find(' ') + 1);

    int ret_value = access_rights_.CheckingUserData(user);
    if (ret_value == -1)
    {
        SendData(user_socket, "Invalid authorization data!");
        return false;
    }
    else if (ret_value == -10)
    {
        SendData(user_socket, "Such a user is already logged in!");
        return false;
    }

    access_rights_.UserIsLoggedIn(user);
    string ed_data = crypt_.AesEncryptData("Authorization is successful!", user.password_);
    SendData(user_socket, ed_data);

    return true;
}

Note NotesManager::CreateNote(string name, NoteType type, string key, string owner)
{
    Note note(name, type, key, owner);

    access_rights_.SetRights(note);

    return note;
}

int NotesManager::DeleteNote(Note &note)
{
    access_rights_.DeleteRights(note);
    return 0;
}

string NotesManager::ReadNote(Note &note)
{
    return note.data_;
}

int NotesManager::WriteNote(Note& note, string data)
{
    note.data_ = data;
    return 0;
}

int NotesManager::SaveAllNotes()
{
    access_rights_.SaveAllData();
    return 0;
}

string NotesManager::LoadAllNotes()
{
    access_rights_.InitializationRights();

    return access_rights_.GetNoteList();
}

string NotesManager::GetNoteList()
{
    return access_rights_.GetNoteList();
}

string NotesManager::GetNoteTypeInfo(string note_name)
{
    auto note_it = access_rights_.FindNote(note_name);
    if (note_it == access_rights_.access_table_.end())
        return string{"Note not found"};
    
    return string{ std::to_string(note_it->type_) };
}