// charconv.cpp

#include "charconv.hpp"

namespace BonDriver_LinuxPTX {

CharConv::CharConv()
{
	cd_ = ::iconv_open("UTF-16LE", "UTF-8");
	if (cd_ == reinterpret_cast<::iconv_t>(-1))
		throw std::runtime_error("BonDriver::BonDriver: ::iconv_open() failed");
}

CharConv::~CharConv()
{
	if (cd_ != reinterpret_cast<::iconv_t>(-1))
		::iconv_close(cd_);
}

bool CharConv::Utf8ToUtf16(const std::string& src, std::unique_ptr<::WCHAR[]>& dst)
{
	char *s = const_cast<char *>(src.c_str());
	std::size_t s_len = src.length();

	dst.reset(new ::WCHAR[s_len + 1]);

	char *d = reinterpret_cast<char *>(dst.get());
	std::size_t d_len = s_len * sizeof(::WCHAR);

	std::size_t cr = ::iconv(cd_, &s, &s_len, &d, &d_len);
	if (cr == static_cast<std::size_t>(-1))
		return false;

	*reinterpret_cast<::WCHAR *>(d) = '\0';
	return true;
}

} // BonDriver_LinuxPTX
