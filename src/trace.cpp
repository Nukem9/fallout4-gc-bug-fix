namespace Trace
{
	// GC level indentation
	thread_local int CurrentPadding = 0;

	// GC mode logging
	thread_local bool IsInFullGCMode = false;

	void Print(const char* Format, ...)
	{
		// Yes, I know this is less than optimal. spdlog doesn't have indentation.
		char buffer[4096]{};
		memset(buffer, ' ', CurrentPadding);

		va_list va;
		va_start(va, Format);
		_vsnprintf_s(buffer + CurrentPadding, std::size(buffer) - CurrentPadding, _TRUNCATE, Format, va);
		va_end(va);

		spdlog::info(buffer);
	}

	void IncPadding()
	{
		CurrentPadding += 2;
	}

	void DecPadding()
	{
		CurrentPadding -= 2;
	}

	void EnableFullGCInfo(bool Enable)
	{
		IsInFullGCMode = Enable;
	}

	bool ShouldPrintGCInfo()
	{
		return true;
		// return IsInFullGCMode;
	}

	uint64_t FNV1ALower(const char* Buffer, size_t Length)
	{
		uint64_t hash = 0x100000001B3ull;

		for (size_t i = 0; i < Length; i++) {
			hash ^= std::tolower(Buffer[i]);
			hash *= 0xCBF29CE484222325ull;
		}

		return hash;
	}
}
