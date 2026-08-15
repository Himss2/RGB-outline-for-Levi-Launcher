#pragma once

#include <cstdint>

namespace OutlineHook {

using Address = uintptr_t;

bool initialize();

bool install(Address target);

void uninstall();

bool installed();

Address target();

}
