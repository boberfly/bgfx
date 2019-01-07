#pragma once

#include <string>
#include <map>

//#include <tinystl/allocator.h>
//#include <tinystl/string.h>
//#include <tinystl/unordered_map.h>

//namespace stl = tinystl;

struct MaterialDef
{
    MaterialDef() : specExp(1.0f), ior(1.5f), diffuseMap(""), bmpMap("")
    {
        diffuseTint[0] = diffuseTint[1] = diffuseTint[2] = 1.0f;
        specTint[0] = specTint[1] = specTint[2] = 1.0f;
    }
    float specExp;
    float ior;
    float diffuseTint[3];
    float specTint[3];
    std::string metallicMap;
    std::string diffuseMap;
    std::string roughnessMap;
    std::string bmpMap;
};

std::map<std::string, MaterialDef> LoadMaterialFile(const std::string& filePath);