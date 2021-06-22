#pragma once
#include <string>
#include "common.hpp"

enum class ResourceType
{
	NONE,
	MATERIAL,
	MODEL,
	TEXTURE2D,
	NODE,
};

struct ResourceBase
{
	std::string _name;
	inline static ResourceType _type = ResourceType::NONE;
	virtual ~ResourceBase() {}
};
