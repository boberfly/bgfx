#pragma once

#include <bx/file.h>

#include <tinystl/allocator.h>
#include <tinystl/string.h>
#include <tinystl/unordered_map.h>
#include <tinystl/vector.h>
namespace stl = tinystl;

#include "mtlloader.h"
#include "bgfx_utils.h"
#include "entry/entry.h"
#include "camera.h"

#include "shader_defines.sh"

uint32_t packUint32(uint8_t _x, uint8_t _y, uint8_t _z, uint8_t _w);
uint32_t packF4u(float _x, float _y = 0.0f, float _z = 0.0f, float _w = 0.0f);

/*
struct Vec2
{
    float x, y;
};

Vec2 make_vec2(const float x, const float y)
{
    Vec2 result = Vec2();
    result.x = x;
    result.y = y;
    return result;
};

struct Vec4
{
    float x, y, z, w;
};

Vec4 make_vec4(const float x, const float y, const float z, const float w)
{
    Vec4 result = Vec4();
    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;
    return result;
};

struct Mat4
{
    Vec4 x;
    Vec4 y;
    Vec4 z;
    Vec4 w;
};
*/

struct Light
{
    union Position
    {
        struct
        {
            float m_x;
            float m_y;
            float m_z;
            float m_w;
        };

        float m_v[4];
    };

    Position m_position;
};

struct Uniforms
{
    void init()
    {
        u_params0          = bgfx::createUniform("u_params0",       bgfx::UniformType::Vec4);
        u_params1          = bgfx::createUniform("u_params1",       bgfx::UniformType::Vec4);
        u_quadPoints       = bgfx::createUniform("u_quadPoints",    bgfx::UniformType::Vec4, 4);
        u_samples          = bgfx::createUniform("u_samples",       bgfx::UniformType::Vec4, NUM_SAMPLES);

        u_albedo           = bgfx::createUniform("u_albedo",        bgfx::UniformType::Vec4);
        u_color            = bgfx::createUniform("u_color",         bgfx::UniformType::Vec4);

        u_lightMtx         = bgfx::createUniform("u_lightMtx",      bgfx::UniformType::Mat4);

        u_lightPosition    = bgfx::createUniform("u_lightPosition", bgfx::UniformType::Vec4);
        u_viewPosition     = bgfx::createUniform("u_viewPosition",  bgfx::UniformType::Vec4);
    }

    void setPtrs(Light* _lightPtr, float* _lightMtxPtr)
    {
        m_lightMtxPtr  = _lightMtxPtr;
        m_lightPtr     = _lightPtr;
    }

    // Call this once per frame.
    void submitPerFrameUniforms(bx::Vec3 viewPos)
    {
        bgfx::setUniform(u_params0, m_params0);
        bgfx::setUniform(u_samples, m_samples, NUM_SAMPLES);
        bgfx::setUniform(u_lightPosition, &m_lightPtr->m_position);
        bgfx::setUniform(u_viewPosition, &viewPos);
        
        bgfx::setUniform(u_albedo, &m_albedo[0]);
        bgfx::setUniform(u_color,  &m_color[0]);
    }

    void submitPerLightUniforms()
    {
        bgfx::setUniform(u_params1, m_params1);

        bgfx::setUniform(u_quadPoints, m_quadPoints, 4);
    }

    // Call this before each draw call.
    void submitPerDrawUniforms()
    {
        bgfx::setUniform(u_lightMtx, m_lightMtxPtr);
    }

    void destroy()
    {
        bgfx::destroy(u_params0);
        bgfx::destroy(u_params1);
        bgfx::destroy(u_quadPoints);
        bgfx::destroy(u_samples);

        bgfx::destroy(u_albedo);
        bgfx::destroy(u_color);

        bgfx::destroy(u_lightMtx);
        bgfx::destroy(u_lightPosition);
        bgfx::destroy(u_viewPosition);
    }

    union
    {
        struct
        {
            float m_reflectance;
            float m_roughness;
            float m_unused02;
            float m_sampleCount;
        };

        float m_params0[4];
    };
    
    union
    {
        struct
        {
            float m_lightIntensity;
            float m_twoSided;
            float m_unused22;
            float m_unused23;
        };
        
        float m_params1[4];
    };

    glm::vec4 m_samples[NUM_SAMPLES];
    glm::vec4 m_quadPoints[4];
    
    glm::vec4 m_albedo;
    glm::vec4 m_color;

    float* m_lightMtxPtr;
    float* m_colorPtr;
    Light* m_lightPtr;

private:
    bgfx::UniformHandle u_params0;
    bgfx::UniformHandle u_params1;
    bgfx::UniformHandle u_quadPoints;
    bgfx::UniformHandle u_samples;
    bgfx::UniformHandle u_albedo;
    bgfx::UniformHandle u_color;

    bgfx::UniformHandle u_lightMtx;
    bgfx::UniformHandle u_lightPosition;
    bgfx::UniformHandle u_viewPosition;
};

struct RenderState
{
    enum Enum
    {
        Default = 0,
        ZPass,
        ZTwoSidedPass,
        ColorPass,
        ColorAlphaPass,
        Count
    };

    uint64_t m_state;
    uint32_t m_blendFactorRgba;
    uint32_t m_fstencil;
    uint32_t m_bstencil;
};

struct Aabb
{
    float m_min[3];
    float m_max[3];
};

struct Obb
{
    float m_mtx[16];
};

struct Sphere
{
    float m_center[3];
    float m_radius;
};

struct Primitive
{
    uint32_t m_startIndex;
    uint32_t m_numIndices;
    uint32_t m_startVertex;
    uint32_t m_numVertices;

    Sphere m_sphere;
    Aabb m_aabb;
    Obb m_obb;
};

typedef stl::vector<Primitive> PrimitiveArray;

struct Group
{
    Group()
    {
        reset();
    }

    void reset()
    {
        m_vbh.idx = bgfx::kInvalidHandle;
        m_ibh.idx = bgfx::kInvalidHandle;
        m_prims.clear();
    }

    bgfx::VertexBufferHandle m_vbh;
    bgfx::IndexBufferHandle m_ibh;
    Sphere m_sphere;
    Aabb m_aabb;
    Obb m_obb;

    struct Material
    {
        bgfx::TextureHandle m_metallicMap;
        bgfx::TextureHandle m_diffuseMap;
        bgfx::TextureHandle m_nmlMap;
        bgfx::TextureHandle m_roughnessMap;

        glm::vec3 diffuseTint;
        glm::vec3 specTint;
        //bx::Vec3 diffuseTint;
        //bx::Vec3 specTint;
        float m_roughness;
    } m_material;

    PrimitiveArray m_prims;
};

namespace bgfx
{
    int32_t read(bx::ReaderI* _reader, bgfx::VertexDecl& _decl, bx::Error* _err = NULL);
}

struct LightMaps
{
    LightMaps() {}
    
    void destroyTextures()
    {
        bgfx::destroy(colorMap);
        bgfx::destroy(filteredMap);
    }
    
    bgfx::TextureHandle colorMap;
    bgfx::TextureHandle filteredMap;
};

struct GlobalRenderingData
{
    Uniforms s_uniforms;

    bgfx::UniformHandle s_uLTCMat;
    bgfx::UniformHandle s_uLTCAmp;
    bgfx::UniformHandle s_uColorMap;
    bgfx::UniformHandle s_uFilteredMap;

    bgfx::UniformHandle s_uDiffuseUniform;
    bgfx::UniformHandle s_uNmlUniform;
    bgfx::UniformHandle s_umetallicUniform;
    bgfx::UniformHandle s_uRoughnessUniform;
    
    bgfx::TextureHandle s_texLTCMat;
    bgfx::TextureHandle s_texLTCAmp;

    LightMaps s_texStainedGlassMaps;
    LightMaps s_texWhiteMaps;
};

struct Mesh
{
    bgfx::TextureHandle loadTexturePriv(const std::string& _fileName, std::string _fallBack = "", uint64_t _sampleFlags = 0)
    {
        if (_fallBack == std::string(""))
            _fallBack = _fileName;

        std::string fileName = _fileName == std::string("") ? _fallBack : _fileName;

        bgfx::TextureHandle hndl;
        if (m_textureCache.find(fileName) != m_textureCache.end())
            hndl = m_textureCache[fileName];
        else 
        {
            hndl = loadTexture(fileName.c_str(), _sampleFlags);
            m_textureCache[fileName] = hndl;
        }

        return hndl;
    }

    void load(const void* _vertices, uint32_t _numVertices, const bgfx::VertexDecl _decl, const uint16_t* _indices, uint32_t _numIndices)
    {
        Group group;
        const bgfx::Memory* mem;
        uint32_t size;

        size = _numVertices*_decl.getStride();
        mem = bgfx::makeRef(_vertices, size);
        group.m_vbh = bgfx::createVertexBuffer(mem, _decl);

        size = _numIndices*2;
        mem = bgfx::makeRef(_indices, size);
        group.m_ibh = bgfx::createIndexBuffer(mem);

        group.m_material.m_metallicMap  = loadTexturePriv("textures/black.png");
        group.m_material.m_diffuseMap   = loadTexturePriv("textures/white.png");
        group.m_material.m_nmlMap       = loadTexturePriv("textures/nml.tga");
        group.m_material.m_roughnessMap = loadTexturePriv("textures/white.png");

        //TODO:
        // group.m_sphere = ...
        // group.m_aabb = ...
        // group.m_obb = ...
        // group.m_prims = ...

        m_groups.push_back(group);
    }

    void load(const char* _filePath)
    {
#define BGFX_CHUNK_MAGIC_VB  BX_MAKEFOURCC('V', 'B', ' ', 0x1)
#define BGFX_CHUNK_MAGIC_IB  BX_MAKEFOURCC('I', 'B', ' ', 0x0)
#define BGFX_CHUNK_MAGIC_PRI BX_MAKEFOURCC('P', 'R', 'I', 0x0)

        // load material file
        std::string fileStr = _filePath;
        std::string mtlFilePath = fileStr.substr(0, fileStr.find_last_of(".")) + ".mtl";
        std::map<std::string, MaterialDef> mtlDefs = LoadMaterialFile(mtlFilePath);

        bx::FileReader reader;
        bx::open(&reader, _filePath);

        Group group;

        uint32_t chunk;
        while (4 == bx::read(&reader, chunk))
        {
            switch (chunk)
            {
            case BGFX_CHUNK_MAGIC_VB:
                {
                    bx::read(&reader, group.m_sphere);
                    bx::read(&reader, group.m_aabb);
                    bx::read(&reader, group.m_obb);

                    bgfx::read(&reader, m_decl);
                    uint16_t stride = m_decl.getStride();

                    uint16_t numVertices;
                    bx::read(&reader, numVertices);
                    const bgfx::Memory* mem = bgfx::alloc(numVertices*stride);
                    bx::read(&reader, mem->data, mem->size);

                    group.m_vbh = bgfx::createVertexBuffer(mem, m_decl);
                }
                break;

            case BGFX_CHUNK_MAGIC_IB:
                {
                    uint32_t numIndices;
                    bx::read(&reader, numIndices);
                    const bgfx::Memory* mem = bgfx::alloc(numIndices*2);
                    bx::read(&reader, mem->data, mem->size);
                    group.m_ibh = bgfx::createIndexBuffer(mem);
                }
                break;

            case BGFX_CHUNK_MAGIC_PRI:
                {
                    uint16_t len;
                    bx::read(&reader, len);

                    // read material name in
                    std::string material;
                    material.resize(len);
                    bx::read(&reader, const_cast<char*>(material.c_str()), len);

                    // convert material definition over and load texture
                    MaterialDef& matDef = mtlDefs[material];
                    group.m_material.diffuseTint = glm::vec3(matDef.diffuseTint[0], matDef.diffuseTint[1], matDef.diffuseTint[2]);
                    group.m_material.specTint = glm::vec3(matDef.specTint[0], matDef.specTint[1], matDef.specTint[2]);
                    group.m_material.m_roughness = pow(2.0f/(2.0f + matDef.specExp), 0.25f);

                    const uint64_t samplerFlags = BGFX_SAMPLER_MIN_ANISOTROPIC;

                    // load textures
                    group.m_material.m_diffuseMap   = loadTexturePriv(matDef.diffuseMap,   std::string("textures/white.png"), samplerFlags | BGFX_TEXTURE_SRGB);
                    group.m_material.m_nmlMap       = loadTexturePriv(matDef.bmpMap,       std::string("textures/nml.tga"),   samplerFlags);
                    group.m_material.m_roughnessMap = loadTexturePriv(matDef.roughnessMap, std::string("textures/white.png"), samplerFlags);
                    group.m_material.m_metallicMap  = loadTexturePriv(matDef.metallicMap,  std::string("textures/black.png"), samplerFlags);
                    
                    // read primitive data
                    uint16_t num;
                    bx::read(&reader, num);

                    for (uint32_t ii = 0; ii < num; ++ii)
                    {
                        bx::read(&reader, len);

                        stl::string name;
                        name.resize(len);
                        bx::read(&reader, const_cast<char*>(name.c_str()), len);

                        Primitive prim;
                        bx::read(&reader, prim.m_startIndex);
                        bx::read(&reader, prim.m_numIndices);
                        bx::read(&reader, prim.m_startVertex);
                        bx::read(&reader, prim.m_numVertices);
                        bx::read(&reader, prim.m_sphere);
                        bx::read(&reader, prim.m_aabb);
                        bx::read(&reader, prim.m_obb);

                        group.m_prims.push_back(prim);
                    }

                    m_groups.push_back(group);
                    group.reset();
                }
                break;

            default:
                DBG("%08x at %d", chunk, bx::seek(&reader));
                break;
            }
        }

        bx::close(&reader);
    }

    void unload()
    {
        for (GroupArray::const_iterator it = m_groups.begin(), itEnd = m_groups.end(); it != itEnd; ++it)
        {
            const Group& group = *it;
            bgfx::destroy(group.m_vbh);

            if (bgfx::kInvalidHandle != group.m_ibh.idx)
            {
                bgfx::destroy(group.m_ibh);
            }
        }
        m_groups.clear();
    }

    void submit(GlobalRenderingData& rdata, uint8_t _viewId, float* _mtx, bgfx::ProgramHandle _program,
                const LightMaps& colorMaps, const RenderState& _renderState)
    {
        for (GroupArray::const_iterator it = m_groups.begin(), itEnd = m_groups.end(); it != itEnd; ++it)
        {
            const Group& group = *it;

            // Set uniforms.
            rdata.s_uniforms.submitPerDrawUniforms();

            // Set model matrix for rendering.
            bgfx::setTransform(_mtx);
            bgfx::setIndexBuffer(group.m_ibh);
            bgfx::setVertexBuffer(0, group.m_vbh);

            bgfx::setTexture(0, rdata.s_uLTCMat,            rdata.s_texLTCMat);
            bgfx::setTexture(1, rdata.s_uLTCAmp,            rdata.s_texLTCAmp);
            bgfx::setTexture(2, rdata.s_uColorMap,          colorMaps.colorMap);
            bgfx::setTexture(3, rdata.s_uFilteredMap,       colorMaps.filteredMap);
            bgfx::setTexture(4, rdata.s_uDiffuseUniform,    group.m_material.m_diffuseMap);
            bgfx::setTexture(5, rdata.s_uNmlUniform,        group.m_material.m_nmlMap);
            bgfx::setTexture(6, rdata.s_uRoughnessUniform,  group.m_material.m_roughnessMap);
            bgfx::setTexture(7, rdata.s_umetallicUniform,   group.m_material.m_metallicMap);

            // Apply render state.
            bgfx::setStencil(_renderState.m_fstencil, _renderState.m_bstencil);
            bgfx::setState(_renderState.m_state, _renderState.m_blendFactorRgba);

            // Submit.
            bgfx::submit(_viewId, _program);
        }
    }

    bgfx::VertexDecl m_decl;
    typedef stl::vector<Group> GroupArray;
    GroupArray m_groups;

    static std::map<std::string, bgfx::TextureHandle> m_textureCache;
};

struct Model
{
    glm::mat4 transform;
    Mesh mesh;

    void loadModel(const char* fileName)
    {
        mesh.load(fileName);
    }

    void loadModel(const void* _vertices, uint32_t _numVertices, const bgfx::VertexDecl _decl,
                  const uint16_t* _indices, uint32_t _numIndices)
    {
        mesh.load(_vertices, _numVertices, _decl, _indices, _numIndices);
    }

    void unload()
    {
        mesh.unload();
    }

    void submit(GlobalRenderingData& rdata, uint8_t _viewId, bgfx::ProgramHandle _program, 
                const LightMaps& colorMaps, const RenderState& _renderState)
    {
        mesh.submit(rdata, _viewId, glm::value_ptr(transform), _program, colorMaps, _renderState);
    }
};

struct LightData
{
    LightData() : textureIdx(0), intensity(4.0f), twoSided(false) { }
    
    //bx::Vec3 rotation;
	//Vec2 scale;
	//bx::Vec3 position;
	//bx::Vec3 color;
    glm::vec3 rotation;
	glm::vec2 scale;
	glm::vec3 position;
	glm::vec3 color;
    uint32_t textureIdx;
    
    float intensity;
    bool  twoSided;
};

inline uint32_t packUint32(uint8_t _x, uint8_t _y, uint8_t _z, uint8_t _w)
{
    union
    {
        uint32_t ui32;
        uint8_t arr[4];
    } un;

    un.arr[0] = _x;
    un.arr[1] = _y;
    un.arr[2] = _z;
    un.arr[3] = _w;

    return un.ui32;
}

inline uint32_t packF4u(float _x, float _y, float _z, float _w)
{
    const uint8_t xx = uint8_t(_x*127.0f + 128.0f);
    const uint8_t yy = uint8_t(_y*127.0f + 128.0f);
    const uint8_t zz = uint8_t(_z*127.0f + 128.0f);
    const uint8_t ww = uint8_t(_w*127.0f + 128.0f);
    return packUint32(xx, yy, zz, ww);
}

struct RenderList 
{
    struct Model* models;
    uint64_t count;
};

// meshes for demo
enum 
{
    eBunny,
    eCube,
    eHollowCube,
    eHPlane,
    eModelCount,

    eLightCount = 1,
};

static Model s_models[eModelCount];
static Model s_lights[eLightCount];
static LightData s_lightData[eLightCount];
