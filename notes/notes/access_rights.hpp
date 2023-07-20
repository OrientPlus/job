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
	User(string login, string password) : login_(login), password_(password) {};

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
	User operator=(const User& other) const
	{
		User new_user;
		new_user.login_ = other.login_;
		new_user.password_ = other.password_;

		return new_user;
	}
	string login_, password_;
};


class Note
{
public:
	Note(string name, NoteType type) : name_(name), type_(type) {};
	Note(string name, NoteType type, string password) : name_(name), type_(type), password_(password) {};
	Note() { type_ = kShared; };

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
	string name_, password_, owner_name_, data_;
	int type_;
};


class AccessRights
{
public:
	AccessRights();

private:
	friend class NotesManager;
	int InitializationRights();
	bool CheckRights(Note _note, string password);
	bool SetRights(Note _note);
	bool DeleteRights(Note _note);
	int SaveAllData();
	set<Note>::iterator FindNote(string name);
	string GetNoteList();

	int CheckingUserData(const User user);
	bool IsAuthorized(const User user);
	int UserIsLoggedIn(const User user);
	int UserIsLoggedOut(const User user);

	
	set<Note> access_table_;
	set<User> authorized_users_;
	set<User> users_table_;
	string internal_key;
};