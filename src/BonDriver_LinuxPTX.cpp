// BonDriver_LinuxPTX.cpp

#include "BonDriver_LinuxPTX.hpp"

#include <cstddef>
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <chrono>

#include <iconv.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <dlfcn.h>

#include "util.hpp"

namespace BonDriver_LinuxPTX {

BonDriver::BonDriver(Config& config)
	: multi_(false),
	fd_(-1),
	lnb_power_state_(false),
	current_system_(::PTX_UNSPECIFIED_SYSTEM),
	current_space_(0),
	current_channel_(0),
	iorp_(*this)
{
	CharCodeConv cv;

	auto sct = config.Get("BonDriver_LinuxPTX");

	device_ = sct.Get("Device");

	if (!cv.Utf8ToUtf16(sct.Get("Name", "LinuxPTX"), name_))
		throw std::runtime_error("BonDriver::BonDriver: CharCodeConv::Utf8ToUtf16() failed");

	lnb_power_ = sct.Get("LNBPower", 0) ? true : false;

	std::vector<std::string> spaces;
	bool isdbt = false, isdbs = false;

	util::Separate(config.Get("Space").Get("Space"), spaces);
	for (auto v : spaces) {
		v = "Space." + v;
		auto subspace_sct = config.Get(v);
		auto subspace_ch_sct = config.Get(v + ".Channel");

		auto sys_str = subspace_sct.Get("System");
		::ptx_system_type sys;

		if (!sys_str.compare("ISDB-T")) {
			sys = ::PTX_ISDB_T_SYSTEM;
			isdbt = true;
		} else if (!sys_str.compare("ISDB-S")) {
			sys = ::PTX_ISDB_S_SYSTEM;
			isdbs = true;
		} else {
			throw std::runtime_error("BonDriver::BonDriver: unknown system");
		}

		auto& space = space_.emplace_back(cv, subspace_sct.Get("Name"), sys);
		space.AddChannel(cv, subspace_ch_sct);
	}

	if (isdbt && isdbs)
		multi_ = true;

	ioq_.reset(new IoQueue(IoQueue::IoOperation::READ, iorp_, 188 * 2048));
}

BonDriver::~BonDriver()
{
	CloseTuner();
}

const BOOL BonDriver::OpenTuner(void)
{
	std::lock_guard<std::mutex> lock(mtx_);

	if (fd_ >= 0)
		return TRUE;

	fd_ = ::open(device_.c_str(), O_RDONLY);
	if (fd_ == -1)
		return FALSE;

	return TRUE;
}

void BonDriver::CloseTuner(void)
{
	std::lock_guard<std::mutex> lock(mtx_);

	if (fd_ == -1)
		return;

	ioq_->Stop();

	current_system_.store(::PTX_UNSPECIFIED_SYSTEM, std::memory_order_release);
	current_space_.store(0, std::memory_order_release);
	current_channel_.store(0, std::memory_order_release);

	if (lnb_power_state_) {
		::ioctl(fd_, PTX_DISABLE_LNB_POWER);
		lnb_power_state_ = false;
	}

	::close(fd_);
	fd_ = -1;

	return;
}

const BOOL BonDriver::SetChannel(const BYTE bCh)
{
	return SetChannel(0, bCh);
}

const float BonDriver::GetSignalLevel(void)
{
	int val;

	if (ioctl(fd_, PTX_GET_CNR, &val) == -1)
		return 0.0f;

	float cnr = 0.0f;

	switch (current_system_.load(std::memory_order_acquire)) {
	case ::PTX_ISDB_T_SYSTEM:
	{
		double p;

		if (!val)
			break;

		p = std::log10(5505024 / (double)val);
		cnr = (240 * p * p * p * p) - (1600 * p * p * p) + (3980 * p * p) + (5491 * p) + 3096.5;
		cnr /= 1000.0f;

		break;
	}

	case ::PTX_ISDB_S_SYSTEM:
	{
		double p;

		if (val < 3000)
			break;

		p = std::sqrt(val - 3000) / 64;
		cnr = (-1634.6 * p * p * p * p * p) + (14341 * p * p * p * p) - (50259 * p * p * p) + (88977 * p * p) - (89565 * p) + 58857;
		cnr /= 1000.0f;

		break;
	}

	default:
		break;
	}

	return cnr;
}

const DWORD BonDriver::WaitTsStream(const DWORD dwTimeOut)
{
	return (ioq_->WaitDataBuffer(std::chrono::milliseconds((dwTimeOut == INFINITE) ? 0 : dwTimeOut))) ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
}

const DWORD BonDriver::GetReadyCount(void)
{
	return ioq_->GetDataBufferCount() + (ioq_->HaveReadingBuffer() ? 1 : 0);
}

const BOOL BonDriver::GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	if (!pDst || !pdwSize)
		return FALSE;

	std::size_t size, remain;

	if (!ioq_->Read(pDst, size, remain, false))
		return FALSE;

	*pdwSize = static_cast<::DWORD>(size);
	if (pdwRemain)
		*pdwRemain = static_cast<::DWORD>(remain);

	return TRUE;
}

const BOOL BonDriver::GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain)
{
	if (!ppDst || !pdwSize)
		return FALSE;

	std::size_t size, remain;

	if (!ioq_->ReadBuffer(reinterpret_cast<void **>(ppDst), size, remain, false))
		return FALSE;

	*pdwSize = static_cast<::DWORD>(size);
	if (pdwRemain)
		*pdwRemain = static_cast<::DWORD>(remain);

	return TRUE;
}

void BonDriver::PurgeTsStream(void)
{
	ioq_->PurgeDataBuffer();
	return;
}

void BonDriver::Release(void)
{
	DestroyInstance();
	return;
}

LPCTSTR BonDriver::GetTunerName(void)
{
	return name_.get();
}

const BOOL BonDriver::IsTunerOpening(void)
{
	std::lock_guard<std::mutex> lock(mtx_);

	return (fd_ != -1) ? TRUE : FALSE;
}

LPCTSTR BonDriver::EnumTuningSpace(const DWORD dwSpace)
{
	try {
		return space_.at(dwSpace).GetName();
	} catch (const std::out_of_range&) {
		return NULL;
	}
}

LPCTSTR BonDriver::EnumChannelName(const DWORD dwSpace, const DWORD dwChannel)
{
	try {
		return space_.at(dwSpace).GetChannel(dwChannel).GetName();
	} catch (const std::out_of_range&) {
		return NULL;
	}
}

const BOOL BonDriver::SetChannel(const DWORD dwSpace, const DWORD dwChannel)
{
	std::lock_guard<std::mutex> lock(mtx_);
	::ptx_system_type system;
	::ptx_freq freq;

	if (fd_ == -1)
		return FALSE;

	try {
		auto& s = space_.at(dwSpace);

		system = s.GetSystem();
		s.GetChannel(dwChannel).ToFreq(freq);
	} catch (const std::out_of_range&) {
		return FALSE;
	}

	if (multi_) {
		if (::ioctl(fd_, PTX_SET_SYSTEM_MODE, system) == -1)
			return FALSE;
	}

	if (system == ::PTX_ISDB_S_SYSTEM) {
		if (lnb_power_ && !lnb_power_state_) {
			if (::ioctl(fd_, PTX_ENABLE_LNB_POWER, 2) == -1)
				return FALSE;

			lnb_power_state_ = true;
		}
	} else if (lnb_power_state_) {
		::ioctl(fd_, PTX_DISABLE_LNB_POWER);
		lnb_power_state_ = false;
	}

	if (::ioctl(fd_, PTX_SET_CHANNEL, &freq) == -1)
		return FALSE;

	current_system_.store(system, std::memory_order_release);
	current_space_.store(dwSpace, std::memory_order_release);
	current_channel_.store(dwChannel, std::memory_order_release);

	ioq_->Start();
	return TRUE;
}

const DWORD BonDriver::GetCurSpace(void)
{
	return current_space_.load(std::memory_order_acquire);
}

const DWORD BonDriver::GetCurChannel(void)
{
	return current_channel_.load(std::memory_order_acquire);
}

BonDriver::Space::Channel::Channel(CharCodeConv& cv, const std::string& name, int number, int slot)
	: number_(number),
	slot_(slot)
{
	if (!cv.Utf8ToUtf16(name, name_))
		throw std::runtime_error("BonDriver::Space::Channel::Channel: CharCodeConv::Utf8ToUtf16() failed");
}

const ::WCHAR * BonDriver::Space::Channel::GetName() const
{
	return name_.get();
}

void BonDriver::Space::Channel::ToFreq(::ptx_freq& freq) const
{
	freq.freq_no = number_;
	freq.slot = slot_;
	return;
}

BonDriver::Space::Space(CharCodeConv& cv, const std::string& name, ::ptx_system_type system)
	: system_(system)
{
	if (!cv.Utf8ToUtf16(name, name_))
		throw std::runtime_error("BonDriver::Space::Space: CharCodeConv::Utf8ToUtf16() failed");
}

const ::WCHAR * BonDriver::Space::GetName() const
{
	return name_.get();
}

::ptx_system_type BonDriver::Space::GetSystem() const
{
	return system_;
}

void BonDriver::Space::AddChannel(CharCodeConv& cv, Config::Section& sct)
{
	for (std::uint16_t i = 0; i < 300; i++) {
		char k[8];
		std::snprintf(k, 8, "Ch%u", i);

		auto key = std::string(k);
		try {
			std::vector<std::string> data;

			util::Separate(sct.Get(key), data);
			if (data.size() != 3)
				throw std::runtime_error("BonDriver::Space::AddChannel: invalid channel");

			channel_.emplace_back(cv, data[0], std::stoi(data[1], nullptr, 0), std::stoi(data[2], nullptr, 0));
		} catch (const std::out_of_range&) {
			break;
		}
	}

	return;
}

const BonDriver::Space::Channel& BonDriver::Space::GetChannel(std::size_t pos) const
{
	return channel_.at(pos);
}

bool BonDriver::ReadProvider::Start()
{
	return (::ioctl(parent_.fd_, PTX_START_STREAMING) >= 0) ? true : false;
}

void BonDriver::ReadProvider::Stop()
{
	::ioctl(parent_.fd_, PTX_STOP_STREAMING);
	return;
}

bool BonDriver::ReadProvider::Do(void *buf, std::size_t& size)
{
	::ssize_t count;

	count = ::read(parent_.fd_, buf, size);
	if (count < 0)
		return false;

	size = count;
	return true;
}

std::mutex BonDriver::instance_mtx_;
BonDriver *BonDriver::instance_ = nullptr;

BonDriver * BonDriver::GetInstance()
{
	std::lock_guard<std::mutex> lock(instance_mtx_);

	if (!instance_) {
		try {
			Dl_info dli;
			if (!::dladdr(reinterpret_cast<void *>(GetInstance), &dli))
				return nullptr;

			std::size_t path_len = std::strlen(dli.dli_fname);
			if (path_len < 3 || std::strcmp(&dli.dli_fname[path_len - 3], ".so"))
				return nullptr;

			auto path = std::make_unique<char[]>(path_len + 5);
			std::strncpy(path.get(), dli.dli_fname, path_len - 2);
			std::strncpy(path.get() + path_len - 2, "ini", 4);

			Config config;
			if (!config.Load(path.get())) {
				std::strncpy(path.get() + path_len - 2, "so.ini", 7);
				if (!config.Load(path.get()))
					return nullptr;
			}

			instance_ = new BonDriver(config);
		} catch (...) {
			return nullptr;
		}
	}

	return instance_;
}

void BonDriver::DestroyInstance()
{
	std::lock_guard<std::mutex> lock(instance_mtx_);

	if (instance_) {
		delete instance_;
		instance_ = nullptr;
	}

	return;
}

extern "C" IBonDriver * CreateBonDriver()
{
	return BonDriver::GetInstance();
}

} // namespace BonDriver_LinuxPTX
