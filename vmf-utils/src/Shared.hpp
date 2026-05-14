#pragma once

#ifndef _SHARED_HPP
#define _SHARED_HPP

#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <optional>

#include "Types.hpp"
#include "VectorMath.hpp"

#include "Logger.hpp"

extern Logger logger;

struct AppParams
{
	bool					fix_faces = false;
	std::filesystem::path	material_path;
	bool					round_values = false;
	bool					export_prefab = false;
	std::string				mat_prefix;
	bool					quake_axis_format = false; //Map uses the Quake format for texture axess
	u16						default_lightmap_scale = 16;
	bool					fix_quake_lights = false;
	bool					write_quake_lights_as_valve_format = false;
	bool					keep_special_kvs = false;
	bool					strip_mat_folder = false;
	bool					no_attrib_write = false;
};

extern AppParams app_params;

struct TexAxis
{
	Vec3 t; //Axis
	double s = .0; //Shift
	double ts = .0; //Texture scale
};

struct SideAttribs
{
	u32 contents;
	u32 flags;
	u32 value;
};

using ID = u64;

struct DispInfo
{
	u8 power = 2;
	Vec3 startposition;
	double elevation = .0;
	bool subdivided = .0;
	u64 flags = 0;
	//Can also store uaxis (Vec3), vaxis (Vec3), mintess (int), smooth (double)

	std::vector<std::vector<Vec3>> normals;
	std::vector<std::vector<double>> distances;
	std::vector<std::vector<Vec3>> offsets;
	std::vector<std::vector<Vec3>> offset_normals;
	std::vector<std::vector<double>> alphas;
	std::vector<std::vector<u8>> triangle_tags;
	std::vector<i64> allowed_verts;
};

struct Side
{
	Vec3 plane_tri[3];
	std::string material;
	TexAxis uaxis, vaxis;
	double rotation = .0;
	std::optional<SideAttribs> attribs;
	std::optional<DispInfo> dispinfo;
	ID id = 0u;

	static Side parse_quake_side(std::istringstream& ss);
	static Side parse_valve_side(std::istringstream& ss);

	void quake_to_valve_axes();
};

struct Solid
{
	std::vector<Side> sides;
	ID id = 0u;
};

using KVPair = std::pair<std::string, std::string>;

struct Entity
{
	std::vector<KVPair> keyvalues; //Classname is necessary for every entity !
	std::vector<Solid> solids; //If empty, this entity is a point entity
	ID id = 0u;
};

//I am calling it a layer, but its purpose is to be vague enough to be able to link TB layers and Hammer visgroups
struct Layer
{
	std::vector<ID> solid_ids; //IDs of the solids that are part of the layer
	std::vector<ID> ent_ids; //IDs of the entities that are part of the layer (their solid doesn't need to be added to solid_ids)
	ID id = 0u;
	std::string name;
	bool hidden = false;
};

struct Group
{
	std::vector<ID> solid_ids;
	std::vector<ID> ent_ids;
	ID id = 0u;
	std::string name; //Groups don't have names in Hammer, but it can be written in a keyvalue
};

struct Block
{
	std::string			name;
	std::vector<KVPair>	keyvalues;
	std::vector<Block>	blocks;
	Block* parent = nullptr;
};

struct Map
{
	class Parser;

	class Writer
	{
	public:
		Writer(const Map& _map)
			:map(_map)
		{ }
		~Writer() = default;

		Writer(const Writer&) = delete;
		Writer(Writer&&) = delete;
		Writer& operator=(const Writer&) = delete;
		Writer& operator=(Writer&&) = delete;

		bool write(const std::filesystem::path filename);

	private:
		void write_str(const std::string_view sv, u32 tabs = 0);
		void write_kv(const KVPair& kv, u32 tabs = 0);

		bool write_map();
		bool write_vmf();

	private:
		const Map& map;
		std::ofstream out;
	};

	std::vector<Entity> entities; //Every world brush is contained in the world entity !
	std::vector<Layer> layers;
	std::vector<Group> groups;
};

class Map::Parser
{
public:
	Parser(const std::filesystem::path& _filename)
		:filename(_filename)
	{ }
	~Parser() = default;

	Parser(const Parser&) = delete;
	Parser(Parser&&) = delete;
	Parser& operator=(const Parser&) = delete;
	Parser& operator=(Parser&&) = delete;

	Map parse();

	bool valid() { return in.good(); }

private:
	void parse_map();
	void parse_vmf();

	bool fix_quake_lights(Entity* ent);

	void parse_vmf_entity(Entity* cur_ent, Block& block);
	void parse_vmf_solid(Solid* cur_solid, Entity* cur_ent, Block& block);
private:
	std::filesystem::path filename;
	std::ifstream in;
	Map map;
};

extern KVPair* find_kv(std::vector<KVPair>& vec, const std::string_view key);
extern const KVPair* find_kv(const std::vector<KVPair>& vec, const std::string_view key);

#endif // !_SHARED_HPP
