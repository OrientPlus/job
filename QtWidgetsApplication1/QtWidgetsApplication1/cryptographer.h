#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

#include <cryptopp/rsa.h>
#include <cryptopp/osrng.h>
#include <cryptopp/base64.h>
#include <cryptopp/aes.h>
#include <cryptopp/modes.h>
#include <cryptopp/filters.h>


using std::fstream;
using std::ifstream;
using std::ofstream;
using std::vector;
using std::string;
using std::endl;
using std::cout;
using std::cin;


class Cryptographer
{
public:
	Cryptographer();
	// Симметричное шифрование
	string AesEncryptData(string data, const string kKey);
	string AesDecryptData(string data, const string kKey);

	int AesEncryptFile(const string input_filename, const string data, const string key);
	string AesDecryptFile(const string input_filename, const string key);

	// Ассиметричное шифрование
	string GenRsaKey();
	string RsaDecrypt(string data);
	string RsaEncrypt(string data, string key);

	string rsa_public_key_string_;

private:
	CryptoPP::RSA::PrivateKey private_key_;
	CryptoPP::RSA::PublicKey public_key_;
};