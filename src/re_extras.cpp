#include "re_extras.h"

namespace RE
{
	uint64_t BSPrecisionTimer::GetTimer()
	{
		static REL::Relocation<decltype(&GetTimer)> func{ REL::ID(1300983) };
		return func();
	}

	float BSPrecisionTimer::FrequencyMS()
	{
		static REL::Relocation<float> var{ REL::ID(711608) };
		return var.get();
	}

	namespace BSScript
	{
		void FormatError(Stack* Stack, const char* Text, int Severity, char* Out, uint32_t StringSize)
		{
			static REL::Relocation<decltype(&FormatError)> func{ REL::ID(986321) };
			func(Stack, Text, Severity, Out, StringSize);
		}

		void ObjectCastToString(Object* Object, char* Buffer, uint32_t BufferSize, bool ObjectHandleOnly)
		{
			static REL::Relocation<decltype(&ObjectCastToString)> func{ REL::ID(495491) };
			func(Object, Buffer, BufferSize, ObjectHandleOnly);
		}

		void StructCastToString(Struct* Struct, char* Buffer, uint32_t BufferSize, bool ObjectHandleOnly)
		{
			static REL::Relocation<decltype(&StructCastToString)> func{ REL::ID(1223123) };
			func(Struct, Buffer, BufferSize, ObjectHandleOnly);
		}
	}
}
