#include <iostream>
#include <span>
#include <chrono>

#include "MAPParser.hpp"
#include "VMFParser.hpp"

AppParams app_params{};

Logger logger{};

Map Map::Parser::parse()
{
	in.open(filename);
	if (in.fail())
	{
		logger.error(std::format("Error reading file {}", filename.string()), 1);
		return {};
	}

	if (filename.extension() == ".map")
		parse_map();
	else if (filename.extension() == ".vmf")
		parse_vmf();
	else
		logger.error(std::format("Invalid extension {}", filename.extension().string()), 1);

	in.close();

	return map;
}

bool Map::Writer::write(const std::filesystem::path filename)
{
	out.open(filename);
	if (out.fail())
	{
		logger.error(std::format("Couldn't write map {}", filename.string()), 1);
		return false;
	}

	bool ret = true;

	if (filename.extension() == ".map")
		ret = write_map();
	else if (filename.extension() == ".vmf")
		ret = write_vmf();
	else
		logger.error(std::format("Invalid extension {}", filename.extension().string()), 1);

	out.close();

	return ret;
}

int main(int argc, char* argv[])
{
	const std::span<char*> args{argv, static_cast<std::size_t>(argc)};
	if (args.size() < 3)
	{
		std::cerr << "Usage: vmf-utils <path to a .map/.vmf> <path for the .vmf/.map> [optional params]\n"
				  << "Refer to the readme for additional parameters\n";
		return 1;
	}

	const std::filesystem::path first_path{ args[1] };
	const std::filesystem::path second_path{ args[2] };

	const std::span<char*> opt_args{ argv + 3, static_cast<std::size_t>(argc) - 3 };
	
	bool expecting_material_path = false;
	bool expecting_material_prefix = false;
	bool expecting_lightmap_scale = false;
	for (const std::string_view arg : opt_args)
	{
			   if (expecting_material_path) {
			app_params.material_path = arg;
			expecting_material_path = false;
			continue;
		} else if (expecting_material_prefix) {
			app_params.mat_prefix = arg;
			expecting_material_prefix = false;
			continue;
		} else if (expecting_lightmap_scale) {
			app_params.default_lightmap_scale = static_cast<u16>(std::atoll(arg.data()));
			if (app_params.default_lightmap_scale == 0)
				app_params.default_lightmap_scale = 16;
			expecting_lightmap_scale = false;
			continue;
		}
		
			   if (arg == "--fix-faces") {
			app_params.fix_faces = true;
			expecting_material_path = true;
		} else if (arg == "--round-values") {
			app_params.round_values = true;
		} else if (arg == "--export-prefab") {
			app_params.export_prefab = true;
		} else if (arg == "--material-prefix") {
			expecting_material_prefix = true;
		} else if (arg == "--quake-axis-format") {
			app_params.quake_axis_format = true;
		} else if (arg == "--default-lightmap-scale") {
			expecting_lightmap_scale = true;
		} else if (arg == "--fix-quake-lights") {
			app_params.fix_quake_lights = true;
		} else if (arg == "--write-quake-lights-as-valve") {
			app_params.write_quake_lights_as_valve_format = true;
		} else if (arg == "--keep-special-keyvalues") {
			app_params.keep_special_kvs = true;
		} else if (arg == "--strip-material-folder") {
			app_params.strip_mat_folder = true;
		} else if (arg == "--no-attrib-write") {
			app_params.no_attrib_write = true;
		} else {
			logger.warn(std::format("Invalid parameter {}", arg));
		}
	}
		
	if (expecting_material_path) {
		logger.error("No material path was provided ! Expecting one when using --fix-faces", 1);
	} else {
		if (!app_params.material_path.empty())
			logger.info(std::format("Using material path {}", app_params.material_path.string()));
	}

	if (expecting_material_prefix) {
		logger.error("No material prefix was provided ! Expecting one when using --material-prefix", 1);
	} else {
		if (!app_params.mat_prefix.empty())
			logger.info(std::format("Using material prefix {}", app_params.mat_prefix));
	}

	if (expecting_lightmap_scale) {
		logger.error("No lightmap scale was provided ! Expecting one when using --default-lightmap-scale");
	} else {
		if (app_params.default_lightmap_scale != 16)
			logger.info(std::format("Using default lightmap scale {}", app_params.default_lightmap_scale));
	}

	const auto first_t_start = std::chrono::steady_clock::now();

	Map::Parser parser{ first_path };
	Map map = parser.parse();

	if (map.entities.empty())
		return 1;

	const auto first_t_end = std::chrono::steady_clock::now();
	logger.info(std::format("Succesfully parsed {}", first_path.string()));
	logger.info(std::format("Took {}", std::chrono::duration<double>(first_t_end - first_t_start)));

	u32 ec = 0;

	if (first_path.extension() == ".map" && second_path.extension() != ".map") {
		ec = handle_trenchbroom_layers(map);
		if (ec != 0)
			return ec;
	}
	else if (first_path.extension() == ".vmf" && second_path.extension() != ".vmf") {
		ec = handle_hammer_layers(map);
		if (ec != 0)
			return 0;
	}

	logger.info("Succesfully handled layers/visgroups");

	const auto second_t_start = std::chrono::steady_clock::now();
	Map::Writer writer{ map };
	if (!writer.write(second_path))
		logger.error(std::format("Couldn't write {}", second_path.string()), 1);

	const auto second_t_end = std::chrono::steady_clock::now();
	logger.info(std::format("Succesfully wrote {}", second_path.string()));
	logger.info(std::format("Took {}", std::chrono::duration<double>(second_t_end - second_t_start)));

	return 0;
}