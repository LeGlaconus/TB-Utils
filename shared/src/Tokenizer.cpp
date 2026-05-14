#include <iostream>
#include <span>
#include <format>

#include "Tokenizer.hpp"

Tokenizer::Tokenizer(std::filesystem::path path, const std::span<const char> splitters)
	:stream{ path }, token_splitters{ splitters }, cur_line(0u), cur_column(0u)
{
	if (stream.fail()) {
		std::cerr << "Couldn't open " << path.string() << " \n";
		return;
	}

	next_line();
}

bool Tokenizer::is_splitter(const char& c) const
{
	for (const auto& s : token_splitters) {
		if (s == c)
			return true;
	}

	return false;
}

void Tokenizer::next_line()
{
	cur_line++;
	cur_column = 1;
}

void Tokenizer::skip_comments()
{
	//Skip until there is a newline
	for (char c = '\0'; stream.good() && c != '\n'; c = stream.get())
	{ }
	next_line();

	skip_spaces();

	if (stream.get() == '/' && stream.peek() == '/')
		skip_comments();
	else
		stream.unget();
}

std::string Tokenizer::Token::location() const
{
	return std::format("line: {} column: {}", line, column);
}

std::string Tokenizer::Token::quoteless() const
{
	if(is_quoted())
		return str.substr(1, str.length() - 2);

	return str;
}

std::string Tokenizer::Token::lowercase() const
{
	std::string l = str;
	for (auto& c : l) {
		c = std::tolower(c);
	}

	return l;
}

std::string Tokenizer::Token::uppercase() const
{
	std::string l = str;
	for (auto& c : l) {
		c = std::toupper(c);
	}

	return l;
}

bool Tokenizer::Token::is_quoted() const
{
	if (str.starts_with("\"") || str.ends_with("\""))
		return true;
	
	return false;
}

void Tokenizer::skip_spaces()
{
	//Skip all spaces
	char c = '\0';
	for (c = stream.get(); stream.good(); c = stream.get())
	{
		if (!std::isspace(c))
			break;

		cur_column++;

		if (c == '\n')
			next_line();
	}

	stream.unget();
}

Tokenizer::Token Tokenizer::get_next_token()
{
	skip_spaces();

	//Skip comments
	if (stream.get() == '/' && stream.peek() == '/')
		skip_comments();
	else
		stream.unget();

	//Fill the token
	std::string token;
	const std::size_t tok_column = cur_column;
	const bool in_string = stream.get() == '"';
	stream.unget();

	if (in_string) {
		char c = '\0';
		while (stream.good())
		{
			c = stream.get();

			if (c == '\n')
				next_line();

			cur_column++;
			token += c;

			if (c == '\"' && token.length() > 1)
				break;
		}
	} else {
		char c = '\0';
		while (stream.good())
		{
			c = stream.get();

			if (std::isspace(c) || (is_splitter(c) && token.length() > 0) )
			{
				stream.unget();
				break;
			}

			if (c == '/' && stream.peek() == '/')
			{
				stream.unget();
				break;
			}

			cur_column++;
			token += c;

			if (is_splitter(c))
				break;
		}
	}

	return {token, cur_line, tok_column};
}

Tokenizer::Token Tokenizer::peek_next_token()
{
	const auto col = cur_column;
	const auto line = cur_line;
	const auto pos = stream.tellg();
	
	const Token tok = get_next_token();
	
	cur_column = col;
	cur_line = line;
	stream.seekg(pos);
	
	return tok;
}
