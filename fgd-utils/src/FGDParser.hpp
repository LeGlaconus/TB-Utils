#pragma once

#ifndef _FGDPARSER_HPP
#define _FGDPARSER_HPP

#include <vector>
#include <string>
#include <string_view>
#include <sstream>
#include <fstream>
#include <optional>
#include <any>

#include "Logger.hpp"
#include "Tokenizer.hpp"
#include "Types.hpp"

extern Logger logger;

struct ClassProperty
{
	struct Value;

	struct TrivialValue
	{
		TrivialValue()
			:_type(Type::Invalid)
		{ }
		TrivialValue(const Value& rhs)
		{
			switch (rhs.type())
			{
			default:
			case				Value::Type::Invalid:
				_type =	 TrivialValue::Type::Invalid; break;
			case				Value::Type::String:
				_type =  TrivialValue::Type::String; break;
			case				Value::Type::Integer:
				_type =  TrivialValue::Type::Integer; break;
			case				Value::Type::Float:
				_type =  TrivialValue::Type::Float; break;
			case				Value::Type::Boolean:
				_type =  TrivialValue::Type::Boolean; break;
			}

			value = rhs.value;
		}
		~TrivialValue() = default;

		enum class Type : u8
		{
			Invalid,
			String,
			Integer,
			Float,
			Boolean,
		};
		
		std::any value;

		TrivialValue::Type& type() { return _type; }
		const TrivialValue::Type& type() const { return _type; }

		static TrivialValue get_from_token(const Tokenizer::Token& tok);
		static TrivialValue::Type type_from_string(const std::string_view);

		std::string value_as_string() const;
	
	private:
		Type _type;
	};

	struct Value : TrivialValue
	{
		Value()
			:_type(Type::Invalid)
		{ }
		Value(const TrivialValue& rhs)
		{
			switch (rhs.type())
			{
			default:
			case TrivialValue::Type::Invalid:
				_type = Value::Type::Invalid; break;
			case TrivialValue::Type::String:
				_type = Value::Type::String; break;
			case TrivialValue::Type::Integer:
				_type = Value::Type::Integer; break;
			case TrivialValue::Type::Float:
				_type = Value::Type::Float; break;
			case TrivialValue::Type::Boolean:
				_type = Value::Type::Boolean; break;
			}

			value = rhs.value;
		}
		~Value() = default;

		enum class Type : u8
		{
			Invalid,
			String,
			Integer,
			Float,
			Boolean,
			Flags,
			Choices,

			Angle, //Number string
			AngleNegativePitch, //Number string
			Axis, //Number string, comma separated
			Color255, //Number string
			Color1, //Number string
			Decal, //String
			FilterClass, //String
			InstanceFile, //String
			InstanceParm, //String
			InstanceVariable, //String
			Material, //String
			NodeDest, //Integer
			NodeID, //Integer (educated guess)
			NPCClass, //String
			Origin, //Number string
			ParticleSystem, //String
			PointEntityClass, //String
			Scene, //String
			Script, //String
			ScriptList, //String
			SideList, //Number string, space separated
			Sound, //String
			Sprite, //String
			Studio, //String
			TargetDestination, //String
			TargetNameOrClass, //String
			TargetSource, //String
			VecLine, //Number string
			Vector, //Number string, space separated
		};

		struct Flag
		{
			i32 value;
			std::string name;
			bool default_ticked;
		};

		struct Choice
		{
			TrivialValue value;
			std::string display_name;
		};

		using Flags = std::vector<Flag>;
		using Choices = std::vector<Choice>;

		Value::Type& type() { return _type; }
		const Value::Type& type() const { return _type; }

		static Value get_from_token(const Tokenizer::Token& tok);
		static Value::Type type_from_string(const std::string_view);
		static std::string string_from_type(const Value::Type);
	
		bool choices_from_tokenizer(Tokenizer& tokenizer);
		bool flags_from_tokenizer(Tokenizer& tokenizer);

		std::string value_as_string() const;

	private:
		Type _type;
	};

	std::string name;
	Value::Type type;
	struct Modifiers
	{
		bool readonly : 1;
		bool report : 1;
	} mods; //Optional
	std::string display_name; //Optional
	std::optional<Value> default_value; //Optional
	std::string description; //Optional

	std::optional<Value> choice_like_value; //Choices/Flags
};

struct IOProperty
{
	enum class Type : u8
	{
		Input = 0,
		Output
	};

	enum class ValueType : u8
	{
		Invalid,
		Void,
		String,
		Integer,
		Float,
		Bool,
		EHandle,

		TargetDestination,
		Color255,
		Vector,
	};

	static ValueType value_type_from_string(const std::string_view);
	static std::string string_from_value_type(const ValueType);

	Type type;
	ValueType value_type;
	std::string name;
	std::string description;
};

struct Helper
{
	std::string name;
	std::vector<std::string> tokens;
};

struct Class
{
	enum class Type : u8
	{
		PointClass,
		SolidClass,
		NPCClass,
		KeyFrameClass,
		MoveClass,
		FilterClass,

		BaseClass,
	};

	std::vector<Helper> helpers;
	std::string name;
	std::string description;
	std::vector<ClassProperty> properties;
	std::vector<IOProperty> io_properties;
	Type type;
};

enum class FormatType : u8
{
	Hammer,
	Trenchbroom
};

struct FGD
{
	class Parser;

	class Writer
	{
	public:
		Writer(const FGD& _fgd)
			:fgd(_fgd), format_type(FormatType::Trenchbroom)
		{ }
		Writer(FGD& _fgd, FormatType _format_type)
			:fgd(_fgd), format_type(_format_type)
		{ }
		~Writer() = default;

		Writer(const Writer&) = delete;
		Writer(Writer&&) = delete;
		Writer& operator=(const Writer&) = delete;
		Writer& operator=(Writer&&) = delete;

		bool write(const std::filesystem::path filename);

	private:
		void write_str(const std::string_view& sv, u32 tabs = 0);
		void write_nl();

		void write_choices(const ClassProperty::Value::Choices& choices);
		void write_flags(const ClassProperty::Value::Flags& flags);
		void write_clprop(const ClassProperty& cp);
		void write_ioprop(const IOProperty& prop);

		void write_class(const Class& cl);

	private:
		std::ofstream out;
		const FGD& fgd;
		FormatType format_type;
	};

	FGD()
		:valid(false)
	{ }
	~FGD() = default;

	std::optional<std::pair<i32, i32>> mapsize;
	std::vector<std::string> includes;
	std::vector<Class> classes;

	bool valid;
};

class FGD::Parser
{
public:

	Parser(std::filesystem::path _filename)
		:filename(_filename), tokenizer(_filename, splitters)
	{
		if (!tokenizer.valid()) {
			logger.error(std::format("Couldn't read {}", filename.string()), 0);
			return;
		}
	}
	~Parser() = default;

	Parser(const Parser&) = delete;
	Parser(Parser&&) = delete;
	Parser& operator=(const Parser&) = delete;
	Parser& operator=(Parser&&) = delete;

	FGD parse();

	bool valid() { return tokenizer.valid(); }

private:
	bool parse_keyvalues(Class& bc);

private:
	std::ifstream in;
	FGD fgd;
	std::filesystem::path filename;
	Tokenizer tokenizer;

	static constexpr char splitters[] = { '[', ']', ':', '(', ')', ',', '+' };

	using Token = Tokenizer::Token;
};

struct AppParams
{
	bool recurse_includes = false;
	std::filesystem::path output_folder;
	std::string spr_extension = "tga";
	std::string mdl_extension = "smd";
};

extern AppParams app_params;

bool handle_file(std::filesystem::path filename);

#endif // !_FGDPARSER_HPP