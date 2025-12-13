#include "siren/siren.h"
#include "siren/Result.h"

#include <iostream>
#include <vector>

// Quick Testing
#include "siren/io/FileDataSource.h"
#include "siren/io/MemoryDataSource.h"

int main() {
	siren::HelloWorld();
	std::cout << std::endl;

	{
		// FileDataSource
		std::cout << "-- TESTING [FileDataSource] --" << std::endl;

		siren::FileDataSource fileTest("assets/test.txt");

		std::cout << "Is valid: " << fileTest.isValid() << std::endl;
		std::cout << "Size: " << fileTest.size() << std::endl;
		std::cout << "Cursor pos: " << fileTest.tell() << std::endl;
		if (fileTest.seek(2) != siren::ResultCode::Success) {
			std::cout << "Failed seek" << std::endl;
		}
		std::cout << "Cursor pos: " << fileTest.tell() << std::endl;
		std::vector<std::byte> fileBuffer(fileTest.size());
		std::cout << "Read " << fileTest.read(fileBuffer) << " bytes into the buffer" << std::endl;
		std::cout << "Ruffer contents: ";
		std::cout.write(reinterpret_cast<const char*>(fileBuffer.data()), fileBuffer.size());
		std::cout << std::endl << std::endl;
	}

	{
		// MemoryDataSource
		std::cout << "-- TESTING [MemoryDataSource] --" << std::endl;

		std::string text = "Testing";
		// siren::MemoryDataSource memoryTest(text.data(), text.size());
		siren::MemoryDataSource memoryTest(std::span(reinterpret_cast<std::byte*>(text.data()), text.size()));

		std::cout << "Size: " << memoryTest.size() << std::endl;
		std::cout << "Cursor pos: " << memoryTest.tell() << std::endl;
		if (memoryTest.seek(2) != siren::ResultCode::Success) {
			std::cout << "Failed seek" << std::endl;
		}
		std::cout << "Cursor pos: " << memoryTest.tell() << std::endl;
		std::vector<std::byte> memoryBuffer(memoryTest.size());
		std::cout << "Read " << memoryTest.read(memoryBuffer) << " bytes into the buffer" << std::endl;
		std::cout << "Buffer: ";
		std::cout.write(reinterpret_cast<const char*>(memoryBuffer.data()), memoryBuffer.size());
		std::cout << std::endl << std::endl;
	}

	return 0;
}