#include "sever.hpp"


int main()
{
	setlocale(LC_ALL, "rus");
	FileManager file_manager;

	file_manager.run();

	system("pause");
	return 0;
}