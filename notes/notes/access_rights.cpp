#include "access_rights.hpp"


AccessRights::AccessRights()
{
	InitializationRights();
}

int AccessRights::InitializationRights()
{
	Cryptographer crypt;
	string source_data;

	fstream src_data_file;
	src_data_file.open(ACCESS_RIGHTS_FILE_PATH);

	// Читаем содержимое файла в строку
	while (!src_data_file.eof())
		src_data_file >> source_data;

	// Расшифровываем содержимое
	if (source_data.empty())
		return 0;
	source_data = crypt.DecryptData(source_data, internal_key);

	// Парсим расшифрованную строку
	stringstream ss(source_data);
	string word;
	Note tmp_note;
	while (ss >> word)
	{
		// Имя Путь Тип Владелец Содержимое
		tmp_note.name_ = word;
		
		ss >> word;
		tmp_note.path_ = word;

		ss >> word;
		tmp_note.type_ = std::stoi(word, nullptr, 10);

		ss >> word;
		tmp_note.owner_name_ = word;

		string note_data;
		while (word != "~!")
		{
			ss >> word;
			note_data += word;
		}
		tmp_note.data_ = note_data;

		access_table_.insert(tmp_note);
	}

	src_data_file.close();
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

int AccessRights::SaveAllNotes()
{
	Cryptographer crypt;
	string source_data;

	fstream src_data_file;
	src_data_file.open(ACCESS_RIGHTS_FILE_PATH, std::ios::trunc);

	string note_string;
	for (auto it : access_table_)
	{
		note_string += it.name_;
		note_string += it.path_;
		note_string += std::to_string(it.type_);
		note_string += it.owner_name_;
		note_string += it.data_ + "\n";
	}
	note_string = crypt.EncryptData(note_string, internal_key);
	src_data_file << note_string;
	src_data_file.close();

	return 0;
}

bool AccessRights::CheckUser(const User user)
{
	auto found_record = find_if(users_table_.begin(), users_table_.end(), [&](auto fnd) {
		return fnd.login_ == user.login_;
		});
	if (found_record == )
}