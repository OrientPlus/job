#include "cryptographer.h"

using namespace CryptoPP;

Cryptographer::Cryptographer()
{

}

// ������� ������ ��������� ������ � ������� ��������� AES
string Cryptographer::AesEncryptData(string data, const string kKey)
{
    string encrypted_text;

    // ���������� AES � ������ CBC (Cipher Block Chaining)
    AES::Encryption aesEncryption((const byte*)kKey.data(), AES::DEFAULT_KEYLENGTH);
    CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, (const byte*)kKey.data());

    // ���������� ������
    StringSource(data, true, new StreamTransformationFilter(cbcEncryption, new Base64Encoder(new StringSink(encrypted_text))));

    return encrypted_text;
}

// ��������� ������ ��������� ������ � ������� ��������� AES
string Cryptographer::AesDecryptData(string data, const string kKey)
{
    string decrypted_text;

    // ���������� AES � ������ CBC (Cipher Block Chaining)
    AES::Decryption aesDecryption((const byte*)kKey.data(), AES::DEFAULT_KEYLENGTH);
    CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, (const byte*)kKey.data());

    // ������������ ������
    StringSource(data, true, new Base64Decoder(new StreamTransformationFilter(cbcDecryption, new StringSink(decrypted_text))));

    return decrypted_text;
}

// ���������� ���� ������ RSA ���������
// ������������ ��������� ���� � ������
string Cryptographer::GenRsaKey()
{
    CryptoPP::AutoSeededRandomPool rng;

    private_key_.GenerateRandomWithKeySize(rng, 2048);
    public_key_ = CryptoPP::RSA::PublicKey(private_key_);

    // ������������ ��������� ���� � ������
    CryptoPP::StringSink public_key_sink(rsa_public_key_string_);

    public_key_.Save(public_key_sink);

    return rsa_public_key_string_;
}

// ������� ��������� ������ RSA
string Cryptographer::RsaEncrypt(string data, string key)
{
    // 1. �������� ��������� ����� �� ������
    ByteQueue public_key_bytes;
    public_key_bytes.Put((const unsigned char*)key.data(), key.size());
    public_key_bytes.MessageEnd();

    RSA::PublicKey publicKey;
    publicKey.Load(public_key_bytes);

    // 2. ������������ ������ �������� ������
    AutoSeededRandomPool rng;
    string encrypted_text;

    RSAES_OAEP_SHA_Encryptor encryptor(publicKey);
    StringSource(encrypted_text, true, new PK_EncryptorFilter(rng, encryptor, new StringSink(encrypted_text)));

    return encrypted_text;
}

// ��������� ��������� RSA ������
string Cryptographer::RsaDecrypt(string data)
{
    string decrypted_text;
    CryptoPP::AutoSeededRandomPool rng;

    CryptoPP::RSAES_OAEP_SHA_Decryptor d(private_key_);
    CryptoPP::StringSource(data, true, new CryptoPP::PK_DecryptorFilter(rng, d, new CryptoPP::StringSink(decrypted_text)));

    return decrypted_text;
}

// ������� ������ ��������� ������ � ����� �� � ����
int Cryptographer::AesEncryptFile(const string input_filename, const string data, const string key)
{
    // ���������� ������
    CFB_Mode<AES>::Encryption cfbEncryption((byte*)key.data(), AES::DEFAULT_KEYLENGTH, (byte*)key.data());
    string cipher_text;

    StringSource(data, true, new StreamTransformationFilter(cfbEncryption, new StringSink(cipher_text)));

    // ���������� ������������� ������ � ����
    ofstream output_file(input_filename, std::ios::binary);
    if (!output_file) {
        std::cerr << "������ �������� ����� ��� ������: " << input_filename << endl;
        return -1;
    }

    output_file.write(cipher_text.data(), cipher_text.size());
    output_file.close();

    return 0;
}

// �������� ������ ���������� ������ �� ���������� ����� � ����� �� � ������
string Cryptographer::AesDecryptFile(const string input_filename, const string key)
{
    // ������ ������������� ������ �� �����
    ifstream input_file(input_filename, std::ios::binary);
    if (!input_file)
    {
        std::cerr << "������ �������� �������������� �����: " << input_filename << std::endl;
        return "Error";
    }

    // ��������� ������� �����
    input_file.seekg(0, std::ios::end);
    size_t file_size = input_file.tellg();
    input_file.seekg(0, std::ios::beg);

    // ������ ������������� ������ � �����
    string cipher_text;
    cipher_text.resize(file_size);
    input_file.read(reinterpret_cast<char*>(cipher_text.data()), file_size);
    input_file.close();

    // ������������ ������
    string decrypted_data;
    CFB_Mode<AES>::Decryption cfbDecryption((byte*)key.data(), AES::DEFAULT_KEYLENGTH, (byte*)key.data());
    StringSource(cipher_text, true, new StreamTransformationFilter(cfbDecryption, new StringSink(decrypted_data)));

    return cipher_text;
}