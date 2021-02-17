// char_code_conv.hpp

#pragma once

#include <memory>
#include <string>

#include <iconv.h>

#include "type_compat.h"

namespace BonDriver_LinuxPTX {

class CharCodeConv final {
public:
	CharCodeConv();
	~CharCodeConv();

	// cannot copy
	CharCodeConv(const CharCodeConv&) = delete;
	CharCodeConv& operator=(const CharCodeConv&) = delete;

	bool Utf8ToUtf16(const std::string& src, std::unique_ptr<::WCHAR[]>& dst);

private:
	::iconv_t cd_;
};

} // namespace BonDriver_LinuxPTX
