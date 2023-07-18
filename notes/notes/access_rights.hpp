#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "cryptographer.hpp"

#define ACCESS_RIGHTS_FILE_PATH "access_rights_encr.txt"

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

	string login_, password_;
};


class Note
{
public:
	Note() {};

	string name_, path_, owner_name_, data_;
	int type_;
};


class AccessRights
{
public:
	AccessRights() {
		internal_key = "ipbdfaisbfasbfih1u43p459@(&hbdf@hdbvfufbyu";
	};

private:
	int InitializationRights();
	bool CheckRights(User user, Note _note);
	bool SetRights(User _user, Note _note);
	int SaveAllNotes();

	
	set<Note> access_table_;
	string internal_key;
};