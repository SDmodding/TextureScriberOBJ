#pragma once

namespace Con
{
	int GetArgc()
	{
		return *reinterpret_cast<int*>(UFG_RVA(0x3D8EC4C));
	}

	char** GetArgv()
	{
		return *reinterpret_cast<char***>(UFG_RVA(0x3D8EC50));
	}

	//================================================================
	// Arguments

	const char* GetArgParam(const char* p_Arg)
	{
		int argc = GetArgc();
		char** argv = GetArgv();
		for (int i = 0; argc > i; ++i)
		{
			if (_stricmp(p_Arg, argv[i]) != 0) {
				continue;
			}

			int nIdx = (i + 1);
			if (nIdx >= argc) {
				break;
			}

			return argv[nIdx];
		}

		return nullptr;
	}

	bool IsArgSet(const char* p_Arg)
	{
		int argc = GetArgc();
		char** argv = GetArgv();
		for (int i = 0; argc > i; ++i)
		{
			if (_stricmp(p_Arg, argv[i]) == 0) {
				return true;
			}
		}

		return false;
	}
}