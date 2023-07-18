#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>


using std::vector;
using std::string;
using std::endl;
using std::cout;
using std::cin;


class Cryptographer
{
public:
	Cryptographer() {};

	string EncryptData(string data, string key);
	string DecryptData(string data, string key);
	string GenSessionKey();

	string DecryptBySessionKey(string data);

	string public_key_;

private:
	string secret_key_;
};