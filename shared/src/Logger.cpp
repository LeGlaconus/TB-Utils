#include <iostream>
#include <format>
#include <chrono>

#include "Logger.hpp"

Logger::Logger(std::ostream& stream)
	:using_timestamps(true), using_colors(true), using_auto_nls(true), out(stream)
{ }

void Logger::set_color(u8 fg)
{
	out << std::format("\x1b[{};40m", fg);
}

void Logger::reset_color()
{
	out << "\x1b[0m";
}

void Logger::print_timestamp()
{
	if (using_timestamps)
		out << "[" << (std::chrono::system_clock::now()) << "] ";
}

void Logger::info(const std::string_view sv)
{
	if(using_colors)
		set_color(96); //bright cyan

	print_timestamp();
	
	out << "[INFO] " << sv;
	if (using_auto_nls)
		out << '\n';
	
	if(using_colors)
		reset_color();
}

void Logger::warn(const std::string_view sv)
{
	if (using_colors)
		set_color(92); //bright green

	print_timestamp();

	out << "[WARNING] " << sv;
	if (using_auto_nls)
		out << '\n';

	if (using_colors)
		reset_color();

	out << std::flush;
}

void Logger::error(const std::string_view sv, i8 ec)
{
	if (using_colors)
		set_color(91); //bright red

	print_timestamp();

	out << "[ERROR] " << sv;
	if (using_auto_nls)
		out << '\n';

	if (using_colors)
		reset_color();

	out << std::flush;

	if(ec != 0)
		std::exit(ec);
}