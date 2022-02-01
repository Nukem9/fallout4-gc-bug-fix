namespace Trace
{
	void Print(const char* Format, ...);

	void IncPadding();
	void DecPadding();

	void EnableFullGCInfo(bool Enable);
	bool ShouldPrintGCInfo();

	uint64_t FNV1ALower(const char* Buffer, size_t Length);
}
