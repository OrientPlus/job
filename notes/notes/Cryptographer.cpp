#include "cryptographer.hpp"

using namespace CryptoPP;

Cryptographer::Cryptographer()
{

}

// Шифрует данные указанным ключом с помощью алгоритма AES
string Cryptographer::EncryptData(string data, const string kKey)
{
    string encrypted_text;

    // Используем AES в режиме CBC (Cipher Block Chaining)
    AES::Encryption aesEncryption((const byte*)kKey.data(), AES::DEFAULT_KEYLENGTH);
    CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, (const byte*)kKey.data());

    // Шифрование данных
    StringSource(data, true, new StreamTransformationFilter(cbcEncryption, new Base64Encoder(new StringSink(encrypted_text))));

    return encrypted_text;
}

// Дешифрует данные указанным ключом с помощью алгоритма AES
string Cryptographer::DecryptData(string data, const string kKey)
{
    string decrypted_text;

    // Используем AES в режиме CBC (Cipher Block Chaining)
    AES::Decryption aesDecryption((const byte*)kKey.data(), AES::DEFAULT_KEYLENGTH);
    CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, (const byte*)kKey.data());

    // Дешифрование данных
    StringSource(data, true, new Base64Decoder(new StreamTransformationFilter(cbcDecryption, new StringSink(decrypted_text))));

    return decrypted_text;
}

// Генерирует пару ключей RSA протокола
// Конвертирует публичный ключ в строку
string Cryptographer::GenSessionKey()
{
    CryptoPP::AutoSeededRandomPool rng;

    private_key_.GenerateRandomWithKeySize(rng, 2048);
    public_key_ = CryptoPP::RSA::PublicKey(private_key_);

    // Конвертируем публичный ключ в строку
    CryptoPP::StringSink public_key_sink(public_key_string_);

    public_key_.Save(public_key_sink);

    return public_key_string_;
}

// Шифрует публичным ключом RSA
string Cryptographer::EncryptBySessionKey(string data)
{
    string encrypted_text;
    CryptoPP::AutoSeededRandomPool rng;

    CryptoPP::RSAES_OAEP_SHA_Encryptor e(public_key_);
    CryptoPP::StringSource(data, true, new CryptoPP::PK_EncryptorFilter(rng, e, new CryptoPP::StringSink(encrypted_text)));

    return encrypted_text;
}

// Дешифрует приватным RSA ключом
string Cryptographer::DecryptBySessionKey(string data)
{
    string decrypted_text;
    CryptoPP::AutoSeededRandomPool rng;

    CryptoPP::RSAES_OAEP_SHA_Decryptor d(private_key_);
    CryptoPP::StringSource(data, true, new CryptoPP::PK_DecryptorFilter(rng, d, new CryptoPP::StringSink(decrypted_text)));

    return decrypted_text;
}

// Шифрует данные указанным ключом и пишет их в файл
int Cryptographer::EncryptFile(const string input_filename, const string data, const string key)
{
    // Шифрование данных
    CFB_Mode<AES>::Encryption cfbEncryption((byte*)key.data(), AES::DEFAULT_KEYLENGTH, (byte*)key.data());
    string cipher_text;

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
string Cryptographer::DecryptFile(const string input_filename, const string key)
{
    // Чтение зашифрованных данных из файла
    ifstream input_file(input_filename, std::ios::binary);
    if (!input_file) {
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