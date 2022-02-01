#pragma once

#pragma warning(push)
#include "F4SE/F4SE.h"
#include "RE/Fallout.h"

#ifdef NDEBUG
#	include <spdlog/sinks/basic_file_sink.h>
#else
#	include <spdlog/sinks/msvc_sink.h>
#endif

#include <xbyak/xbyak.h>
#pragma warning(pop)

#define DLLEXPORT __declspec(dllexport)

namespace logger = F4SE::log;

using namespace std::literals;

#include "Version.h"

template<typename Func>
auto WriteHookWithReturn(REL::ID ID, size_t ByteCopyCount, Func Dest)
{
	struct Patch : Xbyak::CodeGenerator
	{
		explicit Patch(uintptr_t OriginalFuncAddr, size_t OriginalByteLength)
		{
			// Hook returns here. Execute the restored bytes and jump back to the original function.
			for (size_t i = 0; i < OriginalByteLength; i++)
				db(*reinterpret_cast<uint8_t*>(OriginalFuncAddr + i));

			jmp(qword[rip]);
			dq(OriginalFuncAddr + OriginalByteLength);
		}
	};

	REL::Relocation<std::uintptr_t> target{ ID };

	Patch p(target.address(), ByteCopyCount);
	p.ready();

	auto& trampoline = F4SE::GetTrampoline();
	trampoline.write_branch<5>(target.address(), Dest);

	auto alloc = trampoline.allocate(p.getSize());
	memcpy(alloc, p.getCode(), p.getSize());

	return reinterpret_cast<uintptr_t>(alloc);
}
