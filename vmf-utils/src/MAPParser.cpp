#include "MAPParser.hpp"

#include <iostream>
#include <fstream>
#include <format>

//This is from QBSP
void Side::quake_to_valve_axes()
{
	static constexpr Vec3 base_axis[] =
	{
		Vec3(  .0,	  .0,	 1.0),	Vec3(1.0,	 .0,	.0),	Vec3(.0,	-1.0,	  .0),	// Floor
		Vec3(  .0,	  .0,	-1.0),	Vec3(1.0,	 .0,	.0),	Vec3(.0,	-1.0,	  .0),	// Ceiling
		Vec3( 1.0,	  .0,	  .0),	Vec3( .0,	1.0,	.0),	Vec3(.0,	  .0,	-1.0),	// West wall
		Vec3(-1.0,	  .0,	  .0),	Vec3( .0,	1.0,	.0),	Vec3(.0,	  .0,	-1.0),	// East wall
		Vec3(  .0,	 1.0,	  .0),	Vec3(1.0,	 .0,	.0),	Vec3(.0,	  .0,	-1.0),	// South wall
		Vec3(  .0,	-1.0,	  .0),	Vec3(1.0,	 .0,	.0),	Vec3(.0,	  .0,	-1.0)	// North wall
	};

	double best = .0;
	u8 best_axis = 0;

	for (u8 i = 0; i < 6; i++)
	{
		const double dot = GetNormalFromPlane(plane_tri[0], plane_tri[1], plane_tri[2]).DotProduct(base_axis[i * 3]);
		if (dot > best) {
			best = dot;
			best_axis = i;
		}
	}

	uaxis.t = base_axis[best_axis * 3 + 1];
	vaxis.t = base_axis[best_axis * 3 + 2];
}

Side Side::parse_quake_side(std::istringstream& ss)
{
	Side side;
	std::string dummy;

	for (u8 i = 0; i < 3; i++)
	{
		ss >> dummy;
		ss >> side.plane_tri[i].x >> side.plane_tri[i].y >> side.plane_tri[i].z;
		ss >> dummy;
	}

	ss >> side.material;

	//shift shift rotation scale scale

	ss >> side.uaxis.s >> side.vaxis.s;
	ss >> side.rotation;
	ss >> side.uaxis.ts >> side.vaxis.ts;

	if (!ss.eof())
	{
		//Handle contents, flags and values
		//Thankfully TB already exports in base 10
		side.attribs.emplace();
		ss >> side.attribs.value().contents;
		ss >> side.attribs.value().flags;
		ss >> side.attribs.value().value;
	}

	side.quake_to_valve_axes();

	return side;
}

Side Side::parse_valve_side(std::istringstream& ss)
{
	Side side;
	std::string dummy;

	for (u8 i = 0; i < 3; i++)
	{
		ss >> dummy;
		ss >> side.plane_tri[i].x >> side.plane_tri[i].y >> side.plane_tri[i].z;
		ss >> dummy;
	}

	ss >> side.material;

	ss >> dummy;
	ss >> side.uaxis.t.x >> side.uaxis.t.y >> side.uaxis.t.z >> side.uaxis.s;
	ss >> dummy;

	ss >> dummy;
	ss >> side.vaxis.t.x >> side.vaxis.t.y >> side.vaxis.t.z >> side.vaxis.s;
	ss >> dummy;

	ss >> side.rotation;

	ss >> side.uaxis.ts;
	ss >> side.vaxis.ts;

	if (!ss.eof())
	{
		//Handle contents, flags and values
		//Thankfully TB already exports in base 10
		side.attribs.emplace();
		ss >> side.attribs.value().contents;
		ss >> side.attribs.value().flags;
		ss >> side.attribs.value().value;
	}

	return side;
}

//TODO : Parse displacements, even though there are 3 competing formats and none of them are suported
//https://github.com/ValveSoftware/source-sdk-2013/blob/3300848d8a25ef6403c91f82a4cd97d6daefbc06/src/utils/vbsp/map.cpp#L3210-L3326
static inline void parse_disp()
{

}

void Map::Parser::parse_map()
{
	//Depth in curly bracket sections, depth 0 : in nothing, depth 1 : in an entity, depth 2 : in a solid
	u8 depth = 0;

	Entity* cur_entity = nullptr;
	Solid* cur_solid = nullptr;

	char buf[1024];
	buf[1023] = '\0';
	while (in.good())
	{
		in.getline(buf, 1023);
		const std::string_view line{ buf };

		if (line.starts_with("//")) {
			continue;
		}
		else if (line.starts_with("{"))
		{
			depth++;

			if (depth == 1)
			{
				static ID ent_id = 0;

				if (app_params.fix_quake_lights)
					fix_quake_lights(cur_entity);

				map.entities.push_back({});
				cur_entity = &map.entities.back();
				cur_entity->id = ent_id;
				ent_id++;
			}
			else if (depth == 2)
			{
				//Entity id starts at 0, every other at 1
				static ID solid_id = 0;
				solid_id++;
				cur_entity->solids.push_back({});
				cur_solid = &cur_entity->solids.back();
				cur_solid->id = solid_id;
			}
		}
		else if (line.starts_with("}"))
		{
			depth--;

			if (depth == 0)
				cur_entity = nullptr;
			else if (depth == 1)
				cur_solid = nullptr;
		}

		if (depth == 0) {
			continue; //Currently in nothing, do nothing
		}
		else if (depth == 1)
		{
			//Currently in an entity
			if (line.starts_with("{") || line.starts_with("}"))
				continue; //No keyvalues on this line

			std::istringstream line_ss{ std::string{line} };
			KVPair kv;
			line_ss >> kv.first;

			//Assuming here that the key can't have spaces but the value can
			kv.second = line_ss.str().substr(static_cast<std::size_t>(line_ss.tellg()) + 1);

			kv.first = kv.first.substr(1, kv.first.size() - 2);
			kv.second = kv.second.substr(1, kv.second.size() - 2);

			cur_entity->keyvalues.push_back(kv);
		}
		else if (depth == 2)
		{
			//Currently in a solid
			if (line.starts_with("{") || line.starts_with("}"))
				continue; //No data on this line

			std::istringstream line_ss{ std::string{line} };
			
			auto side = app_params.quake_axis_format ? Side::parse_quake_side(line_ss) : Side::parse_valve_side(line_ss);

			static ID side_id = 0;
			side_id++;
			side.id = side_id;

			cur_solid->sides.push_back(side);
		}
	}
}

bool Map::Parser::fix_quake_lights(Entity* ent)
{
	auto const classname_kv = find_kv(ent->keyvalues, "classname");
	if (!classname_kv)
		return false;
	
	if (!classname_kv->second.starts_with("light"))
		return false;
	
	auto light_kv = find_kv(ent->keyvalues, "light");
	if (light_kv) {
		light_kv->first = "_light";
		//VRAD supports grayscale lights, but there is still an option to write 255 255 255 <value>
		if (app_params.write_quake_lights_as_valve_format)
			light_kv->second = std::format("255 255 255 {}", light_kv->second);
	}

	return true;
}

u32 handle_trenchbroom_layers(Map& map)
{
	for (auto it = map.entities.begin(); it != map.entities.end(); it = std::next(it))
	{
		auto& ent = *it;

		auto const classname_kv = find_kv(ent.keyvalues, "classname");
		if (!classname_kv)
			continue; //Technically not supposed to happen but you never know

		if (classname_kv->second != "func_group")
			continue;

		auto const tb_type_kv = find_kv(ent.keyvalues, "_tb_type");
		if (!tb_type_kv) {
			logger.warn("func_group doesn't have a _tb_type !");
			continue;
		}

		if (tb_type_kv->second == "_tb_layer")
		{
			map.layers.push_back({});
			Layer* cur_layer = &map.layers.back();

			{
				auto name_kv = find_kv(ent.keyvalues, "_tb_name");
				if (name_kv)
					cur_layer->name = name_kv->second;

				auto id_kv = find_kv(ent.keyvalues, "_tb_id");
				if (id_kv)
					cur_layer->id = static_cast<ID>(std::stoll(id_kv->second));

				auto omit_from_export_kv = find_kv(ent.keyvalues, "_tb_layer_omit_from_export");
				auto hidden_kv = find_kv(ent.keyvalues, "_tb_layer_hidden");
				if (omit_from_export_kv && (omit_from_export_kv->second == "1"))
					cur_layer->hidden = true;
				else if (hidden_kv && (hidden_kv->second == "1"))
					cur_layer->hidden = true;
			}

			for (auto& solid : ent.solids) {
				cur_layer->solid_ids.push_back(solid.id);
			}

			//Add the entities from the layer, requires searching through the map again
			for (auto& lent : map.entities)
			{
				auto layer_id_kv = find_kv(lent.keyvalues, "_tb_layer");
				if (!layer_id_kv)
					continue;

				ID layer_id = static_cast<ID>(std::stoll(layer_id_kv->second));
				if (layer_id == cur_layer->id)
					cur_layer->ent_ids.push_back(lent.id);
			}
		}
		else if (tb_type_kv->second == "_tb_group")
		{
			map.groups.push_back({});
			Group* cur_group = &map.groups.back();

			{
				auto name_kv = find_kv(ent.keyvalues, "_tb_name");
				if (name_kv)
					cur_group->name = name_kv->second;

				auto id_kv = find_kv(ent.keyvalues, "_tb_id");
				if (id_kv)
					cur_group->id = static_cast<ID>(std::stoll(id_kv->second));
			}

			for (auto& solid : ent.solids) {
				cur_group->solid_ids.push_back(solid.id);
			}

			//Add the entities from the group, requires searching through the map again
			for (auto& lent : map.entities)
			{
				auto group_id_kv = find_kv(lent.keyvalues, "_tb_group");
				if (!group_id_kv)
					continue;

				ID group_id = static_cast<ID>(std::stoll(group_id_kv->second));
				if (group_id == cur_group->id)
					cur_group->ent_ids.push_back(lent.id);
			}
		}
		else {
			logger.warn(std::format("Unsupported _tb_type \"{}\"", tb_type_kv->second));
			continue;
		}

		//Move the solids from this func_group to the world
		map.entities[0].solids.insert(map.entities[0].solids.end(), ent.solids.begin(), ent.solids.end());
		//const auto pos = std::distance(map.entities.begin(), it);
		map.entities.erase(it); //Erase this func_group, and naturally, its solids
		//it = std::next(map.entities.begin(), pos - 1);
		it = map.entities.begin();
	}

	return 0;
}

bool Map::Writer::write_map()
{
	write_str("//Generated by vmf-utils\n");
	write_str("//Format: Quake II (Valve)\n"); //TODO: this might change

	for (auto& ent : map.entities)
	{
		write_str(std::format("//entity n°{}\n", ent.id));

		write_str("{\n");

		for (auto& kv : ent.keyvalues) {
			write_kv(kv);
		}

		for (auto& solid : ent.solids)
		{
			write_str(std::format("//solid n°{}\n", solid.id));

			write_str("{\n");

			for (auto& side : solid.sides)
			{
				for (u8 i = 0; i < 3; i++) {
					write_str(std::format("( {} {} {} ) ", side.plane_tri[i].x , side.plane_tri[i].y, side.plane_tri[i].z));
				}

				write_str(side.material);

				write_str(std::format(" [ {} {} {} {} ] [ {} {} {} {} ] {} {} {}\n",	
							side.uaxis.t.x, side.uaxis.t.y, side.uaxis.t.z, side.uaxis.s,
							side.vaxis.t.x, side.vaxis.t.y, side.vaxis.t.z, side.vaxis.s,
							side.rotation, side.uaxis.ts, side.vaxis.ts));
				
				if (side.attribs.has_value() && !app_params.no_attrib_write) {
					const auto& attribs = side.attribs.value();
					write_str(std::format(" {} {} {}\n", attribs.contents, attribs.flags, attribs.value));
				}
			}

			write_str("}\n");
		}

		write_str("}\n");
	}

	return true;
}
