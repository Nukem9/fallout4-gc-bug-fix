# Fallout 4 Garbage Collector Logger & Bug Fix

```
<<< todo: actual coherent summary. writing isn't my forte. >>>

This is a compilation of debugging & research and will apply to all Fallout 4 script mods to some degree.

Allocating too many arrays *and immediately discarding them* will overload the VM garbage collector.
Allocating too many structs *and immediately discarding them* will overload the VM garbage collector.

Incremental GC pass:
Incremental GC runs once per Papyrus VM frame. ProcessArrayCleanup() and ProcessStructCleanup() are individually allotted 1% of the frame time budget. If a VM frame update takes 1 millisecond, they'll each have 10 microseconds to execute. It's *very* easy to allocate an excess of objects per frame to the point that the garbage collector can't keep up.

Full GC pass (FullGarbageCollectionPass):
Full GC runs on save or load. A "stop the world" GC pass will loop until there's no objects left to destroy. SS2's Long Save Bug happens when this cleanup occurs. The aforementioned loop, combined with the mistakes inside ProcessCleanup, cause time complexity to quickly spiral out of control.

ProcessArrayCleanup and ProcessStructCleanup:
Due to convoluted "last index" tracking, a bug was introduced that could cause both functions to prematurely break out of a loop and forgo their time slices after a single object was destroyed.
```

[Clayne](https://github.com/clayne) has provided an excellent perl script demo of the buggy ProcessCleanup function [here](https://github.com/clayne/random/blob/master/bin/fo4-gc-test).

Based off of [CommonLibF4](https://github.com/Ryan-rsm-McKenzie/CommonLibF4) and its [example plugin](https://github.com/Ryan-rsm-McKenzie/CommonLibF4/tree/master/ExampleProject) along with Fallout 4 VR support.

# Table of contents

* [Requirements](#Requirements)
* [Quick start](#Quick-start)
* [Notes](#Notes)
    * [Papyrus reproduction code](#Papyrus-reproduction-code)
    * [Original function pseudocode with bug included](#Original-function-pseudocode-with-bug-included)
    * [Rewritten function pseudocode with bug corrected](#Rewritten-function-pseudocode-with-bug-corrected)
    * [Full garbage collector pass pseudocode](#Full-garbage-collector-pass-pseudocode)
    * [Log format](#Log-format)
* [License](#License)

# Requirements

* Building
    * Microsoft Visual Studio 2022 or later
    * CMake 3.26 or later
    * vcpkg
* Installation
    * [F4SE](http://f4se.silverlock.org/)
    * [Address Library](https://www.nexusmods.com/fallout4/mods/47327)
    * [64-bit Visual C++ 2019/2022 Redistributable](https://aka.ms/vs/17/release/vc_redist.x64.exe)

# Quick start

1. Clone this repository and any submodules
2. Run `cmake --preset f4se & cmake --build --preset f4se-release & cpack --preset f4se`
3. Copy DLL and INI build artifacts to the game folder

# Notes

## Papyrus reproduction code
```papyrus
Struct PlotSubClass
EndStruct

Function TestKillTheGarbageManWithStructs
    int i = 0
    while (i < 1000)
        PlotSubClass a = new PlotSubClass
        i += 1
    endWhile
EndFunction ; all vars immediately discarded

Function TestKillTheGarbageManWithArrays
    int i = 0
    while (i < 1000)
        PlotSubClass[] a = new PlotSubClass[0]
        i += 1
    endWhile
EndFunction ; all vars immediately discarded

Function ScriptEntryPoint()
    TestKillTheGarbageManWithStructs()
    TestKillTheGarbageManWithArrays()
EndFunction
```

## Original function pseudocode with bug included
```c++
template<typename T>
bool ProcessEntries(float TimeBudget, BSTArray<BSTSmartPointer<T>>& Elements, uint32_t& NextIndexToClean)
{
    bool didGC = false;

    const uint64_t startTime = BSPrecisionTimer::GetTimer();
    const uint64_t budget = static_cast<uint64_t>(BSPrecisionTimer::FrequencyMS() * TimeBudget);

    // NextIndexToClean stores the last checked entry for iterative GC purposes.
    if (NextIndexToClean >= Elements.size())
        NextIndexToClean = Elements.size() - 1;

    uint32_t index = NextIndexToClean;

    if (!Elements.empty()) {
        do {
            if (Elements[index]->QRefCount() == 1) {
                didGC = true;
                Elements.RemoveFast(index);

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
            index != NextIndexToClean && // Break when 'index' == 'NextIndexToClean'. Refer to note 1. BUG: This CAN exit after a single object is collected, forgoing the remaining budget/timeslice.
            (TimeBudget <= 0 || (BSPrecisionTimer::GetTimer() - startTime) <= budget) &&
            !Elements.empty());
    }

    NextIndexToClean = index;
    return didGC;
}
```

## Rewritten function pseudocode with bug corrected
```c++
template<typename T>
bool ProcessEntries(float TimeBudget, BSTArray<BSTSmartPointer<T>>& Elements, uint32_t& NextIndexToClean)
{
    bool didGC = false;

    // Time budget only applies to incremental GCs
    const uint64_t startTime = BSPrecisionTimer::GetTimer();
    const uint64_t maxEndTime = startTime + static_cast<uint64_t>(BSPrecisionTimer::FrequencyMS() * TimeBudget);

    // Examine no more elements than the array currently holds, regardless of position. If no elements are
    // cleaned this will not be more than a full wrap-around. Special thanks for the concept: i860.
    uint32_t maximumElementsChecked = Elements.size();

    // NextIndexToClean stores the last checked entry for iterative GC purposes. If it's beyond the size of the
    // array, reset it to the start and go from there.
    uint32_t index = (NextIndexToClean < Elements.size()) ? NextIndexToClean : Elements.size() - 1;

    while (!Elements.empty()) {
        if (Elements[index]->QRefCount() == 1) {
            didGC = true;
            Elements.RemoveFast(index);
        }

        // Wrap around if necessary
        if (index-- == 0)
            index = Elements.size() - 1;

        // Bail if out of time
        if (TimeBudget > 0 && BSPrecisionTimer::GetTimer() >= maxEndTime)
            break;

        // Bail if we ran out of fresh entries
        if (maximumElementsChecked-- == 1)
            break;
    }

    NextIndexToClean = index;
    return didGC;
}
```

## Full garbage collector pass pseudocode
```c++
//
// "Ground truth" function to run a full garbage collector pass when a game save occurs.
//
// Execution times are estimates and measured with a large SS2 test save file with no patch
// applied. This code also assumes that subpasses have no timeout restriction, so certain
// parameters are omitted for clarity.
//
// Time cost:
//   30,000ms ~ 800,000ms
//
void FullGarbageCollectionPass(VirtualMachine *VM)
{
    // "Begin full garbage collection"

    for (bool didGC = true; didGC;)
    {
        // "Begin GC processing unneeded objects"
        //
        // Time cost:
        //  When no objects are cleaned: ~250ms
        //  When objects are cleaned:    ~250ms + 0.01~0.05ms per object
        didGC = VM->ProcessUnneededObjectCleanup();

        // "Begin GC processing objects"
        //
        // Time cost:
        //  When no objects are cleaned: 0ms
        //  When objects are cleaned:    0.01~0.05ms per object
        didGC = VM->ProcessObjectsCleanup() || didGC;

        // "Begin GC processing arrays"
        //
        // Time cost:
        //  When no arrays are cleaned:  0ms
        //  When arrays are cleaned:     0.01~0.05ms per array
        didGC = VM->ProcessArrayCleanup() || didGC;

        // "Begin GC processing structs"
        //
        // Time cost:
        //  When no structs are cleaned: 0ms
        //  When structs are cleaned:    0.01~0.05ms per struct
        didGC = VM->ProcessStructCleanup() || didGC;
    }

    // "End full garbage collection"
}
```

## Log format
### Timestamps
```
[19:38:20:145 TID 32280]

19 - Hour
38 - Minute
20 - Second
145 - Milliseconds
32280 - Thread ID
```

### Context breakdown
```
[19:17:21:217 TID  5664] GCBugFix v1.0.0 <---- Line 1. Game startup.

; The start of this log includes events all the way back to the main menu when a save is initially loaded. You'll see plenty of
; objects being created and destroyed. They're not very relevant to the bug. It's included for completeness.
;
; Note: "Begin processing X..." and "End processing X..." can occur outside of full GC passes. These are part of
; the per-VM-frame incremental garbage collector. Because they're allotted a miniscule time slice (~1%/frame), few objects
; are cleared.
;
; Note: Full garbage collection can happen multiple times - typically on transitions (loading screens) or saves.

[19:38:20:145 TID 32280]: =============================================== <---- Line 1,333,561. Demarks the start of a full GC pass. This is also the BEGINNING of the Long Hang.
[19:38:20:145 TID 32280]: Begin full garbage collection
[19:38:20:145 TID 32280]: ===============================================

; A list of objects, arrays, and structs being torn down. I don't think I have much else to explain here. I opted to include
; callstacks for PlotSubClass and PlotResource since the log becomes unwieldy with everything else.

[19:50:31:178 TID 32280]: =============================================== <---- Line 1,443,112. Demarks the end of a full GC pass. This is also the END of the Long Hang.
[19:50:31:178 TID 32280]: End full garbage collection (891908 ms)
[19:50:31:178 TID 32280]: ===============================================

; Random script noise after the save completes.
```

# License

* [GPLv3](COPYING) with [exceptions](EXCEPTIONS)
