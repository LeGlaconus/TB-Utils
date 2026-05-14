#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <span>

#include "FGDParser.hpp"

AppParams app_params;

Logger logger{};

bool handle_file(std::filesystem::path filename)
{
	FGD::Parser parser{ filename };
	if (!parser.valid())
		return false;

	logger.info(std::format("Now parsing {}", filename.string()));
	const auto t_parse_start = std::chrono::steady_clock::now();
	auto fgd = parser.parse();
	const auto t_parse_end = std::chrono::steady_clock::now();
	logger.info(std::format("Took {}", std::chrono::duration<double>(t_parse_end - t_parse_start)));

	if (!fgd.valid)
	{
		logger.error(std::format("The fgd for {} is invalid", filename.string()), 0);
		return false;
	}

	if (app_params.output_folder.empty()) {
		app_params.output_folder = std::filesystem::current_path() / "out";
		std::filesystem::create_directory(app_params.output_folder);
	}

	const std::filesystem::path out_filename = app_params.output_folder / filename;
	logger.info(std::format("Now writing {}", out_filename.string()));
	const auto t_write_start = std::chrono::steady_clock::now();
	FGD::Writer writer{ fgd, FormatType::Trenchbroom };
	if (!writer.write(out_filename))
		logger.error("Couldn't write the fgd", 0);
	const auto t_write_end = std::chrono::steady_clock::now();
	logger.info(std::format("Took {}", std::chrono::duration<double>(t_write_end - t_write_start)));

	return true;
}

int main(int argc, char* argv[])
{
	const std::span<char*> args{ argv, static_cast<std::size_t>(argc) };
	if (args.size() < 2)
	{
		std::cerr	<< "Usage: fgd-utils <path to an .fgd> [optional params]\n"
					<< "Refer to the readme for additional parameters\n";
		return 1;
	}

	const std::filesystem::path first_path{ args[1] };

	const std::span<char*> opt_args{ argv + 2, static_cast<std::size_t>(argc) - 2 };
	
	bool expecting_output_path = false;
	bool expecting_spr_extension = false;
	bool expecting_mdl_extension = false;
	for (const std::string_view arg : opt_args)
	{
		if (expecting_output_path) {
			app_params.output_folder = arg;
			expecting_output_path = false;
		} else if (expecting_spr_extension) {
			app_params.spr_extension = arg;
			expecting_spr_extension = false;
		} else if (expecting_spr_extension) {
			app_params.mdl_extension = arg;
			expecting_mdl_extension = false;
		}

		if (arg == "--recurse-includes")
			app_params.recurse_includes = true;
		else if (arg == "--output-path")
			expecting_output_path = true;
		else if (arg == "--sprite-extension")
			expecting_spr_extension = true;
		else if (arg == "--model-extension")
			expecting_mdl_extension = true;
	}

	if (expecting_output_path)
		logger.error("The output folder wasn't provided ! Expecting a path after --output-path", 1);
	else if (expecting_spr_extension)
		logger.error("The sprite extension wasn't provided ! Expecting an extension after --sprite-extension", 1);
	else if (expecting_mdl_extension)
		logger.error("The model extension wasn't provided ! Expecting an extension after --model-extension", 1);

	if (!handle_file(first_path))
		logger.error("FGD handling failed");

	return 0;
}