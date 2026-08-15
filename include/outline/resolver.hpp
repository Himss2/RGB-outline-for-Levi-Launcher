#pragma once

#include <cstdint>
#include <string_view>

namespace outline::resolver {

bool initialize(std::string_view library);

std::uintptr_t selectionGeometry();
std::uintptr_t renderLevel();

std::uintptr_t tessellatorBegin();
std::uintptr_t tessellatorColor();
std::uintptr_t tessellatorVertex();

bool ready();

}
