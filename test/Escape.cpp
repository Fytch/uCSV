/*
 *	uCSV - a small CSV parsing and exporting library
 *	Copyright (C) 2020 fytch (fytch@protonmail.com)
 *	Distributed under the MIT License.
 *	See the enclosed file LICENSE.txt for further information.
 */

#include <uCSV.hpp>
using namespace uCSV;

#include <catch2/catch.hpp>

TEST_CASE("needsEscaping", "[uCSV][Escape]")
{
	CHECK(needsEscaping("a") == false);
	CHECK(needsEscaping("\"") == true);
	CHECK(needsEscaping("a\"a") == true);
	CHECK(needsEscaping(",") == true);
	CHECK(needsEscaping("\r") == true);
	CHECK(needsEscaping("\n") == true);
	CHECK(needsEscaping("\r\n") == true);
}

TEST_CASE("escape", "[uCSV][Escape]")
{
	CHECK(escapeToStr("a") == "\"a\"");
	CHECK(escapeToStr("\"") == "\"\"\"\"");
	CHECK(escapeToStr("a\"a") == "\"a\"\"a\"");
	CHECK(escapeToStr(",") == "\",\"");
	CHECK(escapeToStr("\r") == "\"\r\"");
	CHECK(escapeToStr("\n") == "\"\n\"");
	CHECK(escapeToStr("\r\n") == "\"\r\n\"");
}

TEST_CASE("unescape", "[uCSV][Escape]")
{
	// TODO:
}
