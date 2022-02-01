namespace RE
{
	class BSPrecisionTimer
	{
	public:
		__declspec(noinline) static uint64_t GetTimer();
		__declspec(noinline) static float FrequencyMS();
	};

	namespace BSScript
	{
		void FormatError(Stack* Stack, const char* Text, int Severity, char* Out, uint32_t StringSize);
		void ObjectCastToString(Object* Object, char* Buffer, uint32_t BufferSize, bool ObjectHandleOnly);
		void StructCastToString(Struct* Struct, char* Buffer, uint32_t BufferSize, bool ObjectHandleOnly);
	}
}
