#include "pch.h"
#include "cpmmap.h"


#include <iostream>
#include <fstream>
#include <string>

#include <stdio.h>


using namespace cpmmap;


TEST(Basic, Open) {
	// create a file
	const char text[]{ "my text here 1!" };
	std::ofstream outfile("test.txt", std::ios_base::out | std::ios_base::trunc);
	outfile.write(text, sizeof(text));
	outfile.close();

	// map the file
	mapped_file m("test.txt");

	// the memory is now available
	EXPECT_EQ(m.size(), sizeof(text));
	EXPECT_STREQ(m.get(), text);
}

TEST(Basic, Create) {
	::remove("test.txt");

	auto m{ mapped_file::create("test.txt", 10, true) };

	std::ifstream file("test.txt");

	file.seekg(0, std::ios_base::end);
	EXPECT_EQ(file.tellg(), 10);
}

TEST(Basic, CreateAndWrite) {
	::remove("test.txt");
	const char text[]{ "my text here 3!" };
	char buff[sizeof(text)];

	auto m{ mapped_file::create("test.txt", sizeof(text), true) };

	auto p = m.get();
	memcpy(p, text, sizeof(text));
	m.sync();

	std::ifstream file("test.txt");
	file.read(buff, sizeof(buff));

	EXPECT_STREQ(m.get(), text);
}

TEST(Basic, MultiMap) {
	const char text[]{ "my text here 9!" };
	std::ofstream outfile("test.txt", std::ios_base::out | std::ios_base::trunc);
	outfile.write(text, sizeof(text));
	outfile.close();

	mapped_file m1{ "test.txt", true, true };
	mapped_file m2{ "test.txt", true, true };

	memcpy(m1.get(), text, sizeof(text));

	EXPECT_EQ(m2.size(), m1.size());
	EXPECT_NE(m1.get(), m2.get());
	EXPECT_STREQ(text, m1.get());
	EXPECT_STREQ(text, m2.get());
	m1[0] = 'M';
	EXPECT_STREQ(m1.get(), m2.get());
}

TEST(Basic, ResizeUp) {
	::remove("test.txt");

	auto m{ mapped_file::create("test.txt", 10, true) };

	std::ifstream file("test.txt");

	file.seekg(0, std::ios_base::end);
	EXPECT_EQ(file.tellg(), 10);

	m.resize(20);

	file.seekg(0, std::ios_base::end);
	EXPECT_EQ(file.tellg(), 20);
}

TEST(Basic, ResizeDown) {
	::remove("test.txt");

	auto m{ mapped_file::create("test.txt", 20, true) };

	std::ifstream file("test.txt");

	file.seekg(0, std::ios_base::end);
	EXPECT_EQ(file.tellg(), 20);

	m.resize(10);

	file.seekg(0, std::ios_base::end);
	EXPECT_EQ(file.tellg(), 10);
}
