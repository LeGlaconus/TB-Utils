#include <iostream>
#include <fstream>
#include <format>

#include "VMFParser.hpp"

void Map::Writer::write_str(const std::string_view sv, u32 tabs)
{
	for (u32 i = 0; i < tabs; i++) {
		static const char tab = '\t';
		out.write(&tab, 1);
	}

	out.write(sv.data(), sv.size());
}

void Map::Writer::write_kv(const KVPair& kv, u32 tabs)
{
	write_str(std::format("\"{}\" \"{}\"\n", kv.first, kv.second), tabs);
}

//Note: Hammer rounds by doing fabs(val - rint(val)) < epsilon
static inline double round_to(const double val, const u32 place)
{
	const double mul = std::pow(10.0, place);
	//return std::floor(static_cast<double>(val * mul)) / mul;
	return std::round(static_cast<double>(val * mul)) / mul; //Sometimes floor is more similar and sometimes round is, I believe it uses a different function or a different version of libc
}

static inline void make_uppercase_str(std::string& str)
{
	for (auto& c : str) {
		c = std::toupper(c);
	}
}

KVPair* find_kv(std::vector<KVPair>& vec, const std::string_view key)
{
	for (auto& kv : vec) {
		if (kv.first == key)
			return &kv;
	}

	return nullptr;
}

const KVPair* find_kv(const std::vector<KVPair>& vec, const std::string_view key)
{
	for (auto& kv : vec) {
		if (kv.first == key)
			return &kv;
	}

	return nullptr;
}

static inline const std::optional<std::pair<u16, u16>> dimensions_for_material(const std::string_view material)
{
	const std::filesystem::path mat_path = app_params.material_path / std::filesystem::path{ material }.replace_extension(".tga");
	std::ifstream in{ mat_path, std::ios::binary };
	if (in.fail())
	{
		logger.warn(std::format("Couldn't open {} to get its dimensions", mat_path.string()));
		return std::nullopt;
	}

	in.seekg(0xC, std::ios::beg);
	u16 width = 0;
	in.read(reinterpret_cast<char*>(&width), sizeof(u16));
	u16 height = 0;
	in.read(reinterpret_cast<char*>(&height), sizeof(u16));

	in.close();

	return std::make_pair(width, height);
}

static inline const Side normalize_tex_shifts(const Side& side)
{
	//All values of t and s get rounded to 0.001
	//Then s gets modulo'd by the texture width/height
	
	auto side_c = side;
	side_c.uaxis.t.x	= round_to(side_c.uaxis.t.x,	6);
	side_c.uaxis.t.y	= round_to(side_c.uaxis.t.y,	6);
	side_c.uaxis.t.z	= round_to(side_c.uaxis.t.z,	6);
	side_c.uaxis.s		= round_to(side_c.uaxis.s,		6);

	side_c.vaxis.t.x	= round_to(side_c.vaxis.t.x,	6);
	side_c.vaxis.t.y	= round_to(side_c.vaxis.t.y,	6);
	side_c.vaxis.t.z	= round_to(side_c.vaxis.t.z,	6);
	side_c.vaxis.s		= round_to(side_c.vaxis.s,		6);

	const auto dims = dimensions_for_material(side_c.material);
	if (!dims.has_value())
		return side_c;

	const auto dims_v = dims.value();
	if (dims_v.first != 0)
		side_c.uaxis.s = std::fmod(static_cast<double>(side_c.uaxis.s), dims_v.first);
	if (dims_v.second != 0)
		side_c.vaxis.s = std::fmod(static_cast<double>(side_c.vaxis.s), dims_v.second);

	return side_c;
}

bool Map::Writer::write_vmf()
{
	u32 depth = 0;

	//Write the version info
	{
		write_str("versioninfo\n", depth);

		write_str("{\n", depth);
		depth++;

		auto const map_version_kv = find_kv(map.entities[0].keyvalues, "mapversion");
		if (map_version_kv) {
			write_kv(std::make_pair("mapversion", map_version_kv->second), depth);
		}

		//The VDC wiki says the format version is unkowingly set so it's probably always 100
		write_kv(std::make_pair("formatversion", "100"), depth);

		write_kv(std::make_pair("prefab", app_params.export_prefab ? "1" : "0"), depth);

		depth--;
		write_str("}\n", depth);
	}

	//Write the visgroup info
	{
		write_str("visgroups\n", depth);

		write_str("{\n", depth);
		depth++;

		for (auto& layer : map.layers)
		{
			write_str("visgroup\n", depth);

			write_str("{\n", depth);
			depth++;

			write_kv(std::make_pair("name", layer.name), depth);
			write_kv(std::make_pair("visgroupid", std::to_string(layer.id)), depth);

			depth--;
			write_str("}\n", depth);
		}

		depth--;
		write_str("}\n", depth);
	}

	//Write the entities
	//Assuming the first entity is the world entity
	for (auto& ent : map.entities)
	{
		//Write world or entity

		//The Entity id is used to determine if an entity is the world entity
		if (ent.id == 0)
			write_str("world\n", depth);
		else
			write_str("entity\n", depth);

		write_str("{\n", depth);
		depth++;

		write_kv(std::make_pair("id", std::to_string(ent.id)), depth);

		std::vector<KVPair> connections;

		for (auto& kv : ent.keyvalues) {
			if (!app_params.keep_special_kvs)
				if (kv.first.starts_with("_tb") || kv.first.starts_with("_vmf"))
					continue;

			//Every keyvalue with @ is a connection, it is used like # in multi_managers
			const auto pos = kv.first.find('@');
			if (pos < kv.first.size()) {
				connections.push_back(std::make_pair(kv.first.substr(0, pos), kv.second));
				continue;
			}

			write_kv(kv, depth);
		}

		if (!ent.solids.empty())
		{
			for (auto& solid : ent.solids)	
			{
				write_str("solid\n", depth);

				write_str("{\n", depth);
				depth++;

				write_kv(std::make_pair("id", std::to_string(solid.id)), depth);

				for (auto& side : solid.sides)
				{
					write_str("side\n", depth);

					write_str("{\n", depth);
					depth++;
					
					write_kv(std::make_pair("id", std::to_string(side.id)), depth);
					
					//Plane, lack of depth intentional
					write_str("\"plane\" \"", depth);
					for (u8 i = 0; i < 3; i++) {
						auto plane_c = side.plane_tri[i];
						if (app_params.round_values) {
							plane_c.x = round_to(plane_c.x, 3);
							plane_c.y = round_to(plane_c.y, 3);
							plane_c.z = round_to(plane_c.z, 3);
						}

						write_str(std::format("({} {} {})", plane_c.x, plane_c.y, plane_c.z));
						
						if(i != 2)
							write_str(std::format(" "));
					}
					write_str("\"\n");

					auto material_c = app_params.mat_prefix + side.material;

					//Hammer makes every material name uppercase
					make_uppercase_str(material_c);

					//Material
					write_kv(std::make_pair("material", material_c), depth);

					//Hammer does rounding and unwrapping that Trenchbroom doesn't, let's do it here
					
					const auto side_c = app_params.fix_faces ? normalize_tex_shifts(side) : side;

					//uaxis
					write_str(std::format("\"uaxis\" \"[{} {} {} {}] {}\"\n", side_c.uaxis.t.x, side_c.uaxis.t.y, side_c.uaxis.t.z, side_c.uaxis.s, side_c.uaxis.ts), depth);
					//vaxis
					write_str(std::format("\"vaxis\" \"[{} {} {} {}] {}\"\n", side_c.vaxis.t.x, side_c.vaxis.t.y, side_c.vaxis.t.z, side_c.vaxis.s, side_c.vaxis.ts), depth);

					auto rotation_c = side.rotation;
					if(app_params.round_values)
						rotation_c = round_to(rotation_c, 4);

					//Rotation
					write_kv(std::make_pair("rotation", std::to_string(rotation_c)), depth);

					//Lightmap scale
					write_kv(std::make_pair("lightmapscale", std::to_string(app_params.default_lightmap_scale)), depth);

					//Smoothing groups
					write_kv(std::make_pair("smoothing_groups", "0"), depth); //While I suggested there should be a parameter for this, it is a bad idea

					if (side.attribs.has_value() && !app_params.no_attrib_write)
					{
						//Contents
						write_kv(std::make_pair("contents", std::to_string(side.attribs.value().contents)), depth);
						//Flags
						write_kv(std::make_pair("flags", std::to_string(side.attribs.value().flags)), depth);
						//Values are ignored
					}

					if (side.dispinfo.has_value())
					{
						write_str("dispinfo\n", depth);

						write_str("{\n", depth);
						depth++;

						auto& di = side.dispinfo.value();

						write_kv(std::make_pair("power", std::to_string(di.power)), depth);
						write_kv(std::make_pair("startposition", std::format("[{} {} {}]", di.startposition.x, di.startposition.y, di.startposition.z)), depth);
						write_kv(std::make_pair("elevation", std::to_string(di.elevation)), depth);
						write_kv(std::make_pair("subdiv", di.subdivided ? "1" : "0"), depth);
						write_kv(std::make_pair("flags", std::to_string(di.flags)), depth);

						//Normals
						write_str("normals\n", depth);

						write_str("{\n", depth);
						depth++;

						for (u8 r = 0; r < di.normals.size(); r++)
						{
							write_str(std::format("\"row{}\" \"", r), depth);
							
							for (u8 c = 0; c < di.normals[r].size(); c++)
							{
								write_str(std::format("{} {} {}", di.normals[r][c].x, di.normals[r][c].y, di.normals[r][c].z));
								if (c < (di.normals[r].size() - 1))
									write_str(" ");
							}

							write_str("\"\n");
						}

						depth--;
						write_str("}\n", depth);

						//Distances

						write_str("distances\n", depth);

						write_str("{\n", depth);
						depth++;

						for (u8 r = 0; r < di.distances.size(); r++)
						{
							write_str(std::format("\"row{}\" \"", r), depth);

							for (u8 c = 0; c < di.distances[r].size(); c++)
							{
								write_str(std::format("{}", di.distances[r][c]));
								if (c < (di.distances[r].size() - 1))
									write_str(" ");
							}

							write_str("\"\n");
						}

						depth--;
						write_str("}\n", depth);

						//Offsets

						write_str("offsets\n", depth);

						write_str("{\n", depth);
						depth++;

						for (u8 r = 0; r < di.offsets.size(); r++)
						{
							write_str(std::format("\"row{}\" \"", r), depth);

							for (u8 c = 0; c < di.offsets[r].size(); c++)
							{
								write_str(std::format("{} {} {}", di.offsets[r][c].x, di.offsets[r][c].y, di.offsets[r][c].z));
								if (c < (di.offsets[r].size() - 1))
									write_str(" ");
							}

							write_str("\"\n");
						}

						//Offset normals

						depth--;
						write_str("}\n", depth);

						write_str("offset_normals\n", depth);

						write_str("{\n", depth);
						depth++;

						for (u8 r = 0; r < di.offset_normals.size(); r++)
						{
							write_str(std::format("\"row{}\" \"", r), depth);

							for (u8 c = 0; c < di.offset_normals[r].size(); c++)
							{
								write_str(std::format("{} {} {}", di.offset_normals[r][c].x, di.offset_normals[r][c].y, di.offset_normals[r][c].z));
								if (c < (di.offset_normals[r].size() - 1))
									write_str(" ");
							}

							write_str("\"\n");
						}

						depth--;
						write_str("}\n", depth);

						//Alphas

						write_str("alphas\n", depth);

						write_str("{\n", depth);
						depth++;

						for (u8 r = 0; r < di.alphas.size(); r++)
						{
							write_str(std::format("\"row{}\" \"", r), depth);

							for (u8 c = 0; c < di.alphas[r].size(); c++)
							{
								write_str(std::format("{}", di.alphas[r][c]));
								if (c < (di.alphas[r].size() - 1))
									write_str(" ");
							}

							write_str("\"\n");
						}

						depth--;
						write_str("}\n", depth);

						//Triangle tags

						write_str("triangle_tags\n", depth);

						write_str("{\n", depth);
						depth++;

						for (u8 r = 0; r < di.triangle_tags.size(); r++)
						{
							write_str(std::format("\"row{}\" \"", r), depth);

							for (u8 c = 0; c < di.triangle_tags[r].size(); c++)
							{
								write_str(std::format("{}", di.triangle_tags[r][c]));
								if (c < (di.triangle_tags[r].size() - 1))
									write_str(" ");
							}

							write_str("\"\n");
						}

						depth--;
						write_str("}\n", depth);

						//Allowed verts

						write_str("allowed_verts\n", depth);

						write_str("{\n", depth);
						depth++;

						write_str(std::format("\"{}\" \"", di.allowed_verts.size()), depth);

						for (u8 c = 0; c < di.allowed_verts.size(); c++)
						{
							write_str(std::format("{}", di.allowed_verts[c]));
							if (c < (di.allowed_verts.size() - 1))
								write_str(" ");
						}

						write_str("\"\n");

						depth--;
						write_str("}\n", depth);

						depth--;
						write_str("}\n", depth);
					}

					depth--;
					write_str("}\n", depth);
				}

				//Write visgroup/group info
				bool is_in_visgroup = false;
				Layer* cur_layer = nullptr;
				for (auto& layer : map.layers) {
					for(auto& id : layer.solid_ids) {
						if (solid.id == id) {
							is_in_visgroup = true;
							cur_layer = const_cast<Layer*>(&layer);
							break;
						}
					}
				}

				bool is_in_group = false;
				Group* cur_group = nullptr;
				for (auto& group : map.groups) {
					for (auto& id : group.solid_ids) {
						if (solid.id == id) {
							is_in_group = true;
							cur_group = const_cast<Group*>(&group);
							break;
						}
					}
				}

				if (is_in_visgroup || is_in_group)
				{
					write_str("editor\n", depth);

					write_str("{\n", depth);
					depth++;

					if (cur_layer)
					{
						write_kv(std::make_pair("visgroupid", std::to_string(cur_layer->id)), depth);
						write_kv(std::make_pair("visgroupshown", cur_layer->hidden ? "0" : "1"), depth);
					}

					if (cur_group) {
						write_kv(std::make_pair("groupid", std::to_string(cur_group->id)), depth);
					}

					depth--;
					write_str("}\n", depth);
				}

				depth--;
				write_str("}\n", depth);
			}
		}

		//Write visgroup/group info
		bool is_in_visgroup = false;
		Layer* cur_layer = nullptr;
		for (auto& layer : map.layers) {
			for (auto& id : layer.ent_ids) {
				if (ent.id == id) {
					is_in_visgroup = true;
					cur_layer = const_cast<Layer*>(&layer);
					break;
				}
			}
		}

		bool is_in_group = false;
		Group* cur_group = nullptr;
		for (auto& group : map.groups) {
			for (auto& id : group.ent_ids) {
				if (ent.id == id) {
					is_in_group = true;
					cur_group = const_cast<Group*>(&group);
					break;
				}
			}
		}

		auto comments_kv = find_kv(ent.keyvalues, "_vmf_comments");

		if (is_in_visgroup || is_in_group || comments_kv)
		{
			write_str("editor\n", depth);

			write_str("{\n", depth);
			depth++;

			if (cur_layer)
			{
				write_kv(std::make_pair("visgroupid", std::to_string(cur_layer->id)), depth);
				write_kv(std::make_pair("visgroupshown", cur_layer->hidden ? "0" : "1"), depth);
			}

			if (cur_group) {
				write_kv(std::make_pair("groupid", std::to_string(cur_group->id)), depth);
			}

			if (comments_kv) {
				write_kv(std::make_pair("comments", comments_kv->second), depth);
			}

			depth--;
			write_str("}\n", depth);
		}

		if (!connections.empty())
		{
			write_str("connections\n", depth);

			write_str("{\n", depth);
			depth++;

			for (auto& con : connections) {
				write_kv(con, depth);
			}

			depth--;
			write_str("}\n", depth);
		}

		//If this is the world entity, write group info
		if (ent.id == 0)
		{
			for (auto& group : map.groups)
			{
				write_str("group\n", depth);

				write_str("{\n", depth);
				depth++;

				write_kv(std::make_pair("id", std::to_string(group.id)), depth);

				depth--;
				write_str("}\n", depth);
			}
		}

		depth--;
		write_str("}\n", depth);
	}

	return true;
}

void Map::Parser::parse_vmf_solid(Solid* cur_solid, Entity* cur_ent, Block& solid_block)
{
	cur_ent->solids.push_back({});
	cur_solid = &cur_ent->solids.back();

	for (auto& solid_kv : solid_block.keyvalues) {
		if (solid_kv.first == "id") {
			cur_solid->id = static_cast<ID>(std::stoll(solid_kv.second));
		}
	}

	Side* cur_side = nullptr;

	for (auto& side_block : solid_block.blocks)
	{
		if (side_block.name == "editor")
		{
			for (auto& solid_ed_kv : side_block.keyvalues)
			{
				if (solid_ed_kv.first == "visgroupid") {
					std::istringstream ss{ solid_ed_kv.second };
					ID vgroup_id = 0;
					ss >> vgroup_id;

					for (auto& layer : map.layers) {
						if (layer.id == vgroup_id) {
							layer.solid_ids.push_back(cur_solid->id);

							for (auto& kv : side_block.keyvalues)
							{
								if (kv.first == "visgroupshown") {
									if (kv.second == "0")
										layer.hidden = true;
								}
							}
						}
					}
				}
				if (solid_ed_kv.first == "groupid") {
					std::istringstream ss{ solid_ed_kv.second };
					ID group_id = 0;
					ss >> group_id;

					for (auto& group : map.groups) {
						if (group.id == group_id) {
							group.solid_ids.push_back(cur_solid->id);
						}
					}
				}
			}

			continue;
		}

		cur_solid->sides.push_back({});
		cur_side = &cur_solid->sides.back();

		for (auto& side_kv : side_block.keyvalues) {
			if (side_kv.first == "id") {
				cur_side->id = static_cast<ID>(std::stoll(side_kv.second));
			} else if (side_kv.first == "plane") {
				std::string val = side_kv.second;

				for (auto& c : val) {
					if (c == '(' || c == ')')
						c = ' ';
				}

				std::istringstream ss{ val };

				for (u8 i = 0; i < 3; i++) {
					ss >> cur_side->plane_tri[i].x >> cur_side->plane_tri[i].y >> cur_side->plane_tri[i].z;
				}
			}
			else if (side_kv.first == "material") {
				cur_side->material = side_kv.second;

				if (app_params.strip_mat_folder)
				{
					auto pos = cur_side->material.find_last_of('/');
					if (pos < cur_side->material.size())
						cur_side->material = cur_side->material.substr(pos + 1);
				}
			} else if (side_kv.first == "uaxis") {
				std::string val = side_kv.second;

				for (auto& c : val) {
					if (c == '[' || c == ']')
						c = ' ';
				}

				std::istringstream ss{ val };

				ss >> cur_side->uaxis.t.x >> cur_side->uaxis.t.y >> cur_side->uaxis.t.z >> cur_side->uaxis.s;
				ss >> cur_side->uaxis.ts;
			} else if (side_kv.first == "vaxis") {
				std::string val = side_kv.second;

				for (auto& c : val) {
					if (c == '[' || c == ']')
						c = ' ';
				}

				std::istringstream ss{ val };

				ss >> cur_side->vaxis.t.x >> cur_side->vaxis.t.y >> cur_side->vaxis.t.z >> cur_side->vaxis.s;
				ss >> cur_side->vaxis.ts;
			} else if (side_kv.first == "rotation") {
				std::istringstream ss{ side_kv.second };
				ss >> cur_side->rotation;
			} else if (side_kv.first == "contents") {
				std::istringstream ss{ side_kv.second };
				if (!cur_side->attribs.has_value())
					cur_side->attribs.emplace();

				ss >> cur_side->attribs.value().contents;
			} else if (side_kv.first == "flags") {
				std::istringstream ss{ side_kv.second };
				if (!cur_side->attribs.has_value())
					cur_side->attribs.emplace();

				ss >> cur_side->attribs.value().flags;
				cur_side->attribs.value().value = 0u;
			}
		}

		for (auto& dispinfo_block : side_block.blocks)
		{
			if (dispinfo_block.name != "dispinfo")
				continue;

			cur_side->dispinfo.emplace();
			auto& dispinfo = cur_side->dispinfo.value();

			for (auto& kv : dispinfo_block.keyvalues)
			{
				if(kv.first == "power") {
					dispinfo.power = static_cast<u8>(std::stoll(kv.second));
				}
				else if (kv.first == "startposition") {
					std::string val{ kv.second };

					for (auto& c : val) {
						if (c == '[' || c == ']')
							c = ' ';
					}

					std::stringstream val_ss{ val };
					val_ss >> dispinfo.startposition.x >> dispinfo.startposition.y >> dispinfo.startposition.z;
				} else if (kv.first == "elevation") {
					std::istringstream ss{ kv.second };
					ss >> dispinfo.elevation;
				} else if (kv.first == "subdiv") {
					dispinfo.subdivided = (kv.second == "1") ? true : false;
				} else if (kv.first == "flags") {
					dispinfo.flags = static_cast<u64>(std::stoll(kv.second));
				}
			}

			for (auto& diblock : dispinfo_block.blocks)
			{
				const size_t row_num = static_cast<size_t>(std::pow(2, dispinfo.power) + 1);
				dispinfo.normals.resize(row_num);
				dispinfo.distances.resize(row_num);
				dispinfo.offsets.resize(row_num);
				dispinfo.offset_normals.resize(row_num);
				dispinfo.alphas.resize(row_num);
				dispinfo.triangle_tags.resize(row_num - 1); //triangle_tags uses 2^n because it stores data per triangle instead of per vertex

				if (diblock.name == "normals")
				{
					for (u8 r = 0; r < row_num; r++)
					{
						std::istringstream ss{ diblock.keyvalues[r].second };
						for (u8 c = 0; c < row_num; c++)
						{
							Vec3 v{};
							ss >> v.x >> v.y >> v.z;
							dispinfo.normals[r].push_back(v);
						}
					}
				}
				else if (diblock.name == "distances")
				{
					for (u8 r = 0; r < row_num; r++)
					{
						std::istringstream ss{ diblock.keyvalues[r].second };
						for (u8 c = 0; c < row_num; c++)
						{
							double v = .0;
							ss >> v;
							dispinfo.distances[r].push_back(v);
						}
					}
				}
				if (diblock.name == "offsets")
				{
					for (u8 r = 0; r < row_num; r++)
					{
						std::istringstream ss{ diblock.keyvalues[r].second };
						for (u8 c = 0; c < row_num; c++)
						{
							Vec3 v{};
							ss >> v.x >> v.y >> v.z;
							dispinfo.offsets[r].push_back(v);
						}
					}
				}
				if (diblock.name == "offset_normals")
				{
					for (u8 r = 0; r < row_num; r++)
					{
						std::istringstream ss{ diblock.keyvalues[r].second };
						for (u8 c = 0; c < row_num; c++)
						{
							Vec3 v{};
							ss >> v.x >> v.y >> v.z;
							dispinfo.offset_normals[r].push_back(v);
						}
					}
				}
				else if (diblock.name == "alphas")
				{
					for (u8 r = 0; r < row_num; r++)
					{
						std::istringstream ss{ diblock.keyvalues[r].second };
						for (u8 c = 0; c < row_num; c++)
						{
							double v;
							ss >> v;
							dispinfo.alphas[r].push_back(v);
						}
					}
				}
				else if (diblock.name == "triangle_tags")
				{
					for (u8 r = 0; r < (row_num - 1); r++)
					{
						std::istringstream ss{ diblock.keyvalues[r].second };
						for (u8 c = 0; c < (row_num - 1); c++)
						{
							std::string s;
							ss >> s;
							const u8 v = static_cast<u8>(std::stoll(s));
							dispinfo.triangle_tags[r].push_back(v);
						}
					}
				}
				else if (diblock.name == "allowed_verts")
				{
					std::istringstream ssf{ diblock.keyvalues[0].first };
					size_t size = 0u;
					ssf >> size;
					dispinfo.allowed_verts.resize(size);

					std::istringstream ss{ diblock.keyvalues[0].second };
					for (u8 i = 0; i < size; i++) {
						ss >> dispinfo.allowed_verts[i];
					}
				}
			}

			break;
		}
	}
}

void Map::Parser::parse_vmf_entity(Entity* cur_ent, Block& block)
{
	map.entities.push_back({});
	cur_ent = &map.entities.back();

	if (!block.keyvalues.empty())
	{
		cur_ent->keyvalues.insert(cur_ent->keyvalues.begin(), block.keyvalues.begin(), block.keyvalues.end());

		for (auto it = cur_ent->keyvalues.begin(); it != cur_ent->keyvalues.end(); it = std::next(it)) {
			if (it->first == "id") {
				cur_ent->id = static_cast<ID>(std::stoll(it->second));
				cur_ent->keyvalues.erase(it);
				break;
			}
		}
	}

	Group* cur_group = nullptr;

	for (auto& group_block : block.blocks)
	{
		if (group_block.name != "group")
			continue;

		map.groups.push_back({});
		cur_group = &map.groups.back();

		for (auto& kv : group_block.keyvalues)
		{
			if (kv.first == "id") {
				std::istringstream ss{ kv.second };
				ss >> cur_group->id;

				//The VMF format doesn't have names for groups
				cur_group->name = std::format("Group n{}", cur_group->id);
			}
		}
	}

	Solid* cur_solid = nullptr;

	for (auto& solid_block : block.blocks)
	{
		if (solid_block.name == "editor")
		{
			for (auto& ent_ed_kv : solid_block.keyvalues)
			{
				if (ent_ed_kv.first == "visgroupid") {
					std::istringstream ss{ ent_ed_kv.second };
					ID vgroup_id = 0;
					ss >> vgroup_id;

					for (auto& layer : map.layers) {
						if (layer.id == vgroup_id) {
							layer.ent_ids.push_back(cur_ent->id);

							for (auto& kv : solid_block.keyvalues)
							{
								if (kv.first == "visgroupshown") {
									if (kv.second == "0")
										layer.hidden = true;
								}
							}
						}
					}
				}
				else if (ent_ed_kv.first == "groupid") {
					std::istringstream ss{ ent_ed_kv.second };
					ID group_id = 0;
					ss >> group_id;

					for (auto& group : map.groups) {
						if (group.id == group_id) {
							group.ent_ids.push_back(cur_ent->id);
						}
					}
				}
				else if (ent_ed_kv.first == "comments") {
					cur_ent->keyvalues.push_back(std::make_pair("_vmf_comments", ent_ed_kv.second));
				}
			}

			continue;
		}

		if (solid_block.name == "connections")
		{
			std::vector<std::pair<std::string, u32>> keys;

			for (auto& kv : solid_block.keyvalues)
			{
				bool defined = false;
				for (auto& k : keys)
				{
					if (kv.first == k.first) {
						k.second++;
						defined = true;
					}
				}

				if (!defined)
					keys.push_back(std::make_pair(kv.first, 1));

				for (auto& k : keys)
				{
					if (kv.first == k.first) {
						cur_ent->keyvalues.push_back(std::make_pair(std::format("{}@{}", kv.first, k.second), kv.second));
					}
				}
			}
		}

		if (solid_block.name == "hidden")
		{
			for (auto& real_solid_block : solid_block.blocks)
			{
				if (real_solid_block.name != "solid")
					continue;

				parse_vmf_solid(cur_solid, cur_ent, real_solid_block);
			}
		}

		if (solid_block.name != "solid")
			continue;

		parse_vmf_solid(cur_solid, cur_ent, solid_block);
	}
}

void Map::Parser::parse_vmf()
{
	Block master_block;
	Block* cur_block = &master_block;

	std::string last_token;

	char buf[1024];
	buf[1023] = '\0';
	do
	{
		in.getline(buf, 1023);
		const std::string_view line{ buf };

		std::string first_token;
		{
			const std::string line_c{ line };
			std::istringstream ss{ line_c };
			ss >> first_token;
		}

		if (first_token.starts_with("//")) {
			continue;
		}
		else if (first_token.starts_with("{"))
		{
			cur_block->blocks.push_back({});
			cur_block->blocks.back().parent = cur_block;
			cur_block = &cur_block->blocks.back();

			std::istringstream ll_ss{ last_token };
			ll_ss >> cur_block->name;
		}
		else if (first_token.starts_with("}"))
		{
			cur_block = cur_block->parent;
		}
		else {
			if (!first_token.starts_with("\"") || !first_token.ends_with("\"")) {
				last_token = first_token;
				continue;
			}

			const std::string line_c{ line };
			std::istringstream ss{ line_c };
			KVPair kv;
			ss >> kv.first;

			//Assuming here that the key can't have spaces but the value can
			kv.second = ss.str().substr(static_cast<std::size_t>(ss.tellg()) + 1);

			kv.first = kv.first.substr(1, kv.first.size() - 2);
			kv.second = kv.second.substr(1, kv.second.size() - 2);

			cur_block->keyvalues.push_back(kv);
		}
	} while (in.good());

	for (auto& block : master_block.blocks)
	{
		Entity* cur_ent = nullptr;

		if (block.name == "visgroups")
		{
			Layer* cur_layer = nullptr;

			for (auto& visgroup_block : block.blocks)
			{
				if (visgroup_block.name != "visgroup")
					continue;

				map.layers.push_back({});
				cur_layer = &map.layers.back();

				for (auto& vgr_kv : visgroup_block.keyvalues)
				{
					if (vgr_kv.first == "name")
						cur_layer->name = vgr_kv.second;
					else if (vgr_kv.first == "visgroupid")
						cur_layer->id = static_cast<ID>(std::stoll(vgr_kv.second));
				}
			}
		}

		if (block.name == "world" || block.name == "entity")
		{
			parse_vmf_entity(cur_ent, block);
		}

		if (block.name == "hidden")
		{
			for (auto& ent_block : block.blocks)
			{
				if (ent_block.name != "entity")
					continue;

				parse_vmf_entity(cur_ent, ent_block);
			}
		}
	}
}

u32 handle_hammer_layers(Map& map)
{
	for (auto& layer : map.layers)
	{
		for (auto& ent : map.entities) {
			for (auto& ent_id : layer.ent_ids) {
				if (ent_id == ent.id) {
					ent.keyvalues.push_back(std::make_pair("_tb_layer", std::to_string(layer.id)));
				}
			}
		}

		ID biggest_id = 0;
		for (auto& ent : map.entities) {
			if (ent.id > biggest_id)
				biggest_id = ent.id;
		}

		map.entities.push_back({});
		Entity& func_group = map.entities.back();
		
		func_group.id = biggest_id + 1;
		func_group.keyvalues.push_back(std::make_pair("classname", "func_group"));
		func_group.keyvalues.push_back(std::make_pair("_tb_type", "_tb_layer"));
		func_group.keyvalues.push_back(std::make_pair("_tb_name", layer.name));
		func_group.keyvalues.push_back(std::make_pair("_tb_id", std::to_string(layer.id)));
		if (layer.hidden) {
			func_group.keyvalues.push_back(std::make_pair("_tb_layer_hidden", "1"));
			func_group.keyvalues.push_back(std::make_pair("_tb_layer_omit_from_export", "1"));
		}

		//Merge world solids
		for (auto it = map.entities[0].solids.begin(); it != map.entities[0].solids.end(); it = std::next(it))
		{
			auto& solid = *it;

			for (auto& solid_id : layer.solid_ids) {
				if (solid_id == solid.id)
				{
					func_group.solids.push_back(solid);
					
					//const auto pos = std::distance(map.entities[0].solids.begin(), it);
					map.entities[0].solids.erase(it);
					//it = std::next(map.entities[0].solids.begin(), pos);
					it = map.entities[0].solids.begin();

					break;
				}
			}
		}
	}

	for (auto& group : map.groups)
	{
		for (auto& ent : map.entities) {
			for (auto& ent_id : group.ent_ids) {
				if (ent_id == ent.id) {
					ent.keyvalues.push_back(std::make_pair("_tb_group", std::to_string(group.id)));
				}
			}
		}

		ID biggest_id = 0;
		for (auto& ent : map.entities) {
			if (ent.id > biggest_id)
				biggest_id = ent.id;
		}

		map.entities.push_back({});
		Entity& func_group = map.entities.back();

		func_group.id = biggest_id + 1;
		func_group.keyvalues.push_back(std::make_pair("classname", "func_group"));
		func_group.keyvalues.push_back(std::make_pair("_tb_type", "_tb_group"));
		func_group.keyvalues.push_back(std::make_pair("_tb_name", group.name));
		func_group.keyvalues.push_back(std::make_pair("_tb_id", std::to_string(group.id)));

		//Merge world solids
		for (auto it = map.entities[0].solids.begin(); it != map.entities[0].solids.end(); it = std::next(it))
		{
			auto& solid = *it;

			for (auto& solid_id : group.solid_ids) {
				if (solid_id == solid.id)
				{
					func_group.solids.push_back(solid);

					const auto pos = std::distance(map.entities[0].solids.begin(), it);
					map.entities[0].solids.erase(it);
					it = std::next(map.entities[0].solids.begin(), pos);

					break;
				}
			}
		}
	}

	return 0;
}
