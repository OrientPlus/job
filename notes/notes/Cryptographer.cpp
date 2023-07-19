#include "cryptographer.hpp"



Cryptographer::Cryptographer()
{

}

string Cryptographer::EncryptData(string data, const string kKey)
{
    string encrypted_text;

    // ���������� AES � ������ CBC (Cipher Block Chaining)
    CryptoPP::AES::Encryption aesEncryption((const CryptoPP::byte*)kKey.data(), CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Encryption cbcEncryption(aesEncryption, (const CryptoPP::byte*)kKey.data());

    // ���������� ������
    CryptoPP::StringSource(data, true,
        new CryptoPP::StreamTransformationFilter(cbcEncryption,
            new CryptoPP::Base64Encoder(
                new CryptoPP::StringSink(encrypted_text)
            )
        )
    );

    return encrypted_text;
}

string Cryptographer::DecryptData(string data, const string kKey)
{
    string decrypted_text;

    // ���������� AES � ������ CBC (Cipher Block Chaining)
    CryptoPP::AES::Decryption aesDecryption((const CryptoPP::byte*)kKey.data(), CryptoPP::AES::DEFAULT_KEYLENGTH);
    CryptoPP::CBC_Mode_ExternalCipher::Decryption cbcDecryption(aesDecryption, (const CryptoPP::byte*)kKey.data());

    // ������������ ������
    CryptoPP::StringSource(data, true,
        new CryptoPP::Base64Decoder(
            new CryptoPP::StreamTransformationFilter(cbcDecryption,
                new CryptoPP::StringSink(decrypted_text)
            )
        )
    );

    return decrypted_text;
}

// ���������� ���� ������ RSA ���������
// ������������ ��������� ���� � ������
string Cryptographer::GenSessionKey()
{
    CryptoPP::AutoSeededRandomPool rng;

    private_key_.GenerateRandomWithKeySize(rng, 2048);
    public_key_ = CryptoPP::RSA::PublicKey(private_key_);

    // ������������ ��������� ���� � ������
    CryptoPP::StringSink public_key_sink(public_key_string_);

    public_key_.Save(public_key_sink);

    return public_key_string_;
}

// ������� ��������� ������ RSA
string Cryptographer::EncryptBySessionKey(string data)
{
    string encrypted_text;
    CryptoPP::AutoSeededRandomPool rng;

    CryptoPP::RSAES_OAEP_SHA_Encryptor e(public_key_);
    CryptoPP::StringSource(data, true, new CryptoPP::PK_EncryptorFilter(rng, e, new CryptoPP::StringSink(encrypted_text)));

    return encrypted_text;
}

// ��������� ��������� RSA ������
string Cryptographer::DecryptBySessionKey(string data)
{
    string decrypted_text;
    CryptoPP::AutoSeededRandomPool rng;

    CryptoPP::RSAES_OAEP_SHA_Decryptor d(private_key_);
    CryptoPP::StringSource(data, true, new CryptoPP::PK_DecryptorFilter(rng, d, new CryptoPP::StringSink(decrypted_text)));

    return decrypted_text;
}