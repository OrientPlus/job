#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>

#include <nlohmann/json.hpp>
#include "cryptographer.hpp"


#define ACCESS_RIGHTS_FILE_PATH "access_rights.json"
#define USERS_DATA_FILE_PATH "users_data.json"

using json = nlohmann::json;
using std::vector;
using std::string;
using std::endl;
using std::cout;
using std::cin;
using std::set;
using std::fstream;
using std::stringstream;


enum NoteType { kShared = 0, kEncrypted, kSpecialEncrypted };

class User 
{
public:
	User() {};

	bool operator<(const User &other) const
	{
		return login_ < other.login_;
	}

	bool operator==(const User &other) const
	{
		return login_ == other.login_ and password_ == other.password_;
	}

	bool operator>(const User& other) const
	{
		return login_ > other.login_;
	}
	string login_, password_;
};


class Note
{
public:
	Note() { 
		type_ = kShared;
	};

	string name_, path_, owner_name_, data_;
	int type_;

	bool operator<(const Note& other) const
	{
		return data_.size() < other.data_.size();
	}

	bool operator==(const Note& other) const 
	{
		return name_ == other.name_;
	}

	bool operator>(const Note& other) const
	{
		return data_.size() > other.data_.size();
	}
};


class AccessRights
{
public:
	AccessRights();

private:
	int InitializationRights();
	bool CheckRights(User user, Note _note);
	bool SetRights(User _user, Note _note);
	int SaveAllData();
	bool CheckUser(const User user);

	
	set<Note> access_table_;
	set<User> users_table_;
	string internal_key;
};