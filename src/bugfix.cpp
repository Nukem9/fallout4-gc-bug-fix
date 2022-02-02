#include "bugfix.h"
#include "re_extras.h"

namespace Bugfix
{
	// I'm explicitly defining this function because BSScript::Array and BSScript::Struct don't reimplement new[] and
	// delete[] anywhere. I don't know if this is an issue with commonlib or an issue in my code. Noinline is strictly
	// for debugging purposes.
	//
	// There's an additional performance benefit - this implementation can move trivial items very quickly.
	template<typename T>
	__declspec(noinline) void BSTArrayRemoveFast(RE::BSTArray<RE::BSTSmartPointer<T>>& Elements, uint32_t Index)
	{
		using pfn = void(*)(RE::BSTArray<RE::BSTSmartPointer<T>>&, uint32_t, uint32_t);

		if constexpr (std::is_same_v<T, RE::BSScript::Array>) {
			static REL::Relocation<pfn> addr{ REL::ID(430486) };
			addr(Elements, Index, 1);
		}
		else if constexpr (std::is_same_v<T, RE::BSScript::Struct>) {
			static REL::Relocation<pfn> addr{ REL::ID(1294396) };
			addr(Elements, Index, 1);
		}
		else
			Elements.erase(&Elements[Index]);
	}

	template<typename T>
	bool ProcessCleanup_FixedVersion(float TimeBudget, RE::BSTArray<RE::BSTSmartPointer<T>>& Elements, uint32_t& NextIndexToClean, [[maybe_unused]] void* Unused)
	{
		bool didGC = false;

		// Time budget only applies to incremental GCs
		const uint64_t startTime = RE::BSPrecisionTimer::GetTimer();
		const uint64_t maxEndTime = startTime + static_cast<uint64_t>(RE::BSPrecisionTimer::FrequencyMS() * TimeBudget);

		// Examine no more elements than the array currently holds, regardless of position. If no elements are
		// cleaned this will not be more than a full wrap-around. Special thanks for the concept: i860.
		uint32_t maximumElementsChecked = Elements.size();

		// NextIndexToClean stores the last checked entry for iterative GC purposes. If it's beyond the size of the
		// array, reset it to the start and go from there.
		uint32_t index = (NextIndexToClean < Elements.size()) ? NextIndexToClean : Elements.size() - 1;

		while (!Elements.empty()) {
			if (Elements[index]->QRefCount() == 1) {
				didGC = true;
				BSTArrayRemoveFast(Elements, index);
			}

			// Wrap around if necessary
			if (index-- == 0)
				index = Elements.size() - 1;

			// Bail if out of time
			if (TimeBudget > 0 && RE::BSPrecisionTimer::GetTimer() >= maxEndTime)
				break;

			// Bail if we ran out of fresh entries
			if (maximumElementsChecked-- == 1)
				break;
		}

		NextIndexToClean = index;
		return didGC;
	}

	template<typename T>
	bool ProcessCleanup_BuggyVersion(float TimeBudget, RE::BSTArray<RE::BSTSmartPointer<T>>& Elements, uint32_t& NextIndexToClean, [[maybe_unused]] void* Unused)
	{
		bool didGC = false;

		const uint64_t startTime = RE::BSPrecisionTimer::GetTimer();
		const uint64_t budget = static_cast<uint64_t>(RE::BSPrecisionTimer::FrequencyMS() * TimeBudget);

		// NextIndexToClean stores the last checked entry for iterative GC purposes.
		if (NextIndexToClean >= Elements.size())
			NextIndexToClean = Elements.size() - 1;

		uint32_t index = NextIndexToClean;

		if (!Elements.empty()) {
			do {
				if (Elements[index]->QRefCount() == 1) {
					didGC = true;
					BSTArrayRemoveFast(Elements, index);

					if (!Elements.empty() && NextIndexToClean >= Elements.size())
						NextIndexToClean = Elements.size() - 1;

					// Note 1: 'index' isn't incremented when an entry is deleted. There's a chance that 'NextIndexToClean' == 'index' on the first loop.
				} else {
					index++;
				}

				// Note 2: Wrap around to the beginning of the array like a ring buffer.
				if (index >= Elements.size())
					index = 0;

			} while (
				index != NextIndexToClean && // Break when 'index' == 'NextIndexToClean'. Refer to note 1. This CAN exit after a single object is collected, forgoing the remaining budget/timeslice.
				(TimeBudget <= 0 || (RE::BSPrecisionTimer::GetTimer() - startTime) <= budget) &&
				!Elements.empty());
		}

		NextIndexToClean = index;
		return didGC;
	}

	void InstallHooks()
	{
		auto& trampoline = F4SE::GetTrampoline();

		REL::Relocation<std::uintptr_t> targetArray{ REL::ID(1068525) };
		trampoline.write_branch<5>(targetArray.address(), ProcessCleanup_FixedVersion<RE::BSScript::Array>);

		REL::Relocation<std::uintptr_t> targetStruct{ REL::ID(1466234) };
		trampoline.write_branch<5>(targetStruct.address(), ProcessCleanup_FixedVersion<RE::BSScript::Struct>);

		logger::info("Installed bugfix hooks");
	}
}
