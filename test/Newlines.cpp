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
#include <string>

TEST_CASE("Newlines", "[uCSV]")
{
	using std::begin, std::end;

	using row_t = std::vector<string_t>;
	constexpr std::size_t N = 3;
	const row_t ref[N] =
	{
		row_t{ "a", "b", "c" },
		row_t{ "d", "e", "f" },
		row_t{ "g", "h", "i" },
	};
	row_t rows[N];

	for(auto const& nl : { "\n", "\r", "\r\n" })
	{
		const std::string x = std::string("a,b,c") + nl;
		for(auto const& nl : { "\n", "\r", "\r\n" })
		{
			const std::string y = x + "d,e,f" + nl;
			for(auto const& nl : { "\n", "\r", "\r\n", "" })
			{
				const std::string z = y + "g,h,i" + nl;

				Reader reader(z.begin(), z.end(), ErrorThrow{}, ignoreHeader);
				REQUIRE(reader.fetch(begin(rows), end(rows)) == end(rows));
				for(std::size_t i = 0; i < N; ++i)
					CHECK(rows[i] == ref[i]);
			}
		}
	}
}
