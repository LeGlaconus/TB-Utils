#pragma once

#ifndef _LOGGER_HPP
#define _LOGGER_HPP

#include <string_view>
#include <iostream>

#include "Types.hpp"

class Logger
{
public:
	Logger(std::ostream& stream = std::cout);
	~Logger() = default;
	
	Logger(const Logger&) = delete;
	Logger(Logger&&) = delete;
	Logger& operator=(const Logger&) = delete;
	Logger& operator=(Logger&&) = delete;

	bool valid() const { return out.good(); }

	void use_timestamp(bool b) { using_timestamps = b; }
	void use_colors(bool b) { using_colors = b; }
	void use_automatic_newlines(bool b) { using_auto_nls = b; }

	void info(const std::string_view sv);
	void warn(const std::string_view sv);
	void error(const std::string_view sv, i8 ec = 1u);

private:
	void set_color(u8 fg);
	void reset_color();

	void print_timestamp();

private:
	bool using_timestamps;
	bool using_colors;
	bool using_auto_nls;

	std::ostream& out;
};

#endif // !_LOGGER_HPP