#pragma once
#include <cstdint>

#define UFG_RVA(x)			(UFG::g_BaseAddress + x)
#define UFG_RVA_PTR(x)		reinterpret_cast<void*>(UFG::g_BaseAddress + x)

namespace UFG
{
	inline uintptr_t g_BaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleA(0));

	bool IsValidExecutable()
	{
		return (*reinterpret_cast<uint32_t*>(UFG_RVA(0xF8)) == 0x5578C12E);
	}


	template <typename... Args>
	void qPrintf(const char* p_Format, Args... p_Args)
	{
		reinterpret_cast<void(*)(const char*, Args...)>(UFG_RVA(0x876F))(p_Format, p_Args...);
	}

	bool qFileExists(const char* p_Filename)
	{
		return reinterpret_cast<bool(__fastcall*)(const char*)>(UFG_RVA(0x10D320))(p_Filename);
	}

	char* qMalloc(uint64_t p_Size, const char* p_Name, uint64_t p_AllocParams = 0)
	{
		return reinterpret_cast<char*(__fastcall*)(uint64_t, const char*, uint64_t)>(UFG_RVA(0x115280))(p_Size, p_Name, p_AllocParams);
	}

	void qInit()
	{
		reinterpret_cast<void(__fastcall*)(void*)>(UFG_RVA(0x110930))(0);
	}

	void qClose()
	{
		reinterpret_cast<void(__fastcall*)()>(UFG_RVA(0x1097A0))();
	}

	bool qTextureScribe(const char* p_Platform, const char* p_Config, const char* p_PermFile, const char* p_TempFile, const char* p_TextureCache, const char* p_Detail, const char* p_Unknown)
	{
		return reinterpret_cast<bool(__fastcall*)(const char*, const char*, const char*, const char*, const char*, const char*, const char*)>(UFG_RVA(0x132300))
			(p_Platform, p_Config, p_PermFile, p_TempFile, p_TextureCache, p_Detail, p_Unknown);
	}
}