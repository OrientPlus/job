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
    return ret;
}

int NotesManager::RecvData(SOCKET socket, string data)
{
    char buffer[RECV_BUFFER_SIZE];
    int bytesReceived = recv(socket, buffer, RECV_BUFFER_SIZE, 0);
    if (bytesReceived <= 0)
    {
        cout << "Failed to receive data." << endl;
        return -1;
    }

    data = string{ &buffer[0], static_cast<size_t>(bytesReceived) };

    return bytesReceived;
}

VOID CALLBACK threadStart(PTP_CALLBACK_INSTANCE Instance, PVOID Parameter, PTP_WORK Work)
{
    NotesManager* ptr = reinterpret_cast<NotesManager*>(Parameter);

    ptr->ExecuteCommand();
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

    workcallback_ = threadStart;
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

int NotesManager::ExecuteCommand()
{
    SOCKET current_socket = client_socket_;
    User user;

    while (true)
    {
        if (IdentificationClient(user, current_socket) == 0)
            break;
    }

    return 0;
}

bool NotesManager::IdentificationClient(User &user, SOCKET user_socket)
{
    // 1. Генерируем два ключа (секретный и публичный)
    crypt_.GenSessionKey();

    // 2. Отправляем публичный ключ клиенту
    if (SendData(user_socket, crypt_.public_key_string_) == 0)
    {
        cout << "Error sending a message to the client " << user_socket << endl;
        return -1;
    }

    string data;
    // 3. Получаем данные авторизации
    if (RecvData(user_socket, data) == 0)
    {
        cout << "Error receiving data from the client " << user_socket;
        return -1;
    }

    // 4. Дешифруем приватным ключом
    string decrypted_text = crypt_.DecryptBySessionKey(data);

    // 5. Авторизуем/регистрируем клиента
    user.login_ = decrypted_text.substr(0, decrypted_text.find(' '));
    user.password_ = decrypted_text.substr(decrypted_text.find(' ') + 1);

    auto fnd = access.checkUser(curUser);
    if (!fnd.first)
    {
        sendData("Такой пользователь уже авторизован!", client);
        return -1;
    }
    if (fnd.second.password != curUser.password)
    {
        sendData("Неверный пароль!", client);
        return -1;
    }
    curUser = fnd.second;
    curUser.sock = client;

    // Если авторизация прошла успешно - создаем провайдер для шифрования
    // и генерируем ключ симметричной криптографии на основе пароля юзера
    if (!CryptAcquireContext(&curUser.hCryptProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        cerr << "Ошибка получения провайдера симметричного шифрования: " << GetLastError() << "  " << system_category().message(GetLastError()) << endl;
        system("pause");
        return -1;
    }

    //-----------------------------------------------------------
    // Create a hash object. 
    if (!CryptCreateHash(curUser.hCryptProv, CALG_MD5, 0, 0, &curUser.hHash))
    {
        cerr << "Ошибка получения хеша ключа симметричного шифрования: " << GetLastError() << "  " << system_category().message(GetLastError()) << endl;
        system("pause");
        return -1;
    }

    //-----------------------------------------------------------
    // Hash the password. 
    if (!CryptHashData(curUser.hHash, (BYTE*)curUser.password.data(), curUser.password.size(), 0))
    {
        cerr << "Ошибка хеширования пароля для ключа симметричного шифрования: " << GetLastError() << "  " << system_category().message(GetLastError()) << endl;
        system("pause");
        return -1;
    }

    //-----------------------------------------------------------
    // Derive a session key from the hash object. 
    if (!CryptDeriveKey(curUser.hCryptProv, ENCRYPT_ALGORITHM, curUser.hHash, KEYLENGTH, &curUser.key))
    {
        cerr << "Ошибка получения провайдера симметричного шифрования: " << GetLastError() << "  " << system_category().message(GetLastError()) << endl;
        system("pause");
        return -1;
    }

    curUser.keyFl = curUser.cryptProvFl = curUser.hashFl = true;

    sendData("Authorization is successful!", client);

    return 0;
    return false;
}