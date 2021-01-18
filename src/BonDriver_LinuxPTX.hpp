// BonDriver_LinuxPTX.hpp

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <vector>
#include <utility>

#include "type_compat.h"
#include "IBonDriver2.h"
#include "ptx_ioctl.h"
#include "config.hpp"
#include "char_code_conv.hpp"
#include "io_queue.hpp"

namespace BonDriver_LinuxPTX {

class BonDriver final : public IBonDriver2 {
public:
	explicit BonDriver(Config& config);
	~BonDriver();

	// cannot copy
	BonDriver(const BonDriver&) = delete;
	BonDriver& operator=(const BonDriver&) = delete;

	// IBonDriver
	const BOOL OpenTuner(void) override;
	void CloseTuner(void) override;

	const BOOL SetChannel(const BYTE bCh) override;
	const float GetSignalLevel(void) override;

	const DWORD WaitTsStream(const DWORD dwTimeOut = 0) override;
	const DWORD GetReadyCount(void) override;

	const BOOL GetTsStream(BYTE *pDst, DWORD *pdwSize, DWORD *pdwRemain) override;
	const BOOL GetTsStream(BYTE **ppDst, DWORD *pdwSize, DWORD *pdwRemain) override;

	void PurgeTsStream(void) override;

	void Release(void) override;

	// IBonDriver2
	LPCTSTR GetTunerName(void) override;

	const BOOL IsTunerOpening(void) override;

	LPCTSTR EnumTuningSpace(const DWORD dwSpace) override;
	LPCTSTR EnumChannelName(const DWORD dwSpace, const DWORD dwChannel) override;

	const BOOL SetChannel(const DWORD dwSpace, const DWORD dwChannel) override;

	const DWORD GetCurSpace(void) override;
	const DWORD GetCurChannel(void) override;

	static BonDriver * GetInstance();

private:
	class Space final {
	public:
		class Channel final {
		public:
			Channel(CharCodeConv& cv, const std::string& name, int number, int slot);
			~Channel() {}

			// cannot copy
			Channel(const Channel&) = delete;
			Channel& operator=(const Channel&) = delete;

			// can move
			Channel(Channel&&) = default;
			Channel& operator=(Channel&&) = default;

			const ::WCHAR * GetName() const;
			void ToFreq(::ptx_freq& freq) const;

		private:
			std::unique_ptr<::WCHAR[]> name_;
			int number_;
			int slot_;
		};

		Space(CharCodeConv& cv, const std::string& name, ::ptx_system_type system);
		~Space() {}

		// cannot copy
		Space(const Space&) = delete;
		Space& operator=(Space&) = delete;

		// can move
		Space(Space&&) = default;
		Space& operator=(Space&&) = default;

		const ::WCHAR * GetName() const;
		::ptx_system_type GetSystem() const;
		void AddChannel(CharCodeConv& cv, Config::Section& sct);
		const Channel& GetChannel(std::size_t pos) const;

	private:
		std::unique_ptr<::WCHAR[]> name_;
		::ptx_system_type system_;
		std::vector<Channel> channel_;
	};

	class ReadProvider final : public IoQueue::IoProvider {
	public:
		explicit ReadProvider(BonDriver& parent) noexcept : parent_(parent) {}
		~ReadProvider() {}

		bool Start() override;
		void Stop() override;
		bool Do(void *buf, std::size_t& size) override;

	private:
		BonDriver& parent_;
	};

	std::mutex mtx_;
	std::unique_ptr<::WCHAR[]> name_;
	std::string device_;
	bool lnb_power_;
	bool multi_;
	std::vector<Space> space_;
	std::atomic<int> fd_;
	bool lnb_power_state_;
	std::atomic<::ptx_system_type> current_system_;
	std::atomic<::DWORD> current_space_;
	std::atomic<::DWORD> current_channel_;
	std::unique_ptr<IoQueue> ioq_;
	ReadProvider iorp_;

	static void DestroyInstance();

	static std::mutex instance_mtx_;
	static BonDriver *instance_;
};

} // namespace BonDriver_LinuxPTX
