#include <combaseapi.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

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
#include "Console.hh"

//=============================================================================================

class ObjFile
{
public:
    char m_Name[128] = { '\0' };
    fastObjMesh* m_pObjMesh = nullptr;
    
    std::string m_XMLCfg;

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

            AddResource(pTexFile, texInfo.m_Suffix);
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

int main(int argc, char** argv)
{
    //-------------------------------------------------------------------------------------------
    // Prepare

    const char* pObjPath = Con::GetArgParam("-obj");
    if (!pObjPath)
    {
        UFG::qPrintf("ERROR: Missing argument for object file: '-obj'.\n");
        return 1;
    }

    const char* pOutPath = Con::GetArgParam("-out");
    if (pOutPath)
    {
        // Check if out contains "data", this is necessary for resource uid...
        if (!UFG::qStringFindLastInsensitive(pOutPath, "data\\") && !UFG::qStringFindLastInsensitive(pOutPath, "data/"))
        {
            UFG::qPrintf("ERROR: Output path doesn't contain 'data' folder.\n");
            return 1;
        }
    }
    else
    {
        UFG::qPrintf("ERROR: Missing argument for output file: '-out'.\n");
        return 1;
    }

    //-------------------------------------------------------------------------------------------
    // Handle object file

    if (!gObjFile.LoadFile(pObjPath))
    {
        UFG::qPrintf("ERROR: Failed to load object file.\n");
        return 1;
    }

    if (!gObjFile.InitializeTextures())
    {
        UFG::qPrintf("ERROR: Failed to initialize textures.\n");
        return 1;
    }

    //-------------------------------------------------------------------------------------------

    Hooks::Initialize();

    //-------------------------------------------------------------------------------------------
    // Scribe

    char permFile[MAX_PATH];
    char tempFile[MAX_PATH];

    sprintf_s(permFile, sizeof(permFile), "%s.perm.bin", pOutPath);
    sprintf_s(tempFile, sizeof(tempFile), "%s.temp.bin", pOutPath);

    bool bSuccess = UFG::qTextureScribe("PC64", CONFIG_FILE, permFile, tempFile, 0, 0, 0);

    //-------------------------------------------------------------------------------------------

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

        UFG::qInit();

        int exitCode = main(Con::GetArgc(), Con::GetArgv());
        if (exitCode != 0 && IsDebuggerPresent()) {
            DebugBreak();
        }

        exit(exitCode);
    }

    return 1;
}