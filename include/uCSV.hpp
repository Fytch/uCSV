/*
 *	uCSV - a small CSV parsing and exporting library
 *	Copyright (C) 2020 fytch (fytch@protonmail.com)
 *	Distributed under the MIT License.
 *	See the enclosed file LICENSE.txt for further information.
 */

#ifndef UCSV_HPP_INCLUDED
#define UCSV_HPP_INCLUDED

#include <string>
#include <string_view>
#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <charconv>
#include <system_error>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>
#include <tuple>
#include <cassert>
#include <optional>

namespace uCSV
{
	using char_t = char;
	using string_t = std::basic_string<char_t>;
	using string_view_t = std::basic_string_view<char_t>;

	[[nodiscard]] constexpr bool isNewline(char_t x) noexcept
	{
		return x == '\n' || x == '\r';
	}

	template<typename T>
	[[nodiscard]] constexpr T const& constify(T const& x) noexcept
	{
		return x;
	}

	// we need this because curiously the STL's output iterators all have value_type void
	template<typename iterator_t>
	struct iterator_value_type
	{
		using type = typename std::iterator_traits<iterator_t>::value_type;
	};
	template<typename container_t>
	struct iterator_value_type<std::back_insert_iterator<container_t>>
	{
		using type = typename container_t::value_type;
	};
	template<typename container_t>
	struct iterator_value_type<std::front_insert_iterator<container_t>>
	{
		using type = typename container_t::value_type;
	};
	template<typename container_t>
	struct iterator_value_type<std::insert_iterator<container_t>>
	{
		using type = typename container_t::value_type;
	};
	template <typename T, typename char_t, typename traits_t>
	struct iterator_value_type<std::ostream_iterator<T, char_t, traits_t>>
	{
		using type = T;
	};
	template <typename char_t, typename traits_t>
	struct iterator_value_type<std::ostreambuf_iterator<char_t, traits_t>>
	{
		using type = char_t;
	};
	template<typename T>
	using iterator_value_t = typename iterator_value_type<T>::type;

	inline const string_t emptyString;

	class Deserializer
	{
	public:
		constexpr explicit Deserializer(std::size_t columns, string_t const* header, string_view_t const* cells) noexcept
			: mColumns(columns), mHeader(header), mCells(cells)
		{
			assert(mColumns > 0);
			assert(mCells != nullptr);
		}

		[[nodiscard]] constexpr std::size_t index() const noexcept
		{
			return mIndex;
		}
		[[nodiscard]] constexpr std::size_t total() const noexcept
		{
			return mColumns;
		}
		[[nodiscard]] constexpr std::size_t remaining() const noexcept
		{
			return total() - index();
		}

		[[nodiscard]] constexpr string_t const& name() const noexcept
		{
			if(mHeader != nullptr)
				return mHeader[index()];
			else
				return emptyString;
		}
		constexpr string_view_t next()
		{
			if(mIndex >= mColumns)
				throw std::out_of_range("uCSV::Deserializer: attempting to access more columns that there are");
			return mCells[mIndex++];
		}

	private:
		std::size_t mIndex = 0;
		std::size_t mColumns;
		string_t const* mHeader;
		string_view_t const* mCells;
	};

	inline void deserialize(Deserializer& data, string_t& target)
	{
		target = data.next();
	}
	constexpr void deserialize(Deserializer& data, string_view_t& target)
	{
		target = data.next();
	}
	// TODO: write own number parser. from_chars is not implemented for floatings even in gcc9 and doesn't support adaptive bases for integrals
	inline void deserialize(Deserializer& data, int& target)
	{
		const string_view_t next = data.next();
		const auto end = next.data() + next.size();
		const auto [ptr, ec] = std::from_chars(next.data(), end, target);
		if(ptr != end)
			throw std::runtime_error("uCSV::deserialize: failed to convert string to int");
	}
	// TODO: replace strturd
	inline void deserialize(Deserializer& data, double& target)
	{
		const string_view_t next = data.next();
		// auto end = next.data() + next.size();
		char_t* end;
		target = std::strtod(next.data(), &end);
		if(end == next.data())
			throw std::runtime_error("uCSV::deserialize: failed to convert string to double");
	}
	// TODO: other containers and ranges too, also iterators
	template<typename T>
	constexpr void deserialize(Deserializer& data, std::vector<T>& target)
	{
		target.clear();
		// target.reserve(data.remaining()); // deserialize might consume more than one cell
		while(data.remaining())
		{
			T next;
			deserialize(data, next);
			target.emplace_back(std::move(next));
		}
	}

	namespace Detail
	{
		template<typename... Types, std::size_t... indices>
		constexpr void deserializeTupleHelper(Deserializer& data, std::tuple<Types...>& target, std::index_sequence<indices...>)
		{
			static_assert(sizeof...(Types) == sizeof...(indices));
			(deserialize(data, std::get<indices>(target)), ...);
		}
	}
	template<typename... Types>
	constexpr void deserialize(Deserializer& data, std::tuple<Types...>& target)
	{
		return Detail::deserializeTupleHelper(data, target, std::index_sequence_for<Types...>{});
	}

	template<typename... T>
	constexpr void deserializeMany(Deserializer& data, T&... args)
	{
		(deserialize(data, args), ...);
	}

	struct ErrorIgnore
	{
		constexpr void raiseIncorrectColumns(unsigned int provided, unsigned int expected, unsigned int row) noexcept
		{
			(void)provided; // -Wunused-argument
			(void)expected; // -Wunused-argument
			(void)row; // -Wunused-argument
		}
		constexpr void raiseUnexpectedEnd(unsigned int row) noexcept
		{
			(void)row; // -Wunused-argument
		}
		constexpr void raiseBadCell(unsigned int column, unsigned int row) noexcept
		{
			(void)row; // -Wunused-argument
			(void)column; // -Wunused-argument
		}
	};
	class ErrorFlags
	{
	public:
		constexpr void raiseIncorrectColumns(unsigned int provided, unsigned int expected, unsigned int row) noexcept
		{
			(void)provided; // -Wunused-argument
			(void)expected; // -Wunused-argument
			(void)row; // -Wunused-argument
			mIncorrectColumns = true;
		}
		constexpr void raiseUnexpectedEnd(unsigned int row) noexcept
		{
			(void)row; // -Wunused-argument
			mUnexpectedEnd = true;
		}
		constexpr void raiseBadCell(unsigned int column, unsigned int row) noexcept
		{
			(void)row; // -Wunused-argument
			(void)column; // -Wunused-argument
			mBadCell = true;
		}

		[[nodiscard]] constexpr bool incorrectColumns() const noexcept
		{
			return mIncorrectColumns;
		}
		[[nodiscard]] constexpr bool unexpectedEnd() const noexcept
		{
			return mUnexpectedEnd;
		}
		[[nodiscard]] constexpr bool badCell() const noexcept
		{
			return mBadCell;
		}

		[[nodiscard]] constexpr bool good() const noexcept
		{
			return !mIncorrectColumns
				&& !mUnexpectedEnd
				&& !mBadCell;
		}
		constexpr void clear() noexcept
		{
			mIncorrectColumns = false;
			mUnexpectedEnd = false;
			mBadCell = false;
		}

	private:
		bool mIncorrectColumns = false;
		bool mUnexpectedEnd = false;
		bool mBadCell = false;
	};
	template<typename ExceptionT = std::runtime_error>
	struct ErrorThrow
	{
		using ExceptionType = ExceptionT;

		[[noreturn]] constexpr void raiseIncorrectColumns(unsigned int provided, unsigned int expected, unsigned int row)
		{
			throw ExceptionType("uCSV: incorrect number of columns given; " + std::to_string(provided) + " provided, " + std::to_string(expected) + " expected in line " + std::to_string(row));
		}
		[[noreturn]] constexpr void raiseUnexpectedEnd(unsigned int row)
		{
			throw ExceptionType("uCSV: unexpected end in line " + std::to_string(row));
		}
		[[noreturn]] constexpr void raiseBadCell(unsigned int column, unsigned int row)
		{
			throw ExceptionType("uCSV: bad cell in column " + std::to_string(column) + " and line " + std::to_string(row));
		}
	};
	template<typename StreamT = std::ostream>
	class ErrorLog
	{
	public:
		using StreamType = StreamT;

		constexpr explicit ErrorLog(StreamType& Sink) noexcept(noexcept(&Sink))
			: mSink(&Sink)
		{
		}

		constexpr void raiseIncorrectColumns(unsigned int provided, unsigned int expected, unsigned int row) noexcept
		{
			*mSink << "uCSV: incorrect number of columns given; " << provided << " provided, " << expected << " expected in line " << row << '\n';
		}
		constexpr void raiseUnexpectedEnd(unsigned int row) noexcept
		{
			*mSink << "uCSV: unexpected end in line " << row << '\n';
		}
		constexpr void raiseBadCell(unsigned int column, unsigned int row) noexcept
		{
			*mSink << "uCSV: bad cell in column " << column << " and line " << row << '\n';
		}

	private:
		StreamType* mSink;
	};

	template<char_t... delimiters>
	struct Delimiter
	{
		static_assert(sizeof...(delimiters) >= 1, "at least one delimiter required");
		static_assert(((delimiters != '"') && ...), "the reserved character \" may not be a delimitor");
		static_assert(((delimiters != '\r') && ...), "newlines may not be a delimitor");
		static_assert(((delimiters != '\n') && ...), "newlines may not be a delimitor");

		constexpr bool operator()(char_t candidate) const noexcept
		{
			return ((candidate == delimiters) || ...);
		}
	};

	inline constexpr std::true_type readHeader;
	inline constexpr std::false_type ignoreHeader;

	template<typename InputIteratorFirstType, typename InputIteratorLastType>
	[[nodiscard]] constexpr bool needsEscaping(InputIteratorFirstType first, InputIteratorLastType last)
	{
		return std::any_of(first, last, [](char_t c) constexpr noexcept { return c == '\r' || c == '\n' || c == '"' || c == ','; });
	}
	template<typename InputIteratorFirstType, typename InputIteratorLastType, typename DelimiterMatcherT>
	[[nodiscard]] constexpr bool needsEscaping(InputIteratorFirstType first, InputIteratorLastType last, DelimiterMatcherT const& delimiterMatcher)
	{
		return std::any_of(first, last, [&delimiterMatcher](char_t c) constexpr noexcept { return c == '\r' || c == '\n' || c == '"' || delimiterMatcher(c); });
	}

	// TODO: escape
	// TODO: unescape

	template<
		typename InputIteratorBeginT,
		typename ErrorHandlerT = ErrorIgnore,
		typename DelimiterMatcherT = Delimiter<','>,
		typename InputIteratorEndT = InputIteratorBeginT
	>
	class Reader
	{
	public:
		using InputIteratorBeginType = InputIteratorBeginT;
		using InputIteratorEndType = InputIteratorEndT;
		using ErrorHandlerType = ErrorHandlerT;
		using DelimiterMatcherType = DelimiterMatcherT;
		using HeaderType = std::vector<string_t>;

		template<bool doReadHeader>
		constexpr Reader(InputIteratorBeginType begin, InputIteratorEndType end, std::bool_constant<doReadHeader>)
			: mBegin(begin), mEnd(end)
		{
			if constexpr(doReadHeader)
				readHeader();
		}
		template<bool doReadHeader>
		constexpr Reader(InputIteratorBeginType begin, InputIteratorEndType end, ErrorHandlerType errorHandler, std::bool_constant<doReadHeader>)
			: mBegin(begin), mEnd(end), mErrorHandler(std::move(errorHandler))
		{
			if constexpr(doReadHeader)
				readHeader();
		}
		template<bool doReadHeader>
		constexpr Reader(InputIteratorBeginType begin, InputIteratorEndType end, ErrorHandlerType errorHandler, DelimiterMatcherType delimiterMatcher, std::bool_constant<doReadHeader>)
			: mBegin(begin), mEnd(end), mErrorHandler(std::move(errorHandler)), mDelimiterMatcher(std::move(delimiterMatcher))
		{
			if constexpr(doReadHeader)
				readHeader();
		}

		template<bool doReadHeader>
		constexpr Reader(std::istream& stream, std::bool_constant<doReadHeader> constant)
			: Reader(std::istreambuf_iterator<char_t>(stream), std::istreambuf_iterator<char_t>(), constant)
		{
		}
		template<bool doReadHeader>
		constexpr Reader(std::istream& stream, ErrorHandlerType errorHandler, std::bool_constant<doReadHeader> constant)
			: Reader(std::istreambuf_iterator<char_t>(stream), std::istreambuf_iterator<char_t>(), std::move(errorHandler), constant)
		{
		}
		template<bool doReadHeader>
		constexpr Reader(std::istream& stream, ErrorHandlerType errorHandler, DelimiterMatcherType delimiterMatcher, std::bool_constant<doReadHeader> constant)
			: Reader(std::istreambuf_iterator<char_t>(stream), std::istreambuf_iterator<char_t>(), std::move(errorHandler), std::move(delimiterMatcher), constant)
		{
		}

		[[nodiscard]] constexpr ErrorHandlerType const& errorHandler() const noexcept
		{
			return mErrorHandler;
		}
		[[nodiscard]] constexpr ErrorHandlerType& errorHandler() noexcept
		{
			return mErrorHandler;
		}
		[[nodiscard]] constexpr DelimiterMatcherType const& delimiterMatcher() const noexcept
		{
			return mDelimiterMatcher;
		}
		[[nodiscard]] constexpr DelimiterMatcherType& delimiterMatcher() noexcept
		{
			return mDelimiterMatcher;
		}

		// returns 0 before the first sucessful fetch operation
		[[nodiscard]] constexpr std::size_t columns() const noexcept
		{
			return mColumns;
		}
		[[nodiscard]] constexpr bool hasHeader() const noexcept
		{
			return !mHeader.empty();
		}
		[[nodiscard]] constexpr string_view_t header(std::size_t index) const noexcept
		{
			assert(index < columns());
			if(hasHeader())
				return mHeader[index];
			else
				return {};
		}

		[[nodiscard]] constexpr bool done() const noexcept
		{
			return mBegin == mEnd;
		}

		// number of rows fetched thus far, including the header row (if present)
		[[nodiscard]] constexpr std::size_t rows() const noexcept
		{
			return mRows;
		}
		std::optional<Deserializer> fetch()
		{
			if(readLine())
				return Deserializer(columns(), mHeader.data(), mRowCells.data());
			else
				return std::nullopt;
		}
		template<typename RowT>
		bool fetch(RowT& row)
		{
			if(!readLine())
				return false;

			Deserializer deserializer(columns(), mHeader.data(), mRowCells.data());
			deserialize(deserializer, row);
			return true;
		}
		template<typename OutputIteratorT, typename OutputIterator2T>
		OutputIteratorT fetch(OutputIteratorT first, OutputIterator2T last)
		{
			using ValueT = iterator_value_t<OutputIteratorT>;
			for(ValueT value; mBegin != mEnd && first != last && fetch(value); *first = value, ++first);
			return first;
		}
		template<typename OutputIteratorT>
		OutputIteratorT fetchN(OutputIteratorT first, std::size_t n)
		{
			using ValueT = iterator_value_t<OutputIteratorT>;
			for(ValueT value; n-- && mBegin != mEnd && fetch(value); *first = value, ++first);
			return first;
		}
		template<typename OutputIteratorT, typename OutputIterator2T>
		OutputIteratorT fetchN(OutputIteratorT first, OutputIterator2T last, std::size_t n)
		{
			using ValueT = iterator_value_t<OutputIteratorT>;
			for(ValueT value; n-- && mBegin != mEnd && first != last && fetch(value); *first = value, ++first);
			return first;
		}
		template<typename OutputIteratorT>
		OutputIteratorT fetchAll(OutputIteratorT out)
		{
			using ValueT = iterator_value_t<OutputIteratorT>;
			for(ValueT value; mBegin != mEnd && fetch(value); *out = value, ++out);
			return out;
		}

	private:
		InputIteratorBeginType mBegin;
		InputIteratorEndType mEnd;
		ErrorHandlerType mErrorHandler;
		DelimiterMatcherType mDelimiterMatcher;

		std::size_t mRows = 0;
		std::size_t mColumns = 0;
		HeaderType mHeader;

		/*mutable*/ string_t mRow;
		/*mutable*/ std::vector<string_view_t> mRowCells;

		void readHeader()
		{
			fetch(mHeader);
		}

		// TODO: add an additional fast implementations of all this read... crap for contiguous iterators

		void skipCell(char_t& read, bool& continues)
		{
			continues = false;
			if(read == '"')
			{
				for(;;)
				{
					if(mBegin == mEnd)
						return;
					read = *mBegin++;
					if(read == '"')
					{
						if(mBegin == mEnd)
							break;
						read = *mBegin++;
						continues = mDelimiterMatcher(constify(read));
						if(continues || isNewline(read))
							break;
						else if(read != '"')
							return skipCell(read, continues);
					}
				}
			}
			else
			{
				for(;;)
				{
					continues = mDelimiterMatcher(constify(read));
					if(continues || isNewline(read))
						break;
					if(mBegin == mEnd)
						break;
					read = *mBegin++;
				}
			}
		}
		bool readCell(char_t& read, std::size_t& columns, bool& continues)
		{
			continues = false;
			if(read == '"')
			{
				for(;;)
				{
					if(mBegin == mEnd)
					{
						mErrorHandler.raiseUnexpectedEnd(constify(mRows));
						return false;
					}
					read = *mBegin++;
					if(read == '"')
					{
						if(mBegin == mEnd)
							break;
						read = *mBegin++;
						continues = mDelimiterMatcher(constify(read));
						if(continues || isNewline(read))
							break;
						else if(read != '"')
						{
							skipCell(read, continues);
							mErrorHandler.raiseBadCell(columns - 1, constify(mRows));
							return false;
						}
					}
					mRow.push_back(read);
				}
			}
			else
			{
				for(;;)
				{
					if(read == '"')
					{
						skipCell(read, continues);
						mErrorHandler.raiseBadCell(columns - 1, constify(mRows));
						return false;
					}
					continues = mDelimiterMatcher(constify(read));
					if(continues || isNewline(read))
						break;
					mRow.push_back(read);
					if(mBegin == mEnd)
						break;
					read = *mBegin++;
				}
			}
			if(continues)
			{
				const bool last = (mColumns != 0 && columns >= mColumns);
				if(!last)
					mRow.push_back(read);
				++columns;
			}
			return true;
		}
		bool readLine()
		{
			mRow.clear();
			mRowCells.clear();

			if(mBegin == mEnd)
			{
				mErrorHandler.raiseUnexpectedEnd(constify(mRows));
				return false;
			}

			std::size_t columns = 1;
			char_t read = *mBegin++;

			if(isNewline(read))
			{
				if(read == '\r' && mBegin != mEnd && *mBegin == '\n')
					++mBegin;
				mErrorHandler.raiseIncorrectColumns(0, mColumns > 0 ? mColumns : 1, constify(mRows));
				return false;
			}

			for(;;)
			{
				bool continues;
				const bool badCell = !readCell(read, columns, continues);
				if(!continues)
				{
					if(badCell)
						return false;
					else
						break;
				}

				if(badCell || (mColumns != 0 && columns > mColumns))
				{
					assert(mDelimiterMatcher(read));
					std::size_t excess = 0;
					if(mBegin == mEnd)
						excess = 1;
					else
						for(bool continues;;)
						{
							read = *mBegin++;
							skipCell(read, continues);
							++excess;
							if(!continues)
								break;
						}
					if(read == '\r' && mBegin != mEnd && *mBegin == '\n')
						++mBegin;
					const std::size_t realColumns = columns - 1 + excess;
					if(realColumns != mColumns)
						mErrorHandler.raiseIncorrectColumns(realColumns, constify(mColumns), constify(mRows));
					return false;
				}

				read = *mBegin++;
			}
			if(read == '\r' && mBegin != mEnd && *mBegin == '\n')
				++mBegin;

			// TODO: do this more cleverly. the idea is to store views to the data as it is being procesed (by readCell).
			// the hard part is that we have to also store an mRowCells pointers so that we know which ones to recompute
			// later on due to reallocation
			std::size_t last = 0;
			for(std::size_t i = 0; i < mRow.size() && mRowCells.size() < columns - 1; ++i)
			{
				if(mDelimiterMatcher(constify(mRow[i])))
				{
					mRowCells.emplace_back(mRow.data() + last, i - last);
					last = i + 1;
				}
			}
			mRowCells.emplace_back(mRow.data() + last, mRow.size() - last);

			if(mColumns == 0)
				mColumns = columns;
			else if(columns < mColumns)
			{
				mErrorHandler.raiseIncorrectColumns(constify(columns), constify(mColumns), constify(mRows));
				return false;
			}

			++mRows;
			return true;
		}
	};

	template<typename T, bool doReadHeader>
	Reader(T&&, std::bool_constant<doReadHeader>) -> Reader<std::istreambuf_iterator<uCSV::char_t>>;
	template<typename T, typename U, bool doReadHeader>
	Reader(T&&, U&&, std::bool_constant<doReadHeader>) -> Reader<
		std::conditional_t<
			std::is_base_of_v<std::istream, std::decay_t<T>>,
			std::istreambuf_iterator<uCSV::char_t>,
			std::decay_t<T>
		>,
		std::conditional_t<
			std::is_base_of_v<std::istream, std::decay_t<T>>,
			std::decay_t<U>,
			ErrorIgnore
		>,
		Delimiter<','>,
		std::conditional_t<
			std::is_base_of_v<std::istream, std::decay_t<T>>,
			std::istreambuf_iterator<uCSV::char_t>,
			std::decay_t<U>
		>
	>;
	template<typename T, typename U, typename V, bool doReadHeader>
	Reader(T&&, U&&, V&&, std::bool_constant<doReadHeader>) -> Reader<
		std::conditional_t<
			std::is_base_of_v<std::istream, std::decay_t<T>>,
			std::istreambuf_iterator<uCSV::char_t>,
			std::decay_t<T>
		>,
		std::conditional_t<
			std::is_base_of_v<std::istream, std::decay_t<T>>,
			std::decay_t<U>,
			std::decay_t<V>
		>,
		std::conditional_t<
			std::is_base_of_v<std::istream, std::decay_t<T>>,
			std::decay_t<V>,
			Delimiter<','>
		>,
		std::conditional_t<
			std::is_base_of_v<std::istream, std::decay_t<T>>,
			std::istreambuf_iterator<uCSV::char_t>,
			std::decay_t<U>
		>
	>;
}

#endif // !UCSV_HPP_INCLUDED
