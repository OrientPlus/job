#include "access_rights.hpp"


AccessRights::AccessRights()
{
	internal_key = "ipPDdFa2365#bfAdFbfBih1u43p459@(&hbdf@hApCSbyu";
	InitializationRights();
}

set<Note>::iterator AccessRights::FindNote(string name)
{
	return find_if(access_table_.begin(), access_table_.end(), [&](auto fnd) {
		return fnd.name_ == name;
		});
}

string AccessRights::GetNoteList()
{
	string note_list;

	for (auto it : access_table_)
		note_list += it.name_ + "\n";
	
	return note_list;
}

int AccessRights::InitializationRights()
{
	// ��������� ������ �� json, �������� � json ������
	Cryptographer crypt;
	string json_data;
	json_data = crypt.AesDecryptFile(ACCESS_RIGHTS_FILE_PATH, internal_key);

	json source_json;

	if (json_data != "Error")
	{
		source_json = json::parse(json_data);
		// ��������� ���� �� json �������
		Note temp_note;
		for (const auto& [key, value] : source_json.items())
		{
			temp_note.name_ = key;
			temp_note.type_ = value["type"];
			temp_note.owner_name_ = value["owner"];
			temp_note.data_ = value["data"];
			temp_note.password_ = value["password"];

			access_table_.insert(temp_note);
		}
	}

	// ��������� ������ �� json, �������� � json ������
	json_data.clear();
	json_data = crypt.AesDecryptFile(USERS_DATA_FILE_PATH, internal_key);
	if (json_data != "Error")
	{
		source_json = json::parse(json_data);
		// ��������� ���� �� json �������
		User temp_user;
		for (const auto& [key, value] : source_json.items())
		{
			temp_user.login_ = key;
			temp_user.password_ = value["password"];

			users_table_.insert(temp_user);
		}
	}
	
	return 0;
}

bool AccessRights::CheckRights(Note _note, string password)
{
	auto found_value = find_if(access_table_.begin(), access_table_.end(), [&](auto curr) {
		return curr.name_ == _note.name_;
		});
	if (found_value != access_table_.end())
	{
		if (found_value->type_ != kShared)
			return found_value->password_ == password ? true : false;
		else
			return true;
	}
	else
		return false;
}

bool AccessRights::SetRights(Note _note)
{
	return access_table_.insert(_note).second;
}

bool AccessRights::DeleteRights(Note _note)
{
	access_table_.erase(_note);
	return true;
}

int AccessRights::SaveAllData()
{
	// ��������� json ������
	Cryptographer crypt;
	json source_json;
	for (auto it : users_table_)
	{
		json user_json;
		user_json["login"] = it.login_;
		user_json["password"] = it.password_;
		source_json[it.login_] = user_json;
	}

	// ������������ � ������ � �������� ����������
	string json_data = source_json.dump();
	crypt.AesEncryptFile(USERS_DATA_FILE_PATH, json_data, internal_key);

	source_json.clear();
	json_data.clear();
	// ��������� json ������
	for (auto it : access_table_)
	{
		json at_json;
		at_json["name"] = it.name_;
		at_json["type"] = std::to_string(it.type_);
		at_json["owner"] = it.owner_name_;
		at_json["data"] = it.data_;
		at_json["password"] = it.password_;
	}

	// ������������ � ������ � �������� ����������
	json_data = source_json.dump();
	crypt.AesEncryptFile(ACCESS_RIGHTS_FILE_PATH, json_data, internal_key);

	return 0;
}

int AccessRights::CheckingUserData(const User user)
{
	auto found_record = find_if(users_table_.begin(), users_table_.end(), [&](auto fnd) {
		return fnd.login_ == user.login_;
		});
	if (found_record == users_table_.end())
	{
		// ���� ������ ����� �� ����������, ������������ ���
		User new_user = user;
		users_table_.insert(new_user);
		UserIsLoggedIn(new_user);

		return 0;
	}
	else
	{
		if (!IsAuthorized(user))
			return found_record->password_ == user.password_ ? 0 : -1;
		else
			return -10;
	}
}

bool AccessRights::IsAuthorized(const User user)
{
	auto found_record = find_if(authorized_users_.begin(), authorized_users_.end(), [&](auto fnd) {
		return fnd.login_ == user.login_;
		});
	return found_record == authorized_users_.end() ? false : true;
}

int AccessRights::UserIsLoggedIn(const User user)
{
	authorized_users_.insert(user);
	return 0;
}

int AccessRights::UserIsLoggedOut(const User user)
{
	authorized_users_.erase(user);
	return 0;
}