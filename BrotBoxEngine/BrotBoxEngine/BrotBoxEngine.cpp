// BrotBoxEngine.cpp: Definiert den Einstiegspunkt für die Konsolenanwendung.
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include "PoolAllocator.hpp"
#include "StackAllocator.hpp"
#include "Byte.hpp"

class Person {
public:
	int m_age;
	std::string m_name;
	std::string m_haircolor;

	Person(int age, std::string name, std::string haircolor)
		: m_age(age), m_name(name), m_haircolor(haircolor) {
		std::cout << "CONSTRUCTOR!" << std::endl;
	}

	~Person() {
		std::cout << "DESTRUCTOR!" << std::endl;
	}

	void print() {
		std::cout << m_age << " " << m_name << " " << m_haircolor << std::endl;
	}
};


int main() {
	bbe::StackAllocator<> personenStackAllocator;
	auto marker = personenStackAllocator.getMarker();
	Person* p1 = personenStackAllocator.allocateObject<Person>(10, -1, "Undefined Name", "Undefined Haircolor");

	bbe::PoolAllocator<Person> personenAllocator;

	for (size_t i = 0; i < 10; i++) {
		p1[i].print();
	}

	personenStackAllocator.deallocateToMarker(marker);

	Person* p2 = personenAllocator.allocate(18, "Petra", "Blond");
	Person* p3 = personenAllocator.allocate(22, "Peter", "Braun");

	p2->print();
	p3->print();

	personenAllocator.deallocate(p2);
	personenAllocator.deallocate(p3);

	system("pause");
	return 0;
}