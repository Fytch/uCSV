/*
 *	uCSV - a small CSV parsing and exporting library
 *	Copyright (C) 2020 fytch (fytch@protonmail.com)
 *	Distributed under the MIT License.
 *	See the enclosed file LICENSE.txt for further information.
 */

#include <uCSV.hpp>
using namespace uCSV;

#include <catch2/catch.hpp>

#include <vector>
#include <array>
#include <string>

constexpr unsigned uintPow(unsigned b, unsigned e) noexcept
{
	if(e == 0 || b == 1)
		return 1;
	if(b == 0)
		return 0;

	unsigned r = 1;
	for(;;) {
		if(e & 1)
			r *= b;
		e >>= 1;
		if(e == 0)
			break;
		b *= b;
	}
	return r;
}

using row_t = std::vector<string_view_t>;

TEST_CASE("good combinatorial tests", "[uCSV][Combinatorial]")
{
	constexpr string_view_t in[] =
	{
		"",
		"a",
		" ",
		R"( !#$%&'()*+-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)",
		u8"⛱⛲⛳⛴⛵⛷⛸⛹⛺⛽",
		R"("")",
		R"("a")",
		R"(" ")",
		R"("""")",
		"\"\r\"",
		"\"\n\"",
		"\"\r\n\"",
	};
	constexpr std::size_t nIn = sizeof(in) / sizeof(*in);

	constexpr string_view_t out[] =
	{
		in[0],
		in[1],
		in[2],
		in[3],
		in[4],
		"",
		"a",
		" ",
		"\"",
		"\r",
		"\n",
		"\r\n",
	};
	constexpr std::size_t nOut = sizeof(out) / sizeof(*out);

	static_assert(nIn == nOut);
	constexpr std::size_t n = nIn;

	std::vector<row_t> table;
	constexpr std::size_t N = 3;
	table.reserve(uintPow(n, N));
	std::vector<char_t> data;
	{
		std::array<std::size_t, N> indices{};
		for(;;)
		{
			row_t row(N);
			for(std::size_t i = 0;;)
			{
				row[i] = out[indices[i]];
				data.insert(data.end(), begin(in[indices[i]]), end(in[indices[i]]));
				if(++i >= N)
					break;
				data.push_back(',');
			}
			data.push_back('\n');
			table.emplace_back(std::move(row));

			for(std::size_t i = 0;;)
			{
				if(++indices[i] >= n)
					indices[i] = 0;
				else
					break;
				if(++i >= N)
					goto done;
			}
		}
	done:;
	}

	Reader reader(data.begin(), data.end(), ErrorThrow{}, ignoreHeader);
	for(row_t const& row : table)
	{
		row_t read;
		REQUIRE(reader.fetch(read) == true);
		CHECK(read == row);
	}
}

TEST_CASE("bad combinatorial tests", "[uCSV][Combinatorial]")
{
	constexpr string_view_t endings[] =
	{
		"",
		" ",
		"a",
		"\"\"",
		"\"\r\"",
		"\"\n\"",
		"\"\r\n\"",
		"\"\"\"\"",
		"a\"\"",
		"\"\"a",
		"\",\"",
	};
	constexpr std::size_t n = sizeof(endings) / sizeof(*endings);

	constexpr std::size_t N = 3;
	std::vector<char_t> data;
	int lines = 0;
	{
		std::array<std::size_t, N> indices{};
		for(;;)
		{
			{
				const auto correct = std::to_string(lines);
				data.insert(data.end(), begin(correct), end(correct));
			}
			data.push_back('\n');
			{
				const auto incorrect = std::to_string(static_cast<int>(std::numeric_limits<int>::max() - lines));
				data.insert(data.end(), begin(incorrect), end(incorrect));
			}
			for(std::size_t i = 0; i < N; ++i)
			{
				data.push_back(',');
				data.insert(data.end(), begin(endings[indices[i]]), end(endings[indices[i]]));
			}
			data.push_back('\n');
			++lines;

			for(std::size_t i = 0;;)
			{
				if(++indices[i] >= n)
					indices[i] = 0;
				else
					break;
				if(++i >= N)
					goto done;
			}
		}
	done:;
	}

	REQUIRE(lines >= 0);
	REQUIRE(static_cast<unsigned int>(lines) == uintPow(n, N));

	Reader reader(data.begin(), data.end(), ErrorFlags{}, ignoreHeader);
	for(int i = 0; i < lines; ++i)
	{
		int read = 0;
		read = ~read;

		REQUIRE(reader.fetch(read) == true);
		REQUIRE(read == i);
		CHECK(reader.errorHandler().good() == true);

		REQUIRE(reader.fetch(read) == false);
		REQUIRE(read == i);
		CHECK(reader.errorHandler().incorrectColumns() == true);
		CHECK(reader.errorHandler().unexpectedEnd() == false);
		CHECK(reader.errorHandler().badCell() == false);
		reader.errorHandler().clear();
	}
}
