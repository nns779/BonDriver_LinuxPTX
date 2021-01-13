// charconv.hpp

#pragma once

#include <memory>
#include <string>

#include <iconv.h>

#include "type_compat.h"

namespace BonDriver_LinuxPTX {

class CharConv final {
public:
	CharConv();
	~CharConv();

	// cannot copy
	CharConv(const CharConv&) = delete;
	CharConv& operator=(const CharConv&) = delete;

	bool Utf8ToUtf16(const std::string& src, std::unique_ptr<::WCHAR[]>& dst);

private:
	::iconv_t cd_;
};

} // BonDriver_LinuxPTX
