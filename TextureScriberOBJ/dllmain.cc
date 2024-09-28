#include <combaseapi.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

//=============================================================================================
// Defines

#define CONFIG_FILE         "cfg.xml"
#define DIFFUSE_FILE        "Diffuse"
#define SPECULAR_FILE       "SpecularLook"
#define NORMALMAP_FILE      "NormalMap"

//=============================================================================================
// Headers

#include "Dependencies/MinHook/MinHook.h"
#include "Dependencies/fast_obj.h"

#include "UFG.hh"

//=============================================================================================

class ObjFile
{
public:
    char m_Name[128] = { '\0' };
    fastObjMesh* m_pObjMesh = nullptr;
    
    std::string m_XMLCfg;

    void Cleanup()
    {
        DeleteFileA(DIFFUSE_FILE);
        DeleteFileA(SPECULAR_FILE);
        DeleteFileA(NORMALMAP_FILE);
    }

    void AddResource(const char* p_File, const char* p_Suffix)
    {
        char resource[512];
        sprintf_s(resource, sizeof(resource), R"(<Resource OutputName="%s%s">%s</Resource>)", m_Name, p_Suffix, p_File);
        m_XMLCfg.append(resource);
    }

    char* GetXMLConfig()
    {
        m_XMLCfg.insert(0, "<MediaPack>");
        m_XMLCfg.append("</MediaPack>");

        char* pXML = UFG::qMalloc(m_XMLCfg.size(), "XML");
        if (pXML) {
            memcpy(pXML, m_XMLCfg.data(), m_XMLCfg.size());
        }
        return pXML;
    }

    bool InitializeTextures()
    {
        if (!m_pObjMesh->material_count) {
            return false;
        }

        auto pMat = &m_pObjMesh->materials[0];

        struct TexInfo_t
        {
            uint32_t m_Idx;
            const char* m_Filename;
            const char* m_Suffix;
        };
        TexInfo_t texList[] = {
            { pMat->map_Kd, DIFFUSE_FILE, "_D" },
            { pMat->map_Ks, SPECULAR_FILE, "_S" },
            { pMat->map_bump, NORMALMAP_FILE, "_N" }
        };

        for (auto& texInfo : texList)
        {
            if (!texInfo.m_Idx) {
                continue;
            }

            auto pTex = &m_pObjMesh->textures[texInfo.m_Idx];

            const char* pTexFile = nullptr;
            if (UFG::qFileExists(pTex->path)) {
                pTexFile = pTex->path;
            }
            else if (UFG::qFileExists(pTex->name)) {
                pTexFile = pTex->name;
            }

            if (!pTexFile) {
                continue;
            }

            if (CreateSymbolicLinkA(texInfo.m_Filename, pTexFile, SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) == 0) 
            {
                printf("ERROR: Failed to create symbolic link for '%s'.\n", pTexFile);
                return false;
            }

            AddResource(texInfo.m_Filename, texInfo.m_Suffix);
        }

        return true;
    }

    bool LoadFile(const char* p_FilePath)
    {
        auto pObjMesh = fast_obj_read(p_FilePath);
        if (!pObjMesh) {
            return false;
        }

        const char* pName = strrchr(p_FilePath, '\\');
        if (!pName) {
            pName = strrchr(p_FilePath, '/');
        }

        strncpy(m_Name, (pName ? &pName[1] : p_FilePath), sizeof(m_Name));
        char* pDot = strchr(m_Name, '.');
        if (pDot) {
            *pDot = '\0';
        }

        m_pObjMesh = pObjMesh;
        return true;
    }
};
ObjFile gObjFile;

//=============================================================================================
// Hooks

namespace Hooks
{
    namespace SimpleXML::XMLCache
    {
        char* __fastcall ExtractFromCache(const char* p_Filename)
        {
            if (strcmp(p_Filename, CONFIG_FILE) == 0) {
                return gObjFile.GetXMLConfig();
            }

            return 0;
        }
    }

    void Initialize()
    {
        MH_Initialize();

        MH_CreateHook(UFG_RVA_PTR(0x8355), SimpleXML::XMLCache::ExtractFromCache, 0);

        MH_EnableHook(MH_ALL_HOOKS);
    }
}

//=============================================================================================
// fake main

int main(int argc, wchar_t** argv)
{
    wchar_t* pObjPath = nullptr;
    for (int i = 0; argc > i; ++i)
    {
        if (wcsstr(argv[i], L".obj")) {
            pObjPath = argv[i]; break;
        }
    }

    if (!pObjPath)
    {
        printf("ERROR: Failed to find object file in arguments.\n");
        return 1;
    }

    char mbObjPath[MAX_PATH] = { '\0' };
    wcstombs(mbObjPath, pObjPath, sizeof(mbObjPath));

    if (!gObjFile.LoadFile(mbObjPath)) 
    {
        printf("ERROR: Failed to load object file.\n");
        return 1;
    }

    if (!gObjFile.InitializeTextures())
    {
        printf("ERROR: Failed to initialize textures.\n");
        return 1;
    }

    Hooks::Initialize();

    bool bSuccess = UFG::qTextureScribe("PC64", CONFIG_FILE, "Data/Test.perm.bin", "Data/Test.temp.bin", 0, 0, 0);

    return (!bSuccess);
}

//=============================================================================================
// Entrypoint

int __stdcall DllMain(HMODULE p_hModule, DWORD p_dwReason, void* p_Reserved)
{
    if (p_dwReason == DLL_PROCESS_ATTACH)
    {
        if (!UFG::IsValidExecutable()) 
        {
            UFG::qPrintf("WARN: This is an invalid executable, preventing initilization of the module.\n");
            return 0;
        }

        if (CoInitializeEx(0, 0) != S_OK) 
        {
            UFG::qPrintf("ERROR: Failed to initialize COM library.\n");
            return 0;
        }

        wchar_t* pCmdLine = GetCommandLineW();
        if (!pCmdLine) 
        {
            printf("ERROR: Failed to retrieve command line string.\n");
            return 0;
        }

        int argc;
        wchar_t** argv = CommandLineToArgvW(pCmdLine, &argc);
        if (!argv)
        {
            printf("ERROR: Failed to retrieve command line arguments.\n");
            return 0;
        }

        UFG::qInit();
        gObjFile.Cleanup();

        int exitCode = main(argc, argv);
        if (exitCode != 0 && IsDebuggerPresent()) {
            DebugBreak();
        }

        gObjFile.Cleanup();
        exit(exitCode);
    }

    return 1;
}