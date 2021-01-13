// util.hpp

#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

#include "ptx_ioctl.h"

namespace BonDriver_LinuxPTX {

namespace util {

void Trim(char **str, std::size_t *len);
void RTrim(char **str, std::size_t *len);
void Separate(const std::string& str, std::vector<std::string>& res);

} // namespace util

} // namespace BonDriver_LinuxPTX
