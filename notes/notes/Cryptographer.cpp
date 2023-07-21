#include "cryptographer.hpp"

using namespace CryptoPP;

Cryptographer::Cryptographer()
{

}


// Шифрует данные указанным ключом с помощью алгоритма AES
string Cryptographer::AesEncryptData(string data, const string kKey)
{
    // Генерируем случайную соль
    AutoSeededRandomPool rng;
    byte salt[AES::BLOCKSIZE];
    rng.GenerateBlock(salt, sizeof(salt));

    // Определяем параметры для генерации ключа из пароля
    const int iterations = 10000;
    const int key_size = AES::MAX_KEYLENGTH;

    // Генерируем ключ из пароля и соли
    SecByteBlock key(key_size);
    PKCS5_PBKDF2_HMAC<SHA256> pbkdf2;
    pbkdf2.DeriveKey(key, key.size(), 0, (byte*)kKey.data(), kKey.size(), salt, sizeof(salt), iterations);

    // Шифруем данные
    string encrypted_data;
    CBC_Mode<AES>::Encryption encryption;
    encryption.SetKeyWithIV(key, key.size(), salt);
    StringSource(data, true, new StreamTransformationFilter(encryption, new StringSink(encrypted_data)));

    // Конкатенируем соль с зашифрованными данными для передачи
    string result;
    result.reserve(sizeof(salt) + encrypted_data.size());
    result.append((const char*)salt, sizeof(salt));
    result.append(encrypted_data);

    // Возвращаем зашифрованные данные в формате Base64
    string encoded_encrypted_data;
    StringSource(result, true, new Base64Encoder(new StringSink(encoded_encrypted_data)));

    return encoded_encrypted_data;
}

// Дешифрует данные указанным ключом с помощью алгоритма AES
string Cryptographer::AesDecryptData(string data, const string kKey)
{
    // Декодируем данные из Base64
    string decoded_data;
    StringSource(data, true, new Base64Decoder(new StringSink(decoded_data)));

    // Получаем соль из данных
    byte salt[AES::BLOCKSIZE];
    memcpy(salt, decoded_data.data(), sizeof(salt));

    // Определяем параметры для генерации ключа из пароля
    const int iterations = 10000;
    const int key_size = AES::MAX_KEYLENGTH;

    // Генерируем ключ из пароля и соли
    SecByteBlock key(key_size);
    PKCS5_PBKDF2_HMAC<SHA256> pbkdf2;
    pbkdf2.DeriveKey(key, key.size(), 0, (byte*)kKey.data(), kKey.size(), salt, sizeof(salt), iterations);

    // Дешифруем данные
    string decrypted_data;
    CBC_Mode<AES>::Decryption decryption;
    decryption.SetKeyWithIV(key, key.size(), salt);
    StringSource(decoded_data.substr(sizeof(salt)), true, new StreamTransformationFilter(decryption, new StringSink(decrypted_data)));

    return decrypted_data;
}

// Генерирует пару ключей RSA протокола
// Конвертирует публичный ключ в строку
string Cryptographer::GenRsaKey()
{
    CryptoPP::AutoSeededRandomPool rng;

    private_key_.GenerateRandomWithKeySize(rng, 1024);
    public_key_ = CryptoPP::RSA::PublicKey(private_key_);

    // Конвертируем публичный ключ в строку
    CryptoPP::StringSink public_key_sink(rsa_public_key_string_);

    public_key_.Save(public_key_sink);

    return rsa_public_key_string_;
}

// Шифрует публичным ключом RSA
string Cryptographer::RsaEncrypt(string data, string key)
{
    // 1. Загрузка открытого ключа из строки
    ByteQueue public_key_bytes;
    public_key_bytes.Put((const unsigned char*)key.data(), key.size());
    public_key_bytes.MessageEnd();

    RSA::PublicKey publicKey;
    publicKey.Load(public_key_bytes);

    // 2. Зашифрование строки открытым ключом
    AutoSeededRandomPool rng;
    string encrypted_text;

    RSAES_OAEP_SHA_Encryptor encryptor(publicKey);
    StringSource(data, true, new PK_EncryptorFilter(rng, encryptor, new StringSink(encrypted_text)));

    return encrypted_text;
}

// Дешифрует приватным RSA ключом
string Cryptographer::RsaDecrypt(string data)
{
    if (data.empty())
        return string{ "Empty data" };
    string decrypted_text;
    AutoSeededRandomPool rng;

    RSAES_OAEP_SHA_Decryptor d(private_key_);
    StringSource(data, true, new PK_DecryptorFilter(rng, d, new StringSink(decrypted_text)));

    return decrypted_text;
}

// Шифрует данные указанным ключом и пишет их в файл
int Cryptographer::AesEncryptFile(const string input_filename, const string data, const string key)
{
    // Шифрование данных
    CFB_Mode<AES>::Encryption cfbEncryption((byte*)key.data(), AES::DEFAULT_KEYLENGTH, (byte*)key.data());
    string cipher_text;

    if (data.empty())
        return 0;
    StringSource(data, true, new StreamTransformationFilter(cfbEncryption, new StringSink(cipher_text)));

    // Сохранение зашифрованных данных в файл
    ofstream output_file(input_filename, std::ios::binary);
    if (!output_file) {
        std::cerr << "Ошибка открытия файла для записи: " << input_filename << endl;
        return -1;
    }

    output_file.write(cipher_text.data(), cipher_text.size());
    output_file.close();

    return 0;
}

// Дешфрует данные переданным ключом из указанного файла и пишет их в строку
string Cryptographer::AesDecryptFile(const string input_filename, const string key)
{
    // Чтение зашифрованных данных из файла
    ifstream input_file(input_filename, std::ios::binary);
    if (!input_file) 
    {
        std::cerr << "Ошибка открытия зашифрованного файла: " << input_filename << std::endl;
        return "Error";
    }

    // Получение размера файла
    input_file.seekg(0, std::ios::end);
    size_t file_size = input_file.tellg();
    input_file.seekg(0, std::ios::beg);

    // Чтение зашифрованных данных в буфер
    string cipher_text;
    cipher_text.resize(file_size);
    input_file.read(reinterpret_cast<char*>(cipher_text.data()), file_size);
    input_file.close();

    // Дешифрование данных
    string decrypted_data;
    CFB_Mode<AES>::Decryption cfbDecryption((byte*)key.data(), AES::DEFAULT_KEYLENGTH, (byte*)key.data());
    StringSource(cipher_text, true, new StreamTransformationFilter(cfbDecryption, new StringSink(decrypted_data)));

    return cipher_text;
}