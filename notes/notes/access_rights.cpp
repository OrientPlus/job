#include "access_rights.hpp"


AccessRights::AccessRights()
{
	InitializationRights();
}


int AccessRights::InitializationRights()
{
	// Дешифруем данные из json, передаем в json объект
	Cryptographer crypt;
	string json_data;
	json_data = crypt.DecryptFile(ACCESS_RIGHTS_FILE_PATH, internal_key);
	json source_json;
	source_json = json::parse(json_data);
	// Заполняем поля из json объекта
	Note temp_note;
	for (const auto& [key, value] : source_json.items())
	{
		temp_note.name_ = key;
		temp_note.type_ = value["type"];
		temp_note.owner_name_ = value["owner"];
		temp_note.data_ = value["data"];
		temp_note.path_ = value["path"];

		access_table_.insert(temp_note);
	}

	// Дешифруем данные из json, передаем в json объект
	json_data.clear();
	json_data = crypt.DecryptFile(USERS_DATA_FILE_PATH, internal_key);
	source_json = json::parse(json_data);
	// Заполняем поля из json объекта
	User temp_user;
	for (const auto& [key, value] : source_json.items())
	{
		temp_user.login_ = key;
		temp_user.password_ = value["password"];

		users_table_.insert(temp_user);
	}
	
	return 0;
}

bool AccessRights::CheckRights(User _user, Note _note)
{
	auto finded_value = find_if(access_table_.begin(), access_table_.end(), [&](auto curr) {
		return curr.name_ == _note.name_ and curr.owner_name_ == _user.login_;
		});
	return finded_value == access_table_.end() ? false : true;
}

bool AccessRights::SetRights(User _user, Note _note)
{
	return access_table_.insert(_note).second;
}

int AccessRights::SaveAllData()
{
	// Заполняем json объект
	Cryptographer crypt;
	json source_json;
	for (auto it : users_table_)
	{
		json user_json;
		user_json["login"] = it.login_;
		user_json["password"] = it.password_;
		source_json[it.login_] = user_json;
	}

	// Конвертируем в строку и передаем шифровщику
	string json_data = source_json.dump();
	crypt.EncryptFile(USERS_DATA_FILE_PATH, json_data, internal_key);

	source_json.clear();
	json_data.clear();
	// Заполняем json объект
	for (auto it : access_table_)
	{
		json at_json;
		at_json["name"] = it.name_;
		at_json["type"] = std::to_string(it.type_);
		at_json["owner"] = it.owner_name_;
		at_json["data"] = it.data_;
		at_json["path"] = it.path_;
	}

	// Конвертируем в строку и передаем шифровщику
	json_data = source_json.dump();
	crypt.EncryptFile(ACCESS_RIGHTS_FILE_PATH, json_data, internal_key);
}

bool AccessRights::CheckUser(const User user)
{
	auto found_record = find_if(users_table_.begin(), users_table_.end(), [&](auto fnd) {
		return fnd.login_ == user.login_;
		});
	if (found_record == users_table_.end())
		return false;
	else
		return found_record->password_ == user.password_ ? true : false;
}