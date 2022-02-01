#include <unordered_set>

#include "variabletracehooks.h"
#include "trace.h"
#include "re_extras.h"

using namespace Trace;

namespace VariableTraceHooks
{
	// Object type name filter
	std::unordered_set<uint64_t> AllowedObjectTraceTypes;

	// Callstack logging on struct creation
	std::recursive_mutex StructInstanceToCallstackLock;
	std::unordered_map<RE::BSScript::Struct*, std::string> StructInstanceToCallstack;

	thread_local RE::BSScript::Stack* LastStructStackPointer;

	REL::Relocation<void __fastcall(uintptr_t, void*)> orig_BSScript__Internal__CodeTasklet__HandleStructCreate;
	void __fastcall hk_BSScript__Internal__CodeTasklet__HandleStructCreate(uintptr_t thisptr, void* a2)
	{
		LastStructStackPointer = *(RE::BSScript::Stack**)(thisptr + 0x10);
		orig_BSScript__Internal__CodeTasklet__HandleStructCreate(thisptr, a2);
		LastStructStackPointer = nullptr;
	}

	REL::Relocation<void __fastcall(RE::BSScript::Struct*)> orig_BSScript__Struct__Construct;
	void __fastcall hk_BSScript__Struct__Construct(RE::BSScript::Struct* thisptr)
	{
		orig_BSScript__Struct__Construct(thisptr);

		auto name = thisptr->type->name.c_str();
		auto nameHash = FNV1ALower(name, strlen(name));

		Print("Created a struct %s 0x%p", name, thisptr);

		if (AllowedObjectTraceTypes.empty() || AllowedObjectTraceTypes.contains(nameHash)) {
			char outputError[2048]{};
			RE::BSScript::FormatError(LastStructStackPointer, "Object creation site", 0, outputError, static_cast<uint32_t>(std::ssize(outputError)));

			StructInstanceToCallstackLock.lock();
			StructInstanceToCallstack[thisptr] = outputError;
			StructInstanceToCallstackLock.unlock();
		}
	}

	REL::Relocation<void __fastcall(RE::BSScript::Struct*)> orig_BSScript__Struct__dtorStruct;
	void __fastcall hk_BSScript__Struct__dtorStruct(RE::BSScript::Struct* thisptr)
	{
		// Fetch callstack text
		std::string callstackText;

		StructInstanceToCallstackLock.lock();  // This hurts a little bit
		{
			auto itr = StructInstanceToCallstack.find(thisptr);

			if (itr != StructInstanceToCallstack.end()) {
				callstackText = std::move(itr->second);
				StructInstanceToCallstack.erase(itr);
			}
		}
		StructInstanceToCallstackLock.unlock();

		// Do the log print
		if (ShouldPrintGCInfo()) {
			Print("About to destroy a struct:");
			IncPadding();

			char buffer[1024]{};
			RE::BSScript::StructCastToString(thisptr, buffer, static_cast<uint32_t>(std::ssize(buffer)), false);

			Print("%s 0x%p", thisptr->type->name.c_str(), thisptr);
			Print("%s", buffer);

			if (!callstackText.empty())
				Print("\n%s", callstackText.c_str());

			DecPadding();
		}

		orig_BSScript__Struct__dtorStruct(thisptr);
	}

	REL::Relocation<__int64 __fastcall(RE::BSScript::Object*)> orig_BSScript__Object__DecRef;
	__int64 __fastcall hk_BSScript__Object__DecRef(RE::BSScript::Object* thisptr)
	{
		char buffer[1024]{};

		if (ShouldPrintGCInfo())
			RE::BSScript::ObjectCastToString(thisptr, buffer, static_cast<uint32_t>(std::size(buffer)), false);

		auto result = orig_BSScript__Object__DecRef(thisptr);

		if (ShouldPrintGCInfo()) {
			if (result == 0) {
				Print("Released an object: 0x%p Handle: 0x%016llX", thisptr, thisptr->handle);
				Print(" %s", buffer);
			}
		}

		return result;
	}

	REL::Relocation<__int64 __fastcall(RE::BSScript::Variable*)> orig_BSScript__Variable__Cleanup;
	void __fastcall hk_BSScript__Variable__Cleanup(RE::BSScript::Variable* thisptr)
	{
		if (ShouldPrintGCInfo())
			Print("Cleaning up a variable: 0x%p", thisptr);

		orig_BSScript__Variable__Cleanup(thisptr);
	}

#define MAKE_HOOK(name, relid, instrlen) orig_##name = WriteHookWithReturn(relid, instrlen, hk_##name);

	void InstallHooks()
	{
		MAKE_HOOK(BSScript__Internal__CodeTasklet__HandleStructCreate, REL::ID(587593), 8);
		MAKE_HOOK(BSScript__Struct__Construct, REL::ID(459744), 6);
		MAKE_HOOK(BSScript__Struct__dtorStruct, REL::ID(877265), 8);
		MAKE_HOOK(BSScript__Object__DecRef, REL::ID(541793), 5);
		//MAKE_HOOK(BSScript__Variable__Cleanup, REL::ID(1183888), 5);

		logger::info("Installed object trace hooks");
	}

#undef MAKE_HOOK
}
