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

    SOCKADDR_IN server_address, client_address;

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

//int NotesManager::SendData(SOCKET socket, string data)
//{
//    auto ret = send(socket, data.c_str(), data.size(), 0);
//    if (ret == SOCKET_ERROR)
//    {
//        cout << "Failed to send data." << endl;
//        return -1;
//    }
//    return ret;
//}
//
//int NotesManager::RecvData(SOCKET socket, string& data)
//{
//    char buffer[RECV_BUFFER_SIZE];
//    int bytesReceived = recv(socket, buffer, RECV_BUFFER_SIZE, 0);
//    if (bytesReceived <= 0)
//    {
//        lock_guard<mutex> lock(mt_);
//        cout << "Failed to receive data." << endl;
//        return -1;
//    }
//
//    data = string{ &buffer[0], static_cast<size_t>(bytesReceived) };
//
//    return bytesReceived;
//}

int NotesManager::SendData(SOCKET socket, string data)
{
    int bytes_sent = send(socket, data.c_str(), static_cast<int>(data.length()), 0);
    if (bytes_sent == SOCKET_ERROR) 
    {
        std::cerr << "Ошибка при отправке данных: " << WSAGetLastError() << std::endl;
        return 0;
    }
    return bytes_sent;
}

int NotesManager::RecvData(SOCKET socket, string& data)
{
    char buffer[BUFFER_SIZE];

    int bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);
    if (bytes_received == SOCKET_ERROR)
    {
        std::cerr << "Ошибка при получении данных: " << WSAGetLastError() << std::endl;
        return 0;
    }
    data = string{ buffer, static_cast<size_t>(bytes_received) };

    return bytes_received;
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
            lock_guard<mutex> lock(mt_);
            cout << "Failed to accept client connection." << endl;
            closesocket(listen_socket_);
            WSACleanup();
            return -1;
        }
        {
            lock_guard<mutex> lock(mt_);
            cout << "Client connected - " << client_socket_ << endl;
        }

        // Выполняем работу в потоке
        SubmitThreadpoolWork(work_);
    }

    StopServer();
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
        int auth_value = IdentificationClient(user, current_socket);
        if (auth_value == 1)
            break;
        else if (auth_value == -1)
        {
            closesocket(current_socket);
            cout << "Client socket close";
            return -1;
        }
    }

    string requested_command;
    // Выполняем команды в цикле
    int cont = 0;
    while (cont == 0)
    {
        // Получаем и дешфируем данные от юзера
        if (RecvData(current_socket, requested_command) == -1)
        {
            lock_guard<mutex> lock(mt_);
            cout << "Due to a data acquisition error, the client flow is stopped!\nTHREAD: " << std::this_thread::get_id();
            return -1;
        }

        {
            lock_guard<mutex> lock(mt_);
            requested_command = crypt_.AesDecryptData(requested_command, user.password_);
        }
        
        // Парсим и выполняем команду
        cont = ParsCommand(user, requested_command);

        {
            lock_guard<mutex> lock(mt_);
            requested_command = crypt_.AesEncryptData(requested_command, user.password_);
        }

        if (SendData(current_socket, requested_command) == -1)
        {
            lock_guard<mutex> lock(mt_);
            cout << "Due to a data sending error, the client flow is stopped!";
            return -1;
        }
    }

    closesocket(current_socket);
    cout << "Client socket close";

    return 0;
}

int NotesManager::ParsCommand(User user, string &data)
{
    // The mutex is captured for the duration of the method!
    lock_guard<mutex> lock(mt_);

    stringstream ss(data);
    string command, arg1, arg2, key, temp_str;
    ss >> command;
    
    vector<Note>::iterator note_iterator;
    Note note;
    NoteType type, new_type;
    int switch_on = std::stoi(command);
    switch (switch_on)
    {
    // CREATE_NOTE
    case kCreateNew:
        // Парсим команду
        ss >> arg1 >> arg2;
        if (arg1.empty() or arg2.empty())
        {
            data = "Invalid argument!";
            return 0;
        }

        note_iterator = access_rights_.FindNote(arg1);
        if (note_iterator != access_rights_.access_table_.end())
        {
            data = "A note with that name already exist!";
            return 0;
        }

        type = static_cast<NoteType>(stoi(arg2));
        if (type == kSpecialEncrypted)
        {
            ss >> key;
            if (key.empty())
            {
                data = "Invalid argument!";
                return 0;
            }
        }
        else
            key = user.password_;

        // Создаем заметку
        CreateNote(arg1, type, key, user.login_);

        data = "0";
        return 0;

    // DELETE_NOTE
    case kDelete:
        ss >> arg1;
        if (arg1.empty())
        {
            data = "Invalid argument!";
            return 0;
        }
        
        note_iterator = access_rights_.FindNote(arg1);
        if (note_iterator == access_rights_.access_table_.end())
        {
            data = "A note with that name does not exist!";
            return 0;
        }

        key.clear();
        if (note_iterator->type_ == kSpecialEncrypted)
        {
            ss >> key;
            if (key.empty())
            {
                data = "Invalid special password!";
                return 0;
            }
        }
        else if (note_iterator->type_ == kEncrypted)
            key = user.password_;

        if (access_rights_.CheckRights(note_iterator, key))
        {
            DeleteNote(note_iterator);
            data = "0";
        }
        else
            data = "Access denied!";
        return 0;

    // WRITE_NOTE
    case kWrite:
        ss >> arg1;
        if (arg1.empty())
        {
            data = "Invalid argument!";
            return 0;
        }

        note_iterator = access_rights_.FindNote(arg1);
        if (note_iterator == access_rights_.access_table_.end())
        {
            data = "A note with that name does not exist!";
            return 0;
        }

        key.clear();
        if (note_iterator->type_ == kSpecialEncrypted)
        {
            ss >> key;
            if (key.empty())
            {
                data = "Invalid argument!";
                return 0;
            }
        }
        else if (note_iterator->type_ == kEncrypted)
            key = user.password_;
        
        while(ss >> temp_str)
            arg2 += " " + temp_str;

        if (access_rights_.CheckRights(note_iterator, key))
        {
            WriteNote(note_iterator, arg2);
            data = "0";
        }
        else
            data = "Access denied!";
        return 0;

    // READ_NOTE
    case kRead:
        ss >> arg1;
        if (arg1.empty())
        {
            data = "Invalid argument!";
            return 0;
        }

        note_iterator = access_rights_.FindNote(arg1);
        if (note_iterator == access_rights_.access_table_.end())
        {
            data = "A note with that name does not exist!";
            return 0;
        }

        key.clear();
        if (note_iterator->type_ == kSpecialEncrypted)
        {
            ss >> key;
            if (key.empty())
            {
                data = "Invalid argument!";
                return 0;
            }
        }
        else if (note_iterator->type_ == kEncrypted)
            key = user.password_;

        if (access_rights_.CheckRights(note_iterator, key))
            data = ReadNote(note_iterator);
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
    
    // GET_NOTE_LIST
    case kGetActualNoteList:
        data = GetNoteList();
        return 0;

    // GET_NOTE_TYPE
    case kGetNoteTypeInfo:
        ss >> arg1;
        if (arg1.empty())
        {
            data = "Invalid argument!";
            return 0;
        }
        
        data = std::to_string(GetNoteTypeInfo(arg1));
        return 0;

    // CHANGE_NOTE_TYPE
    case kChangeType:
        ss >> arg1 >> arg2;
        if (arg1.empty() or arg2.empty())
        {
            data = "Invalid parameters!";
            return 0;
        }
        type = GetNoteTypeInfo(arg1),
            new_type = static_cast<NoteType>(std::stoi(arg2));
        if (type == kSpecialEncrypted or new_type == kSpecialEncrypted)
        {
            ss >> key;
            if (key.empty())
            {
                data = "Invalid special password!";
                return 0;
            }
        }
        else
            key = user.password_;

        note_iterator = access_rights_.FindNote(arg1);
        if (note_iterator == access_rights_.access_table_.end())
        {
            data = "A note with that name does not exist!";
            return 0;
        }

        int ch_value;
        if (note_iterator->owner_name_ != user.login_)
        {
            data = "Only the owner can change the type of note!";
            return 0;
        }
        if (access_rights_.CheckRights(note_iterator, key))
            ch_value = access_rights_.ChangeNoteType(note_iterator, new_type, key);
        else
        {
            data = "Access denied!";
            return 0;
        }

        ch_value == 0 ? data = "0" : data = "Change rejected!";
        return 0;

    case kLogout:
        access_rights_.UserIsLoggedOut(user);
        return -1;

    default:
        data = "Unsupported command!";
        return 0;
    }

    return 0;
}

int NotesManager::IdentificationClient(User &user, SOCKET user_socket)
{
    // 1. Генерируем два ключа (секретный и публичный)
    {
        lock_guard<mutex> lock(mt_);
        crypt_.GenRsaKey();
    }

    // 2. Отправляем публичный ключ клиенту
    if (SendData(user_socket, crypt_.rsa_public_key_string_) == 0)
    {
        cout << "Error sending a message to the client " << user_socket << endl;
        return -1;
    }

    string data, ed_data;
    // 3. Получаем данные авторизации
    if (RecvData(user_socket, data) == 0)
    {
        cout << "Error receiving data from the client " << user_socket;
        return -1;
    }

    {
        lock_guard<mutex> lock(mt_);
        // 4. Дешифруем приватным ключом
        string decrypted_text = crypt_.RsaDecrypt(data);
        string check_disconnect = decrypted_text.substr(0, decrypted_text.find(' '));
        if (check_disconnect.size() == 1 and std::stoi(check_disconnect) == kLogout)
            return -1;

        // 5. Авторизуем/регистрируем клиента
        user.login_ = decrypted_text.substr(0, decrypted_text.find(' '));
        user.password_ = decrypted_text.substr(decrypted_text.find(' ') + 1);

        int ret_value = access_rights_.CheckingUserData(user);
        if (ret_value == -1)
        {
            SendData(user_socket, crypt_.AesEncryptData("Invalid authorization data!", user.password_));
            return 0;
        }
        else if (ret_value == -10)
        {
            SendData(user_socket, crypt_.AesEncryptData("Such a user is already logged in!", user.password_));
            return 0;
        }

        access_rights_.UserIsLoggedIn(user);
        ed_data = crypt_.AesEncryptData("Authorization is successful!", user.password_);
    }

    SendData(user_socket, ed_data);
    access_rights_.SaveUsersData();
    return 1;
}

Note NotesManager::CreateNote(string name, NoteType type, string key, string owner)
{
    Note note(name, type, key, owner);

    access_rights_.SetRights(note);

    return note;
}

int NotesManager::DeleteNote(vector<Note>::iterator note_it)
{
    access_rights_.DeleteRights(note_it);
    return 0;
}

string NotesManager::ReadNote(vector<Note>::iterator note_it)
{
    return note_it->data_;
}

int NotesManager::WriteNote(vector<Note>::iterator note_it, string data)
{
    note_it->data_ = data;
    return 0;
}

int NotesManager::SaveAllNotes()
{
    access_rights_.SaveNotesData();
    return 0;
}

string NotesManager::LoadAllNotes()
{
    //access_rights_.InitializationRights();

    return access_rights_.GetNoteList();
}

string NotesManager::GetNoteList()
{
    return access_rights_.GetNoteList();
}

NoteType NotesManager::GetNoteTypeInfo(string note_name)
{
    auto note_it = access_rights_.FindNote(note_name);
    if (note_it == access_rights_.access_table_.end())
        return kShared;
    
    return static_cast<NoteType>(note_it->type_);
}