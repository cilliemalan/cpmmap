#include "pch.h"
#include "cpmmap.h"

using namespace cpmmap;

TEST(Basic, Open) {
	const char text[]{ "my text here 1!" };
	std::ofstream outfile("test.txt", std::ios_base::out | std::ios_base::trunc);
	outfile.write(text, sizeof(text));
	outfile.close();

	mapped_file m("test.txt");

	EXPECT_EQ(m.size(), sizeof(text));
	EXPECT_STREQ(&m[0], text);
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

	memcpy(&m[0], text, sizeof(text));
	m.sync();

	std::ifstream file("test.txt");
	file.read(buff, sizeof(buff));

	EXPECT_STREQ(&m[0], text);
}

TEST(Basic, MultiMap) {
	const char text[]{ "my text here 9!" };
	std::ofstream outfile("test.txt", std::ios_base::out | std::ios_base::trunc);
	outfile.write(text, sizeof(text));
	outfile.close();

	mapped_file m1{ "test.txt", true, true };
	mapped_file m2{ "test.txt", true, true };

	memcpy(&m1[0], text, sizeof(text));

	EXPECT_EQ(m2.size(), m1.size());
	EXPECT_NE(&m1[0], &m2[0]);
	EXPECT_STREQ(text, &m1[0]);
	EXPECT_STREQ(text, &m2[0]);
	m1[0] = 'M';
	EXPECT_STREQ(&m1[0], &m2[0]);
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