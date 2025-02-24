#pragma once

#include <string>
#include <glm/detail/type_vec.hpp>
#include <glm/glm.hpp>
#include "math/Vec.h"


struct Atom {
	const std::string name;
	const std::string residueName;
	const char chain;
	const int residueNum;
	const glm::vec3 coords;
	const std::string element;
};
