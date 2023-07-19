#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include <cryptopp/base64.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>


using std::vector;
using std::string;
using std::endl;
using std::cout;
using std::cin;


class Cryptographer
{
public:

    // Симметричное шифрование
	string EncryptData(string data, const string kKey);
	string DecryptData(string data, const string kKey);
	
    // Ассиметричное шифрование
    string GenSessionKey();
	string DecryptBySessionKey(string data);
    string EncryptBySessionKey(string data);

	string public_key_string_;

private:
	CryptoPP::RSA::PrivateKey private_key_;
	CryptoPP::RSA::PublicKey public_key_;
};