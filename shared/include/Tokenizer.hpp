#pragma once

#ifndef _TOKENIZER_HPP
#define _TOKENIZER_HPP

#include <vector>
#include <filesystem>
#include <fstream>
#include <span>

class Tokenizer
{
public:
	struct Token
	{
		Token(const std::string& _str, std::size_t _line, std::size_t _column)
			:str(_str), line(_line), column(_column)
		{ }
		~Token() = default;

		std::string str;
		std::size_t line;
		std::size_t column;

		std::string location() const;

		std::string quoteless() const;
		std::string lowercase() const;
		std::string uppercase() const;
		
		bool is_quoted() const;
	};
	
	Tokenizer(std::filesystem::path path, const std::span<const char> splitters);
	~Tokenizer() = default;

	Tokenizer(const Tokenizer&) = delete;
	Tokenizer(Tokenizer&&) = delete;
	Tokenizer& operator=(const Tokenizer&) = delete;
	Tokenizer& operator=(Tokenizer&&) = delete;

	bool valid() const { return stream.good(); }

	void close() { stream.close(); }

	Token get_next_token();
	Token peek_next_token();

private:
	void skip_comments();
	bool is_splitter(const char& c) const;
	void next_line();
	void skip_spaces();

private:
	std::ifstream stream;
	const std::span<const char> token_splitters;

	std::size_t cur_line;
	std::size_t cur_column;
};

#endif // !_TOKENIZER_HPP