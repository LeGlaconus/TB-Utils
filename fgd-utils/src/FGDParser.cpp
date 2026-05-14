#include <iostream>
#include <array>
#include <filesystem>
#include <format>

#include "FGDParser.hpp"

ClassProperty::TrivialValue ClassProperty::TrivialValue::get_from_token(const Tokenizer::Token& tok)
{
	using CVal = ClassProperty::TrivialValue;
	using CValType = CVal::Type;
	CVal val;

	if (tok.is_quoted())
	{
		//Can be a string or a float
		auto qtok = tok.quoteless();
		
		if (qtok == "0") {
			val.type() = CValType::Float; //Hammer doesn't allow quoted integers but TB does
										  //TODO: this is currently Hammer-TB conversion, but for the opposite, change that behavior
										  //BaseHelicopter takes a value of "0" for a string... it shouldn't even be a string.....
			val.value = std::make_any<double>(static_cast<double>(.0));
		
			return val;
		}

		if (qtok == "0.0") {
			val.type() = CValType::Float;
			val.value = std::make_any<double>(static_cast<double>(.0));

			return val;
		}

		if (val.type() == CValType::Invalid) {
			double maybe_float = std::atof(qtok.c_str());
			val.type() = (maybe_float == .0) ? CValType::String : CValType::Float;

			if (val.type() == CValType::String) val.value = std::make_any<std::string>(std::string{ qtok });
			else val.value = std::make_any<double>(maybe_float);

			return val;
		}
	}

	for (auto& c : tok.str) {
		if (!std::isdigit(c) && c != '-') {
			val.type() = CValType::Invalid;

			return val;
		}
	}

	val.type() = CValType::Integer;
	val.value = std::make_any<i32>(static_cast<i32>(std::stoll(tok.str)));

	return val;
}

//TODO: move it
static inline ClassProperty::TrivialValue::Type get_trivial_type_equivalent(const ClassProperty::Value::Type type)
{
	using TrivialType = ClassProperty::TrivialValue::Type;
	using ComplexType = ClassProperty::Value::Type;

	static constexpr std::pair<ComplexType, TrivialType> map[]
	{
		{ComplexType::Invalid,				TrivialType::Invalid},
		{ComplexType::String,				TrivialType::String},
		{ComplexType::Integer,				TrivialType::Integer},
		{ComplexType::Float,				TrivialType::Float},
		{ComplexType::Boolean,				TrivialType::Boolean},
		{ComplexType::Flags,				TrivialType::Invalid}, //ssdfg
		{ComplexType::Choices,				TrivialType::Invalid}, //ssdfg
		{ComplexType::Angle,				TrivialType::String},
		{ComplexType::AngleNegativePitch,	TrivialType::String},
		{ComplexType::Axis,					TrivialType::String},
		{ComplexType::Color255,				TrivialType::String},
		{ComplexType::Color1,				TrivialType::String},
		{ComplexType::Decal,				TrivialType::String},
		{ComplexType::FilterClass,			TrivialType::String},
		{ComplexType::InstanceFile,			TrivialType::String},
		{ComplexType::InstanceParm,			TrivialType::String},
		{ComplexType::InstanceVariable,		TrivialType::String},
		{ComplexType::Material,				TrivialType::String},
		{ComplexType::NodeDest,				TrivialType::Integer},
		{ComplexType::NodeID,				TrivialType::Integer},
		{ComplexType::NPCClass,				TrivialType::String},
		{ComplexType::Origin,				TrivialType::String},
		{ComplexType::ParticleSystem,		TrivialType::String},
		{ComplexType::PointEntityClass,		TrivialType::String},
		{ComplexType::Scene,				TrivialType::String},
		{ComplexType::Script,				TrivialType::String},
		{ComplexType::ScriptList,			TrivialType::String},
		{ComplexType::SideList,				TrivialType::String},
		{ComplexType::Sound,				TrivialType::String},
		{ComplexType::Sprite,				TrivialType::String},
		{ComplexType::Studio,				TrivialType::String},
		{ComplexType::TargetDestination,	TrivialType::String},
		{ComplexType::TargetNameOrClass,	TrivialType::String},
		{ComplexType::TargetSource,			TrivialType::String},
		{ComplexType::VecLine,				TrivialType::String},
		{ComplexType::Vector,				TrivialType::String},
	};

	for (auto& e : map)
	{
		if (type == e.first)
			return e.second;
	}

	return TrivialType::Invalid;
}

static inline ClassProperty::Value::Type get_tb_type_equivalent(const ClassProperty::Value::Type type)
{
	using TrivialType = ClassProperty::TrivialValue::Type;
	using ComplexType = ClassProperty::Value::Type;

	static constexpr std::pair<ComplexType, ComplexType> map[]
	{
		{ComplexType::Invalid,				ComplexType::Invalid},
		{ComplexType::String,				ComplexType::String},
		{ComplexType::Integer,				ComplexType::Integer},
		{ComplexType::Float,				ComplexType::Float},
		{ComplexType::Boolean,				ComplexType::Boolean},
		{ComplexType::Flags,				ComplexType::Flags},
		{ComplexType::Choices,				ComplexType::Choices},
		{ComplexType::Angle,				ComplexType::String},
		{ComplexType::AngleNegativePitch,	ComplexType::String},
		{ComplexType::Axis,					ComplexType::String},
		{ComplexType::Color255,				ComplexType::Color255}, //Supported
		{ComplexType::Color1,				ComplexType::Color1}, //Supported
		{ComplexType::Decal,				ComplexType::String},
		{ComplexType::FilterClass,			ComplexType::String},
		{ComplexType::InstanceFile,			ComplexType::String},
		{ComplexType::InstanceParm,			ComplexType::String},
		{ComplexType::InstanceVariable,		ComplexType::String},
		{ComplexType::Material,				ComplexType::String},
		{ComplexType::NodeDest,				ComplexType::Integer},
		{ComplexType::NodeID,				ComplexType::Integer},
		{ComplexType::NPCClass,				ComplexType::String},
		{ComplexType::Origin,				ComplexType::Origin}, //Supported
		{ComplexType::ParticleSystem,		ComplexType::String},
		{ComplexType::PointEntityClass,		ComplexType::String},
		{ComplexType::Scene,				ComplexType::String},
		{ComplexType::Script,				ComplexType::String},
		{ComplexType::ScriptList,			ComplexType::String},
		{ComplexType::SideList,				ComplexType::String},
		{ComplexType::Sound,				ComplexType::String},
		{ComplexType::Sprite,				ComplexType::String},
		{ComplexType::Studio,				ComplexType::String},
		{ComplexType::TargetDestination,	ComplexType::TargetDestination}, //Supported
		{ComplexType::TargetNameOrClass,	ComplexType::String},
		{ComplexType::TargetSource,			ComplexType::TargetSource}, //Supported
		{ComplexType::VecLine,				ComplexType::String},
		{ComplexType::Vector,				ComplexType::String},
	};

	for (auto& e : map)
	{
		if (type == e.first)
			return e.second;
	}

	return ComplexType::Invalid;
}

ClassProperty::Value ClassProperty::Value::get_from_token(const Tokenizer::Token& tok)
{
	//TODO
	return ClassProperty::TrivialValue::get_from_token(tok);
}

ClassProperty::TrivialValue::Type ClassProperty::TrivialValue::type_from_string(const std::string_view sv)
{
	using CValType = ClassProperty::TrivialValue::Type;

	static constexpr std::pair<std::string_view, CValType> map[]
	{
		{"string", CValType::String},
		{"integer", CValType::Integer},
		{"float", CValType::Float},
		{"boolean", CValType::Boolean},
	};

	std::string lsv{ sv };
	for (auto& c : lsv) {
		c = std::tolower(c);
	}

	for (auto& e : map)
	{
		if (e.first == lsv)
			return e.second;
	}

	return CValType::Invalid;
}

ClassProperty::Value::Type ClassProperty::Value::type_from_string(const std::string_view sv)
{
	using CValType = ClassProperty::Value::Type;

	static constexpr std::pair<std::string_view, CValType> map[]
	{
		{"string",					CValType::String},
		{"integer",					CValType::Integer},
		{"float",					CValType::Float},
		{"boolean",					CValType::Boolean},
		{"flags",					CValType::Flags},
		{"choices",					CValType::Choices},

		{"angle",					CValType::Angle},
		{"angle_negative_pitch",	CValType::AngleNegativePitch},
		{"axis",					CValType::Axis},
		{"color255",				CValType::Color255},
		{"color1",					CValType::Color1},
		{"decal",					CValType::Decal},
		{"filterclass",				CValType::FilterClass},
		{"instance_file",			CValType::InstanceFile},
		{"instance_parm",			CValType::InstanceParm},
		{"instance_variable",		CValType::InstanceVariable},
		{"material",				CValType::Material},
		{"node_dest",				CValType::NodeDest},
		{"node_id",					CValType::NodeID},
		{"npcclass",				CValType::NPCClass},
		{"origin",					CValType::Origin},
		{"particlesystem",			CValType::ParticleSystem},
		{"pointentityclass",		CValType::PointEntityClass},
		{"scene",					CValType::Scene},
		{"script",					CValType::Script},
		{"scriptlist",				CValType::ScriptList},
		{"sidelist",				CValType::SideList},
		{"sound",					CValType::Sound},
		{"sprite",					CValType::Sprite},
		{"studio",					CValType::Studio},
		{"target_destination",		CValType::TargetDestination},
		{"target_name_or_class",	CValType::TargetNameOrClass},
		{"target_source",			CValType::TargetSource},
		{"vecline",					CValType::VecLine},
		{"vector",					CValType::Vector},
	};

	std::string lsv{ sv };
	for (auto& c : lsv) {
		c = std::tolower(c);
	}

	for (auto& e : map)
	{
		if (e.first == lsv)
			return e.second;
	}

	return CValType::Invalid;
}

std::string ClassProperty::Value::string_from_type(const ClassProperty::Value::Type val)
{
	using CValType = ClassProperty::Value::Type;

	static constexpr std::pair<std::string_view, CValType> map[]
	{
		{"string",					CValType::String},
		{"integer",					CValType::Integer},
		{"float",					CValType::Float},
		{"boolean",					CValType::Boolean},
		{"flags",					CValType::Flags},
		{"choices",					CValType::Choices},

		{"angle",					CValType::Angle},
		{"angle_negative_pitch",	CValType::AngleNegativePitch},
		{"axis",					CValType::Axis},
		{"color255",				CValType::Color255},
		{"color1",					CValType::Color1},
		{"decal",					CValType::Decal},
		{"filterclass",				CValType::FilterClass},
		{"instance_file",			CValType::InstanceFile},
		{"instance_parm",			CValType::InstanceParm},
		{"instance_variable",		CValType::InstanceVariable},
		{"material",				CValType::Material},
		{"node_dest",				CValType::NodeDest},
		{"node_id",					CValType::NodeID},
		{"npcclass",				CValType::NPCClass},
		{"origin",					CValType::Origin},
		{"particlesystem",			CValType::ParticleSystem},
		{"pointentityclass",		CValType::PointEntityClass},
		{"scene",					CValType::Scene},
		{"script",					CValType::Script},
		{"scriptlist",				CValType::ScriptList},
		{"sidelist",				CValType::SideList},
		{"sound",					CValType::Sound},
		{"sprite",					CValType::Sprite},
		{"studio",					CValType::Studio},
		{"target_destination",		CValType::TargetDestination},
		{"target_name_or_class",	CValType::TargetNameOrClass},
		{"target_source",			CValType::TargetSource},
		{"vecline",					CValType::VecLine},
		{"vector",					CValType::Vector},
	};

	for (auto& e : map)
	{
		if (e.second == val)
			return std::string{ e.first };
	}

	return "invalid";
}


static inline bool expect_token(const std::string_view expected, Tokenizer::Token& tok, bool print = true)
{
	if (tok.str != expected) {
		if (print)
			logger.error(std::format("Expecting {} but getting {} ({})", expected, tok.str, tok.location()), 0);
		return false;
	}

	return true;
}

static inline bool check_end(auto& it, auto& vec)
{
	if (it == vec.end())
	{
		logger.error("Reached an unexpected in the tokens");
		return true;
	}

	return false;
}

bool ClassProperty::Value::choices_from_tokenizer(Tokenizer& tokenizer)
{
	using Token = Tokenizer::Token;

	Token tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
	if (!expect_token("=", tok)) return false;
	tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
	if (!expect_token("[", tok)) return false;

	Choices choices;

	for (tok = tokenizer.get_next_token(); !expect_token("]", tok, false); tok = tokenizer.get_next_token())
	{
		if (!tokenizer.valid()) return false;

		choices.push_back({});
		Choice& cur_choice = choices.back();
		cur_choice.value = TrivialValue::get_from_token(tok);
		if (cur_choice.value.type() == TrivialValue::Type::Invalid) {
			logger.error(std::format("Getting an invalid type from token {} ({})", tok.str, tok.location()), 0);
			return false;
		}

		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
		if (!expect_token(":", tok)) return false;

		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;

		if (!tok.is_quoted())
		{
			logger.error(std::format("Expecting a quoted display name but got {} ({})", tok.str, tok.location()), 0);
			return false;
		}

		cur_choice.display_name = tok.quoteless();
	}

	value = std::make_any<Choices>(choices);
	type() = Type::Choices;

	return true;
}

bool ClassProperty::Value::flags_from_tokenizer(Tokenizer& tokenizer)
{
	using Token = Tokenizer::Token;

	Token tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
	if (!expect_token("=", tok)) return false;
	tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
	if (!expect_token("[", tok)) return false;

	Flags flags;

	for (tok = tokenizer.get_next_token(); !expect_token("]", tok, false); tok = tokenizer.get_next_token())
	{
		if (!tokenizer.valid()) return false;

		flags.push_back({});
		Flag& cur_flag = flags.back();
		cur_flag.value = static_cast<i32>(std::stoll(tok.str));

		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
		if (!expect_token(":", tok)) return false;

		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;

		if (!tok.is_quoted())
		{
			logger.error(std::format("Expecting a quoted name but got {} ({})", tok.str, tok.location()), 0);
			return false;
		}

		cur_flag.name = tok.quoteless();

		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
		if (!expect_token(":", tok)) return false;

		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
			 if (tok.str == "1") cur_flag.default_ticked = true;
		else if (tok.str == "0") cur_flag.default_ticked = false;
		else
		{
			logger.warn(std::format("Expected 0/1 but got {} ({})", tok.str, tok.location()));
			if (static_cast<i32>(std::stoll(tok.str)) > 1) {
				cur_flag.default_ticked = true;
				logger.warn("Accepting this value because it is a valid integer.");
			}
			else {
				return false;
			}
		}
	}

	value = std::make_any<Flags>(flags);
	type() = Type::Flags;

	return true;
}

std::string ClassProperty::TrivialValue::value_as_string() const
{
	std::string str;

	if (type() == ClassProperty::TrivialValue::Type::Boolean) {
		const bool v = std::any_cast<bool>(value);
		str = v ? "1" : "0";
	}
	else if (type() == ClassProperty::TrivialValue::Type::String) {
		const std::string v = std::any_cast<std::string>(value);
		str = std::format("\"{}\"", v);
	}
	else if (type() == ClassProperty::TrivialValue::Type::Integer) {
		const i32 v = std::any_cast<i32>(value);
		str = std::to_string(v);
	}
	else if (type() == ClassProperty::TrivialValue::Type::Float) {
		const double v = std::any_cast<double>(value);
		str = std::to_string(v);
	}

	str += " ";

	return str;
}

std::string ClassProperty::Value::value_as_string() const
{
	const auto eq = get_trivial_type_equivalent(type());

	std::string str;

	if (eq == ClassProperty::TrivialValue::Type::Boolean) {
		const bool v = std::any_cast<bool>(value);
		str = v ? "1" : "0";
	}
	else if (eq == ClassProperty::TrivialValue::Type::String) {
		const std::string v = std::any_cast<std::string>(value);
		str = std::format("\"{}\"", v);
	}
	else if (eq == ClassProperty::TrivialValue::Type::Integer) {
		const i32 v = std::any_cast<i32>(value);
		str = std::format("{}", v);
	}
	else if (eq == ClassProperty::TrivialValue::Type::Float) {
		const double v = std::any_cast<double>(value);
		str = std::format("\"{}\"", v);
	}

	return str;
}

IOProperty::ValueType IOProperty::value_type_from_string(std::string_view sv)
{
	using CValType = IOProperty::ValueType;

	static constexpr std::pair<std::string_view, CValType> map[]
	{
		{"void", CValType::Void},
		{"string", CValType::String},
		{"integer", CValType::Integer},
		{"float", CValType::Float},
		{"bool", CValType::Bool},
		{"boolean", CValType::Bool}, //GMOD thing !
		{"ehandle", CValType::EHandle},

		{"target_destination", CValType::TargetDestination},
		{"color255", CValType::Color255},
		{"vector", CValType::Vector},
	};

	std::string lsv{ sv };
	for (auto& c : lsv) {
		c = std::tolower(c);
	}

	for (auto& e : map)
	{
		if (e.first == lsv)
			return e.second;
	}

	return CValType::Invalid;
}

std::string IOProperty::string_from_value_type(const ValueType val)
{
	using CValType = IOProperty::ValueType;

	static constexpr std::pair<std::string_view, CValType> map[]
	{
		{"void",				CValType::Void},
		{"string",				CValType::String},
		{"integer",				CValType::Integer},
		{"float",				CValType::Float},
		{"bool",				CValType::Bool},
	  //{"boolean",				CValType::Bool}, //GMOD thing !
		{"ehandle",				CValType::EHandle},

		{"target_destination",	CValType::TargetDestination},
		{"color255",			CValType::Color255},
		{"vector",				CValType::Vector},
	};

	for (auto& e : map)
	{
		if (e.second == val)
			return std::string{ e.first };
	}

	return "invalid";
}

bool FGD::Parser::parse_keyvalues(Class& bc)
{
	for (Token tok = tokenizer.get_next_token(); !expect_token("]", tok, false); tok = tokenizer.get_next_token())
	{
		if (!tokenizer.valid()) return false;

		//An optional "key" can exist
		if(tok.lowercase() == "key") {
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
		}
		else if (tok.lowercase() == "input" || tok.lowercase() == "output")
		{
			bc.io_properties.push_back({});
			IOProperty& ip = bc.io_properties.back();
			ip.type = (tok.str == "input") ? IOProperty::Type::Input : IOProperty::Type::Output;

			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
			ip.name = tok.str;

			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
			if (!expect_token("(", tok)) return false;
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;

			ip.value_type = IOProperty::value_type_from_string(tok.str);
			if (ip.value_type == IOProperty::ValueType::Invalid) {
				logger.error(std::format("Getting invalid IO value type from {} ({})", tok.str, tok.location()), 0);
				return false;
			}

			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
			if (!expect_token(")", tok)) return false;

			tok = tokenizer.peek_next_token();

			if (expect_token(":", tok, false))
			{
				tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
				tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;

				if (!tok.is_quoted())
				{
					logger.error(std::format("Expecting a quoted description but got {} ({})", tok.str, tok.location()), 0);
					return false;
				}

				ip.description = tok.quoteless();

				//Check for + string concatenation
				for (tok = tokenizer.peek_next_token(); expect_token("+", tok, false); tok = tokenizer.peek_next_token())
				{
					if (!tokenizer.valid()) return false;

					tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
					tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
					if (!tok.is_quoted())
					{
						logger.error(std::format("Expecting the rest of the description but got {} ({})", tok.str, tok.location()), 0);
						return false;
					}

					ip.description.append(tok.quoteless());
				}
			}
			else
			{
				//It wasn't a colon
			}

			continue;
		}

		bc.properties.push_back({});
		ClassProperty& cp = bc.properties.back();
		cp.name = tok.str;
		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
		if (!expect_token("(", tok)) return false;
		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;

		//Types starting with * are reportable
		std::string_view type_sv{ tok.str };
		if (type_sv.starts_with("*")) {
			cp.mods.report = true;
			type_sv = type_sv.substr(1);
		}

		cp.type = ClassProperty::Value::type_from_string(type_sv);
		if (cp.type == ClassProperty::Value::Type::Invalid) {
			logger.error(std::format("Invalid type {} ({})", tok.str, tok.location()), 0);
			return false;
		}

		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
		if (!expect_token(")", tok)) return false;

		if (cp.type == ClassProperty::Value::Type::Flags)
		{
			cp.choice_like_value.emplace();
			if (!cp.choice_like_value->flags_from_tokenizer(tokenizer))
				return false;
			//We end up at the next value/end
			continue;
		}

		tok = tokenizer.peek_next_token();

		if (tok.str == "=" && cp.type == ClassProperty::Value::Type::Choices)
		{
			cp.choice_like_value.emplace();
			if (!cp.choice_like_value->choices_from_tokenizer(tokenizer))
				return false;
			//We end up at the next value/end
			continue;
		}

		//A colon is expected, if there is none, then it can be another value or a modifier
		if (!expect_token(":", tok, false)) 
		{
			if (tok.str == "readonly") {
				cp.mods.readonly = true;
			} else if (tok.str == "report") {
				cp.mods.report = true;
			} else {
				//This is a new value, go back so that we can start the loop again
				continue;
			}

			tok = tokenizer.get_next_token();
			tok = tokenizer.peek_next_token();
			if (!expect_token(":", tok, false)) {
				//It is also a new value
				continue;
			}

			tok = tokenizer.get_next_token();
		}
		else {
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
		}

		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;

		if (!tok.is_quoted())
		{
			logger.error(std::format("Expecting a quoted display name but got {} ({})", tok.str, tok.location()), 0);
			return false;
		}

		cp.display_name = tok.quoteless();

		tok = tokenizer.peek_next_token();
		
		if (tok.str == "=" && cp.type == ClassProperty::Value::Type::Choices)
		{
			cp.choice_like_value.emplace();
			if (!cp.choice_like_value->choices_from_tokenizer(tokenizer))
				return false;
			//We end up at the next value/end
			continue;
		}

		if (!expect_token(":", tok, false)) {
			//New value
			continue;
		}

		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;

		tok = tokenizer.peek_next_token();

		//The default value is here, however it can be empty and still have following data

		//If there is a colon as a next token, the default value is purposefuly left empty
		if (tok.str == ":") {
			//Go back so that we can advance again and do the colon check
		
		} else {
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;

			ClassProperty::Value pot_val = ClassProperty::Value::get_from_token(tok);
			if (pot_val.type() == ClassProperty::Value::Type::Invalid) { //If there is none of that, this is a new value
				//New value
				continue;
			} else { //If there is a valid value and it matches the keyvalue type, this is fine
				cp.default_value = pot_val;

				//The type might differ from the expected type but still be valid, for instance: float gets treated as integer
				const ClassProperty::TrivialValue::Type eq = get_trivial_type_equivalent(cp.type);
				if (eq == ClassProperty::TrivialValue::Type::Float &&
					cp.default_value->type() == ClassProperty::Value::Type::Integer)
				{
					const i32 v = std::any_cast<i32>(cp.default_value->value);
					cp.default_value->type() = ClassProperty::Value::Type::Float;
					cp.default_value->value = std::make_any<double>(static_cast<double>(v));
				}
				else if (eq == ClassProperty::TrivialValue::Type::String &&
						(cp.default_value->type() == ClassProperty::Value::Type::Float ||
						 cp.default_value->type() == ClassProperty::Value::Type::Integer))
				{
					cp.default_value->type() = ClassProperty::Value::Type::String;
					cp.default_value->value = std::make_any<std::string>(tok.quoteless());
				}
			}
		}

		tok = tokenizer.peek_next_token();
		if (!expect_token(":", tok, false)) {
			//New value / choices

			if (cp.type == ClassProperty::Value::Type::Choices)
			{
				cp.choice_like_value.emplace();
				if (!cp.choice_like_value->choices_from_tokenizer(tokenizer))
					return false;
				//We end up at the next value/end
			}

			continue;
		}

		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
		tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;

		if (!tok.is_quoted())
		{
			logger.error(std::format("Expecting a description but got {} ({})", tok.str, tok.location()), 0);
			return false;
		}

		cp.description = tok.quoteless();

		//Check for + string concatenation
		for (tok = tokenizer.peek_next_token(); expect_token("+", tok, false); tok = tokenizer.peek_next_token())
		{
			if (!tokenizer.valid()) return false;

			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return false;
			if (!tok.is_quoted())
			{
				logger.error(std::format("Expecting the rest of the description but got {} ({})", tok.str, tok.location()), 0);
				return false;
			}

			cp.description.append(tok.quoteless());
		}


		if (cp.type == ClassProperty::Value::Type::Choices)
		{
			cp.choice_like_value.emplace();
			if (!cp.choice_like_value->choices_from_tokenizer(tokenizer))
				return false;
			//We end up at the next value/end
		}


	}

	return true;
}

FGD FGD::Parser::parse()
{
	for (Token tok = tokenizer.get_next_token(); tokenizer.valid(); tok = tokenizer.get_next_token())
	{
		std::string ltok = tok.lowercase();

		if (ltok == "@mapsize")
		{
			fgd.mapsize.emplace();

			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;
			if (!expect_token("(", tok)) return fgd;
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;
			fgd.mapsize->first = std::stoi(tok.str);
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;
			if (!expect_token(",", tok)) return fgd;
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;
			fgd.mapsize->second = std::stoi(tok.str);
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;
			if (!expect_token(")", tok)) return fgd;
		}
		else if (ltok == "@include")
		{
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;
			std::string include_path = tok.quoteless();
			fgd.includes.push_back(include_path);

			if (app_params.recurse_includes) {
				if (!handle_file(include_path)) {
					logger.error("FGD handling failed", 0);
					return fgd;
				}
			}
		}
		else if (ltok == "@pointclass" || ltok == "@solidclass" || ltok == "@npcclass" ||
				 ltok == "@keyframeclass" || ltok == "@moveclass" || ltok == "@filterclass" ||
				 ltok == "@baseclass")
		{
			fgd.classes.push_back({});
			Class& bc = fgd.classes.back();
				 if (ltok == "@pointclass")		bc.type = Class::Type::PointClass;
			else if (ltok == "@solidclass")		bc.type = Class::Type::SolidClass;
			else if (ltok == "@npcclass")		bc.type = Class::Type::NPCClass;
			else if (ltok == "@keyframeclass")	bc.type = Class::Type::KeyFrameClass;
			else if (ltok == "@moveclass")		bc.type = Class::Type::MoveClass;
			else if (ltok == "@filterclass")	bc.type = Class::Type::FilterClass;
			else if (ltok == "@baseclass")		bc.type = Class::Type::BaseClass;
			else {}

			//Helpers, optional
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;
			if (!expect_token("=", tok, false)) //If there's no equal sign, there are helper(s)
			{
				for (; !expect_token("=", tok, false); tok = tokenizer.get_next_token())
				{
					if (!tokenizer.valid()) return fgd;
					
					bc.helpers.push_back({});
					Helper& helper = bc.helpers.back();
					helper.name = tok.str;

					if (tok.lowercase() == "halfgridsnap")
					{
						//halfgridsnap is the only helper without parentheses, thank you Valve
						helper.tokens.push_back(tok.str);
						continue;
					}
					
					tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;
					if (!expect_token("(", tok)) return fgd;
					
					//Copy the tokens inside of the parentheses
					for (tok = tokenizer.get_next_token(); !expect_token(")", tok, false); tok = tokenizer.get_next_token())
					{
						if (!tokenizer.valid()) return fgd;

						helper.tokens.push_back(tok.str);
					}
				}
			}

			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;
			bc.name = tok.str;

			//Description, optional
			tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;

			if (expect_token(":", tok, false))
			{
				tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;

				if (!tok.is_quoted())
				{
					logger.error(std::format("Expecting a description but got {} ({})", tok.str, tok.location()), 0);
					return fgd;
				}

				bc.description = tok.quoteless();

				//Check for + string concatenation
				for (tok = tokenizer.get_next_token(); expect_token("+", tok, false); tok = tokenizer.get_next_token())
				{
					if (!tokenizer.valid()) return fgd;

					tok = tokenizer.get_next_token(); if (!tokenizer.valid()) return fgd;
					if (!tok.is_quoted())
					{
						logger.error(std::format("Expecting the rest of the description but got {} ({})", tok.str, tok.location()));
						return fgd;
					}

					bc.description.append(tok.quoteless());
				}
			}

			if (!expect_token("[", tok)) return fgd;

			if (!parse_keyvalues(bc))
				return fgd;
		}
		//@MaterialExclusion, @AutoVisGroup, @gridnav, @version
		else {
			logger.warn(std::format("Ran into unsupported \"{}\" ({})", tok.str, tok.location()));
		}

	}

	fgd.valid = true;
	return fgd;
}

void FGD::Writer::write_str(const std::string_view& sv, u32 tabs)
{
	std::string str{ sv };

	for (u32 i = 0; i < tabs; i++) {
		str.insert(str.begin(), '\t');
	}

	out.write(str.c_str(), str.size());
}

void FGD::Writer::write_nl()
{
	write_str("\n");
}

void FGD::Writer::write_choices(const ClassProperty::Value::Choices& choices)
{
	u32 depth = 1;

	write_str("[\n", depth);
	depth++;

	for (auto& ch : choices) {
		write_str(std::format("{}: \"{}\"\n", ch.value.value_as_string(), ch.display_name), depth);
	}

	depth--;
	write_str("]\n", depth);
}

void FGD::Writer::write_flags(const ClassProperty::Value::Flags& flags)
{
	u32 depth = 1;

	write_str("[\n", depth);
	depth++;

	for (auto& fl : flags) {
		write_str(std::format("{} : \"{}\" : {}\n", std::to_string(fl.value), fl.name, fl.default_ticked ? "1" : "0"), depth);
	}

	depth--;
	write_str("]\n", depth);
}


void FGD::Writer::write_clprop(const ClassProperty& cp)
{
	const u32 depth = 1;

	//Name
	write_str(std::format("{}(", cp.name), depth);
	//Type
	if(format_type == FormatType::Hammer)
		write_str(std::format("{}) ", ClassProperty::Value::string_from_type(cp.type)));
	else
	{
		//For Trenchbroom write choices for the boolean type, as it doesn't support it
		const ClassProperty::Value::Type tb_eq = get_tb_type_equivalent(cp.type);
		std::string type_str = ClassProperty::Value::string_from_type(tb_eq);
		if (type_str == "boolean") type_str = "choices";
		write_str(std::format("{}) ", type_str));
	}

	//Contrarily to what the VDC says, readonly does work in Trenchbroom, not report though
		 if (cp.mods.readonly)										write_str("readonly ");
	else if (cp.mods.report && format_type == FormatType::Hammer)	write_str("report ");
	else {}

	if (cp.type == ClassProperty::Value::Type::Flags)
	{
		write_str("=\n");

		ClassProperty::Value::Flags v = std::any_cast<ClassProperty::Value::Flags>(cp.choice_like_value->value);
		write_flags(v);

		write_nl();
		return;
	}

	//Display name
	if (!cp.display_name.empty()) {
		write_str(std::format(": \"{}\" ", cp.display_name));
	}

	//Default value
	if (cp.default_value.has_value()) {
		write_str(std::format(": {}", cp.default_value->value_as_string()));
	}
	
	if (!cp.description.empty() && !cp.default_value.has_value()) {
		write_str(":   ");
	}

	//Description
	if (!cp.description.empty()) {
		write_str(std::format(" : \"{}\"", cp.description));
	}

	//Trenchbroom doesn't support the boolean type, write our own choices
	if (format_type == FormatType::Trenchbroom && cp.type == ClassProperty::Value::Type::Boolean)
	{
		ClassProperty::Value::Choices choices =
		{
			{ ClassProperty::TrivialValue::get_from_token({"1", 0, 0}), "Yes"},
			{ ClassProperty::TrivialValue::get_from_token({"0", 0, 0}), "No" }
		};

		write_str(" =\n");

		write_choices(choices);
	}

	//Choices
	if (cp.type == ClassProperty::Value::Type::Choices && cp.choice_like_value.has_value())
	{
		write_str(" =\n");

		ClassProperty::Value::Choices v = std::any_cast<ClassProperty::Value::Choices>(cp.choice_like_value->value);
		write_choices(v);
	}

	write_nl();
}

void FGD::Writer::write_ioprop(const IOProperty& prop)
{
	const u32 depth = 1;

	if (prop.type == IOProperty::Type::Input) write_str("input ", depth);
										 else write_str("output ", depth);

	if(format_type == FormatType::Hammer)
		write_str(std::format("{}({})", prop.name, IOProperty::string_from_value_type(prop.value_type)));
	else //Trenchbroom doesn't support target_destination, color255 and vector due to VDC misinfo
	{
		std::string val_type_str = IOProperty::string_from_value_type(prop.value_type);
			 if (val_type_str == "target_destination") val_type_str = "string";
		else if (val_type_str == "color255") val_type_str = "string";
		else if (val_type_str == "vector") val_type_str = "string";
		else {}
		write_str(std::format("{}({})", prop.name, val_type_str));
	}

	if (!prop.description.empty())
		write_str(std::format(" : \"{}\"", prop.description));

	write_nl();
}

void FGD::Writer::write_class(const Class& cl)
{
	static constexpr std::string_view tb_helpers[] =
	{
		"base",
		"color",
		"size",
		"decal",
		"sprite"
	};

		 if (cl.type == Class::Type::BaseClass)			write_str("@BaseClass ");
	else if (cl.type == Class::Type::PointClass)		write_str("@PointClass ");
	else if (cl.type == Class::Type::SolidClass)		write_str("@SolidClass ");
	else if (cl.type == Class::Type::NPCClass)			write_str("@PointClass ");
	else if (cl.type == Class::Type::KeyFrameClass)		write_str("@PointClass ");
	else if (cl.type == Class::Type::MoveClass)			write_str("@PointClass ");
	else if (cl.type == Class::Type::FilterClass)		write_str("@PointClass ");

	//Helpers
	for (auto& hl : cl.helpers)
	{
		//Special conversion to model
		if (format_type == FormatType::Trenchbroom)
		{
			if (hl.name == "iconsprite" || hl.name == "studio" || hl.name == "studioprop" || hl.name == "lightprop")
			{
				//Empty studio() helpers expect the model to be the model kv
				if (hl.tokens.empty()) {
					if (hl.name == "studio" || hl.name == "studioprop" || hl.name == "lightprop") {
						write_str("model({\"path\": model}) ");
						continue;
					}
					logger.warn("Empty iconsprite helper");
					continue;
				}

				write_str("model(");

				std::string path = hl.tokens[0].substr(1, hl.tokens[0].length() - 2);
				if (hl.name == "iconsprite")
				{
					path = "materials/" + path;
					if (path.ends_with(".vmt"))
						path = path.substr(0, path.length() - 4);

					path.append(".");
					path.append(app_params.spr_extension);
				}
				else
				{
					if (path.ends_with(".mdl"))
						path = path.substr(0, path.length() - 4);

					path.append(".");
					path.append(app_params.mdl_extension);
				}

				write_str("{ ");
				write_str(std::format("\"path\": \"{}\"", path));

				if (hl.name == "iconsprite") {
					write_str(", \"scale\": 0.25");
				}

				write_str("}) ");
				continue;
			}
		}

		for (auto& tb_hl : tb_helpers) {
			if (tb_hl != hl.name)
				continue;
		}

		write_str(std::format("{}(", hl.name));

		for (auto it = hl.tokens.begin(); it != hl.tokens.end(); it = std::next(it)) {
			write_str(std::format("{}", *it));
			if (std::next(it) != hl.tokens.end() && *std::next(it) != ",")
				write_str(" ");
		}

		write_str(") ");
	}

	write_str("= ");

	//Name
	write_str(std::format("{} ", cl.name));

	//Description
	if (!cl.description.empty()) {
		write_str(std::format(": \"{}\"", cl.description));
	}

	write_nl();

	write_str("[\n");

	if (cl.properties.empty())
		write_nl();

	//Properties
	for (auto& cp : cl.properties)
		write_clprop(cp);
	
	if(!cl.io_properties.empty())
		write_nl();

	//IO properties
	for (auto& iop : cl.io_properties)
		write_ioprop(iop);

	write_str("]\n");
}

bool FGD::Writer::write(const std::filesystem::path filename)
{
	out.open(filename);
	if (out.fail()) {
		logger.error(std::format("Couldn't create {}", filename.string()), 0);
		return false;
	}

	if (!fgd.valid)
	{
		logger.error("Attempting to write an invalid fgd", 0);
		return false;
	}

	//General comment
	write_str("//FGD written by fgd-utils\n");
	write_nl();

	u32 depth = 0;

	//@mapsize
	//Contrarily to what the VDC says, it's not supported by Trenchbroom !
	if (fgd.mapsize.has_value() && format_type != FormatType::Trenchbroom) {
		write_str(std::format("@mapsize({}, {})\n", fgd.mapsize->first, fgd.mapsize->second), depth);
		write_nl();
	}

	//@include
	for (auto& inc : fgd.includes) {
		write_str(std::format("@include \"{}\"\n", inc), depth);
	}
	write_nl();

	//Classes
	for (auto& cl : fgd.classes) {
		write_class(cl);
		write_nl();
	}

	out.close();

	return true;
}
