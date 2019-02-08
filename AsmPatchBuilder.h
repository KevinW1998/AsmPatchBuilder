#pragma once

#include <cstdint>
#include <exception>
#include <type_traits>
#include <tuple>
#include "details/LambdaPayloadInjector.h"
#include <vector>
#include <utility>

namespace AsmPatch {

class AsmPatchData
{
	std::uintptr_t mAddr;
	std::vector<std::uint8_t> mData;
public:
	AsmPatchData(std::uintptr_t addr, std::vector<std::uint8_t> data) :
		mAddr(addr),
		mData(std::move(data))
	{}

	std::uintptr_t getAddress() const
	{
		return mAddr;
	}

	const std::vector<std::uint8_t>& getData() const
	{
		return mData;
	}

	std::pair<std::uintptr_t, std::vector<std::uint8_t>> const& getDataPack() const&
	{
		return std::make_pair(mAddr, mData);
	}

	std::pair<std::uintptr_t, std::vector<std::uint8_t>>&& getDataPack() &&
	{
		return std::make_pair(mAddr, std::move(mData));
	}
};

namespace AsmConsts {
	enum R32 {
		R32_EAX = 0x0,
		R32_ECX = 0x1,
		R32_EDX = 0x2,
		R32_EBX = 0x3,
		R32_ESP = 0x4,
		R32_EBP = 0x5,
		R32_ESI = 0x6,
		R32_EDI = 0x7
	};
};

struct AsmPatchNoCondtionalJump : std::exception {
	const char* what() const override { return "No conditional jump at detected"; }
};

template <std::uintptr_t Size>
struct AsmPatchBuilder final {
	/********************
	* Member variables *
	********************/
public:
	const std::uintptr_t mAddr;
	std::uint8_t mPatchBytes[Size ? Size : 1];
	
	/***********************************
	* Constructor and utility methods *
	***********************************/
public:
	AsmPatchBuilder(std::uintptr_t addr) :
		mAddr(addr)
	{}

	std::uintptr_t size() const {
		return Size;
	}

	std::uintptr_t cursor() const {
		return mAddr + Size;
	}

	AsmPatchData compile()
	{
		return AsmPatchData(mAddr, { std::begin(mPatchBytes), std::end(mPatchBytes) });
	}

	/*******************************
	* Appending data to the patch *
	*******************************/
	inline AsmPatchBuilder<Size> bytes() const {
		return *this;
	}

	template <typename... Ts>
	inline AsmPatchBuilder<Size + 1 + sizeof...(Ts)> bytes(std::uint8_t newByte, Ts... params) const {
		AsmPatchBuilder<Size + 1> ret(mAddr);
		for (std::uintptr_t i = 0; i < Size; i++) {
			ret.mPatchBytes[i] = mPatchBytes[i];
		}
		ret.mPatchBytes[Size] = newByte;
		return ret.bytes(params...);
	}

	inline AsmPatchBuilder<Size + 1> byte(std::uint8_t newByte) const {
		return bytes(newByte);
	}

	inline AsmPatchBuilder<Size + 2> word(std::uint32_t newWord) const {
		const std::uint8_t* data = (const std::uint8_t*)&newWord;
		return bytes(data[0], data[1]);
	}

	inline AsmPatchBuilder<Size + 4> dword(std::uint32_t newDWord) const {
		const std::uint8_t* data = (const std::uint8_t*)&newDWord;
		return bytes(data[0], data[1], data[2], data[3]);
	}

	/*****************************
	* Insertion of instructions *
	*****************************/
public:
	inline AsmPatchBuilder<Size + 1> nop() const {
		return byte(0x90);
	}
	inline AsmPatchBuilder<Size + 1> ret() const {
		return byte(0xC3);
	}
	inline AsmPatchBuilder<Size + 3> retNear() const {
		return bytes(0xC2, 0x04, 0x00);
	}
	inline AsmPatchBuilder<Size + 1> pushR32(AsmConsts::R32 arg) const {
		return byte(0x50 | arg);
	}
	inline AsmPatchBuilder<Size + 1> popR32(AsmConsts::R32 arg) const {
		return byte(0x58 | arg);
	}
	inline AsmPatchBuilder<Size + 1> pushf() const {
		return byte(0x9C);
	}
	inline AsmPatchBuilder<Size + 1> popf() const {
		return byte(0x9D);
	}

	// Convenience shorthand
	inline AsmPatchBuilder<Size + 1> pushEAX() const { return pushR32(AsmConsts::R32_EAX); }
	inline AsmPatchBuilder<Size + 1> pushECX() const { return pushR32(AsmConsts::R32_ECX); }
	inline AsmPatchBuilder<Size + 1> pushEDX() const { return pushR32(AsmConsts::R32_EDX); }
	inline AsmPatchBuilder<Size + 1> pushEBX() const { return pushR32(AsmConsts::R32_EBX); }
	inline AsmPatchBuilder<Size + 1> pushESP() const { return pushR32(AsmConsts::R32_ESP); }
	inline AsmPatchBuilder<Size + 1> pushEBP() const { return pushR32(AsmConsts::R32_EBP); }
	inline AsmPatchBuilder<Size + 1> pushESI() const { return pushR32(AsmConsts::R32_ESI); }
	inline AsmPatchBuilder<Size + 1> pushEDI() const { return pushR32(AsmConsts::R32_EDI); }
	inline AsmPatchBuilder<Size + 1> popEAX() const { return popR32(AsmConsts::R32_EAX); }
	inline AsmPatchBuilder<Size + 1> popECX() const { return popR32(AsmConsts::R32_ECX); }
	inline AsmPatchBuilder<Size + 1> popEDX() const { return popR32(AsmConsts::R32_EDX); }
	inline AsmPatchBuilder<Size + 1> popEBX() const { return popR32(AsmConsts::R32_EBX); }
	inline AsmPatchBuilder<Size + 1> popESP() const { return popR32(AsmConsts::R32_ESP); }
	inline AsmPatchBuilder<Size + 1> popEBP() const { return popR32(AsmConsts::R32_EBP); }
	inline AsmPatchBuilder<Size + 1> popESI() const { return popR32(AsmConsts::R32_ESI); }
	inline AsmPatchBuilder<Size + 1> popEDI() const { return popR32(AsmConsts::R32_EDI); }

	inline AsmPatchBuilder<Size + 2> movRestoreStackptr() {
		return bytes(0x8B, 0xE5);
	}

	inline AsmPatchBuilder<Size + 9> retStdcallFull() {
		return (
			popEDI().
			popESI().
			popEBX().
			movRestoreStackptr().
			popEBP().
			retNear()
			);
	}

	template<typename LambdaFunc>
	inline AsmPatchBuilder<Size + 5> callLambdaStdcall(LambdaFunc func) const {
		return CALL(AsmBuilder::LambdaPayloadInjector::CreateFuncPtrFromLambda(func));
	}

	template<typename LambdaFunc>
	inline AsmPatchBuilder<Size + 5> callLambdaCdecl(LambdaFunc func) const {
		return CALL(AsmBuilder::LambdaPayloadInjector::CreateFuncPtrFromLambda<AsmBuilder::LambdaPayloadInjector::CallingConvention::CDeclCall>(func));
	}

	template<typename LambdaFunc>
	inline AsmPatchBuilder<Size + 5> callLambdaThiscall(LambdaFunc func) const {
		return CALL(AsmBuilder::LambdaPayloadInjector::CreateFuncPtrFromLambda<AsmBuilder::LambdaPayloadInjector::CallingConvention::ThisCall>(func));
	}

	template<typename LambdaFunc>
	inline AsmPatchBuilder<Size + 5> callLambdaFastcall(LambdaFunc func) const {
		return CALL(AsmBuilder::LambdaPayloadInjector::CreateFuncPtrFromLambda<AsmBuilder::LambdaPayloadInjector::CallingConvention::FastCall>(func));
	}

	template<AsmBuilder::LambdaPayloadInjector::CallingConvention CallConv, typename LambdaFunc>
	inline AsmPatchBuilder<Size + 5> callLambdaByCallConv(LambdaFunc func) const {
		return CALL(AsmBuilder::LambdaPayloadInjector::CreateFuncPtrFromLambda<CallConv>(func));
	}

	// TODO: Compile-time error if working with nullptr_t
	inline AsmPatchBuilder<Size + 5> call(void* func) const { return CALL((std::uintptr_t)func); }
	inline AsmPatchBuilder<Size + 5> call(std::uintptr_t func) const {
		return byte(0xE8).dword(func - cursor() - 5);
	}

	// TODO: Compile-time error if working with nullptr_t
	inline AsmPatchBuilder<Size + 5> jmp(void* addr) const { return JMP((std::uintptr_t)addr); }
	inline AsmPatchBuilder<Size + 5> jmp(std::uintptr_t addr) const {
		return byte(0xE9).dword(addr - cursor() - 5);
	}

	inline AsmPatchBuilder<Size + 13> safeCall(void* func) const { return safeCall((std::uintptr_t)func); }
	inline AsmPatchBuilder<Size + 13> safeCall(std::uintptr_t func) const {
		return (
			pushf().
			pushEAX().
			pushECX().
			pushEDX().
			call(func).
			popEDX().
			popECX().
			popEAX().
			popf()
			);
	}

	template <std::uintptr_t PadSize>
	inline AsmPatchBuilder<PadSize> nopPadToSize() const {
		static_assert(PadSize > Size, "Cannot pad smaller than old size");

		AsmPatchBuilder<PadSize> ret(mAddr);
		for (std::uintptr_t i = 0; i < Size; i++) {
			ret.mPatchBytes[i] = mPatchBytes[i];
		}

		for (std::uintptr_t i = Size; i < PadSize; i++) {
			ret.mPatchBytes[i] = 0x90;
		}

		return ret;
	}

	template <std::uintptr_t NopCount>
	inline AsmPatchBuilder<Size + NopCount> nops() const {
		return NOP_PAD_TO_SIZE<Size + NopCount>();
	}

	inline AsmPatchBuilder<Size + 2> condjmpToNopjmp() const {
		const uint8_t* ptr = (const uint8_t*)cursor();
		if ((ptr[0] != 0x0F) || ((ptr[1] & 0xF0) != 0x80)) {
			throw AsmPatchNoCondtionalJump();
		}
		return NOP().byte(0xE9);
	}
};

static inline AsmPatchBuilder<0> Patch(std::uintptr_t addr) {
	return AsmPatchBuilder<0>(addr);
}
static inline AsmPatchBuilder<0> Patch(void* addr) {
	return Patch(reinterpret_cast<std::uintptr_t>(addr));
}

}

