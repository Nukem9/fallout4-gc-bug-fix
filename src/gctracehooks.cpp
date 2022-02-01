#include "gctracehooks.h"
#include "trace.h"

using namespace std::chrono;
using namespace Trace;

namespace GCTraceHooks
{
	//
	// Full GC pass (everything)
	//
	REL::Relocation<void __fastcall(RE::BSScript::Internal::VirtualMachine*)> orig_BSScript__Internal__VirtualMachine__ForceFullGarbageCollection;
	void __fastcall hk_BSScript__Internal__VirtualMachine__ForceFullGarbageCollection(RE::BSScript::Internal::VirtualMachine* thisptr)
	{
		EnableFullGCInfo(true);

		if (ShouldPrintGCInfo()) {
			Print("===============================================");
			Print("Begin full garbage collection");
			Print("===============================================");
			IncPadding();
		}

		auto timerStart = high_resolution_clock::now();
		orig_BSScript__Internal__VirtualMachine__ForceFullGarbageCollection(thisptr);
		auto timerEnd = high_resolution_clock::now();

		if (ShouldPrintGCInfo()) {
			DecPadding();
			Print("===============================================");
			Print("End full garbage collection (%lld ms)", duration_cast<milliseconds>(timerEnd - timerStart).count());
			Print("===============================================");
		}

		EnableFullGCInfo(false);
	}

	//
	// Unused objects
	//
	REL::Relocation<__int64 __fastcall(RE::BSScript::Internal::VirtualMachine*, float)> orig_BSScript__Internal__VirtualMachine__ProcessUnneededObjectCleanup;
	__int64 __fastcall hk_BSScript__Internal__VirtualMachine__ProcessUnneededObjectCleanup(RE::BSScript::Internal::VirtualMachine* thisptr, float TimeBudget)
	{
		if (ShouldPrintGCInfo()) {
			Print("Begin GC processing unneeded objects...");
			IncPadding();
		}

		auto timerStart = high_resolution_clock::now();
		auto result = orig_BSScript__Internal__VirtualMachine__ProcessUnneededObjectCleanup(thisptr, TimeBudget);
		auto timerEnd = high_resolution_clock::now();

		if (ShouldPrintGCInfo()) {
			DecPadding();
			Print("End GC processing unneeded objects...result: %lld (%lld ms)", result, duration_cast<milliseconds>(timerEnd - timerStart).count());
		}

		return result;
	}

	//
	// Active objects
	//
	REL::Relocation<bool __fastcall(float, void*, void*, void*)> orig_ProcessObjectCleanup;
	bool __fastcall hk_ProcessObjectCleanup(float TimeBudget, void* a2, void* a3, void* a4)
	{
		if (ShouldPrintGCInfo()) {
			Print("Begin GC processing objects...");
			IncPadding();
		}

		auto timerStart = high_resolution_clock::now();
		auto result = orig_ProcessObjectCleanup(TimeBudget, a2, a3, a4);
		auto timerEnd = high_resolution_clock::now();

		if (ShouldPrintGCInfo()) {
			DecPadding();
			Print("End GC processing objects...result: %d (%lld ms)", result, duration_cast<milliseconds>(timerEnd - timerStart).count());
		}

		return result;
	}

	//
	// Arrays
	//
	REL::Relocation<bool __fastcall(RE::BSScript::Internal::VirtualMachine*, float)> orig_BSScript__Internal__VirtualMachine__ProcessArrayCleanup;
	bool __fastcall hk_BSScript__Internal__VirtualMachine__ProcessArrayCleanup(RE::BSScript::Internal::VirtualMachine* thisptr, float TimeBudget)
	{
		if (ShouldPrintGCInfo()) {
			Print("Begin GC processing arrays... Total: %u, Next index: %u", *(uint32_t*)((uintptr_t)thisptr + 0xBEA0), *(uint32_t*)((uintptr_t)thisptr + 0xBE88));
			IncPadding();
		}

		auto timerStart = high_resolution_clock::now();
		auto result = orig_BSScript__Internal__VirtualMachine__ProcessArrayCleanup(thisptr, TimeBudget);
		auto timerEnd = high_resolution_clock::now();

		if (ShouldPrintGCInfo()) {
			DecPadding();
			Print("End GC processing arrays...result: %d (%lld ms)", result, duration_cast<milliseconds>(timerEnd - timerStart).count());
		}

		return result;
	}

	//
	// Structs
	//
	REL::Relocation<bool __fastcall(RE::BSScript::Internal::VirtualMachine*, float)> orig_BSScript__Internal__VirtualMachine__ProcessStructCleanup;
	bool __fastcall hk_BSScript__Internal__VirtualMachine__ProcessStructCleanup(RE::BSScript::Internal::VirtualMachine* thisptr, float TimeBudget)
	{
		if (ShouldPrintGCInfo()) {
			Print("Begin GC processing structs... Total: %u, Next index: %u", *(uint32_t*)((uintptr_t)thisptr + 0xBE78), *(uint32_t*)((uintptr_t)thisptr + 0xBE60));
			IncPadding();
		}

		auto timerStart = high_resolution_clock::now();
		auto result = orig_BSScript__Internal__VirtualMachine__ProcessStructCleanup(thisptr, TimeBudget);
		auto timerEnd = high_resolution_clock::now();

		if (ShouldPrintGCInfo()) {
			DecPadding();
			Print("End GC processing structs...result: %d (%lld ms)", result, duration_cast<milliseconds>(timerEnd - timerStart).count());
		}

		return result;
	}

#define MAKE_HOOK(name, relid, instrlen) orig_##name = WriteHookWithReturn(relid, instrlen, hk_##name);

	void InstallHooks()
	{
		MAKE_HOOK(BSScript__Internal__VirtualMachine__ForceFullGarbageCollection, REL::ID(1202197), 5);
		MAKE_HOOK(BSScript__Internal__VirtualMachine__ProcessUnneededObjectCleanup, REL::ID(925208), 5);
		MAKE_HOOK(ProcessObjectCleanup, REL::ID(1400223), 6);  // Separate hook due to inlining
		MAKE_HOOK(BSScript__Internal__VirtualMachine__ProcessArrayCleanup, REL::ID(780305), 5);
		MAKE_HOOK(BSScript__Internal__VirtualMachine__ProcessStructCleanup, REL::ID(155003), 5);

		logger::info("Installed GC trace hooks");
	}

#undef MAKE_HOOK
}
