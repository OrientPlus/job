#include <ioastream>

enum SomeErrors {
	kOk = 0,
	kErrorBad,
	kAnotherError
};

class MyClass
{
public:
	string local_name_;
	const int kLocalFlag = 1;
	
	LocalFunction();
}


struct SomeStruct
{
	string local_name;
}

// 1. typedef, using, enum
// 2. Статические константы
// 3. Фабричные методы
// 4. Конструкторы и операторы присваивания
// 5. Деструкторы
// 6. Остальные функции
// 7. Члены данных (статические и не статические)