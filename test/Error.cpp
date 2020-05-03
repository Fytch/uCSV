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
#include <iterator>

using row_t = std::vector<string_view_t>;

TEST_CASE("incorrect columns", "[uCSV][Error]")
{
	SECTION("+-1")
	{
		constexpr char_t data[] = "A,B\na,b\nc,d,e\nf,g\nh\ni,j\nk,l,\nm,n";
		Reader reader(std::begin(data), std::end(data) - 1, ErrorFlags{}, readHeader);
		REQUIRE(reader.columns() == 2);

		row_t ab;
		CHECK(reader.fetch(ab) == true);
		CHECK(ab == row_t{ "a", "b" });
		CHECK(reader.errorHandler().good() == true);
		reader.errorHandler().clear();

		row_t cde;
		CHECK(reader.fetch(cde) == false);
		CHECK(cde.empty());
		CHECK(reader.errorHandler().incorrectColumns() == true);
		CHECK(reader.errorHandler().unexpectedEnd() == false);
		CHECK(reader.errorHandler().badCell() == false);
		reader.errorHandler().clear();

		row_t fg;
		CHECK(reader.fetch(fg) == true);
		CHECK(fg == row_t{ "f", "g" });
		CHECK(reader.errorHandler().good() == true);
		reader.errorHandler().clear();

		row_t h;
		CHECK(reader.fetch(h) == false);
		CHECK(h.empty());
		CHECK(reader.errorHandler().incorrectColumns() == true);
		CHECK(reader.errorHandler().unexpectedEnd() == false);
		CHECK(reader.errorHandler().badCell() == false);
		reader.errorHandler().clear();

		row_t ij;
		CHECK(reader.fetch(ij) == true);
		CHECK(ij == row_t{ "i", "j" });
		CHECK(reader.errorHandler().good() == true);
		reader.errorHandler().clear();

		row_t kl;
		CHECK(reader.fetch(kl) == false);
		CHECK(kl.empty());
		CHECK(reader.errorHandler().incorrectColumns() == true);
		CHECK(reader.errorHandler().unexpectedEnd() == false);
		CHECK(reader.errorHandler().badCell() == false);
		reader.errorHandler().clear();

		row_t mn;
		CHECK(reader.fetch(mn) == true);
		CHECK(mn == row_t{ "m", "n" });
		CHECK(reader.errorHandler().good() == true);
		reader.errorHandler().clear();

		CHECK(reader.done());
	}
	SECTION("empty lines")
	{
		for(auto const& nl : { "\n", "\r", "\r\n" })
		{
			const string_t data = string_t("A,B") + nl + "a,b" + nl + nl + "c,d";
			Reader reader(std::begin(data), std::end(data), ErrorFlags{}, readHeader);
			REQUIRE(reader.columns() == 2);

			row_t ab;
			CHECK(reader.fetch(ab) == true);
			CHECK(ab == row_t{ "a", "b" });
			CHECK(reader.errorHandler().good() == true);
			reader.errorHandler().clear();

			row_t dummy;
			CHECK(reader.fetch(dummy) == false);
			CHECK(dummy.empty());
			CHECK(reader.errorHandler().incorrectColumns() == true);
			CHECK(reader.errorHandler().unexpectedEnd() == false);
			CHECK(reader.errorHandler().badCell() == false);
			reader.errorHandler().clear();

			row_t cd;
			CHECK(reader.fetch(cd) == true);
			CHECK(cd == row_t{ "c", "d" });
			CHECK(reader.errorHandler().good() == true);
			reader.errorHandler().clear();

			CHECK(reader.done());
		}
	}
}

TEST_CASE("unexpected end", "[uCSV][Error]")
{
	SECTION("empty file")
	{
		Reader reader(static_cast<char_t const*>(nullptr), static_cast<char_t const*>(nullptr), ErrorFlags{}, ignoreHeader);

		row_t dummy;
		CHECK(reader.fetch(dummy) == false);
		CHECK(dummy.empty());
		CHECK(reader.errorHandler().incorrectColumns() == false);
		CHECK(reader.errorHandler().unexpectedEnd() == true);
		CHECK(reader.errorHandler().badCell() == false);

		CHECK(reader.done());
	}
	SECTION("empty file with header")
	{
		constexpr char_t data[] = "A\n";
		Reader reader(std::begin(data), std::end(data) - 1, ErrorFlags{}, readHeader);

		row_t dummy;
		CHECK(reader.fetch(dummy) == false);
		CHECK(dummy.empty());
		CHECK(reader.errorHandler().incorrectColumns() == false);
		CHECK(reader.errorHandler().unexpectedEnd() == true);
		CHECK(reader.errorHandler().badCell() == false);

		CHECK(reader.done());
	}
	SECTION("end in quote")
	{
		constexpr char_t data[] = "A\n\"";
		Reader reader(std::begin(data), std::end(data) - 1, ErrorFlags{}, readHeader);

		row_t dummy;
		CHECK(reader.fetch(dummy) == false);
		CHECK(dummy.empty());
		CHECK(reader.errorHandler().incorrectColumns() == false);
		CHECK(reader.errorHandler().unexpectedEnd() == true);
		CHECK(reader.errorHandler().badCell() == false);

		CHECK(reader.done());
	}
}

TEST_CASE("bad cell", "[uCSV][Error]")
{
	// TODO: implement
}
