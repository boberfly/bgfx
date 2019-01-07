// standard includes
#include <algorithm>

#include "common.h"

// external libraries
#include <glm/glm.hpp>
#include <glm/ext.hpp>

using namespace glm;

// core includes
#include <bgfx/bgfx.h>
#include <bx/timer.h>
#include <bx/math.h>
//#include <bx/fpumath.h>
#include "imgui/imgui.h"

// project includes
#include "arealights.h"


struct PosNormalTexcoordVertex
{
    float    m_x;
    float    m_y;
    float    m_z;
    uint32_t m_normal;
    float    m_u;
    float    m_v;
};

static PosNormalTexcoordVertex s_vplaneVertices[] =
{
    { -1.0f,  1.0f, 0.0f, packF4u(0.0f, 0.0f, -1.0f), 0.0f, 0.0f },
    {  1.0f,  1.0f, 0.0f, packF4u(0.0f, 0.0f, -1.0f), 1.0f, 0.0f },
    { -1.0f, -1.0f, 0.0f, packF4u(0.0f, 0.0f, -1.0f), 0.0f, 1.0f },
    {  1.0f, -1.0f, 0.0f, packF4u(0.0f, 0.0f, -1.0f), 1.0f, 1.0f },
};

struct PosNormalTangentTexcoordVertex
{
    float    m_x;
    float    m_y;
    float    m_z;
    uint32_t m_normal;
    uint32_t m_tangent;
    float    m_u;
    float    m_v;
};

// horizontal/vertical plane geometry
static const float s_texcoord = 50.0f;
static PosNormalTangentTexcoordVertex s_hplaneVertices[] =
{
    { -1.0f, 0.0f,  1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(0.0f, 0.0f, 1.0f), s_texcoord, s_texcoord },
    {  1.0f, 0.0f,  1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(0.0f, 0.0f, 1.0f), s_texcoord, 0.0f       },
    { -1.0f, 0.0f, -1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(0.0f, 0.0f, 1.0f), 0.0f,       s_texcoord },
    {  1.0f, 0.0f, -1.0f, packF4u(0.0f, 1.0f, 0.0f), packF4u(0.0f, 0.0f, 1.0f), 0.0f,       0.0f       },
};

static const uint16_t s_planeIndices[] =
{
    0, 1, 2,
    1, 3, 2,
};

struct PosColorTexCoord0VertexA
{
    float m_x;
    float m_y;
    float m_z;
    uint32_t m_rgba;
    float m_u;
    float m_v;

    static void init()
    {
        ms_decl
            .begin()
            .add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0,    4, bgfx::AttribType::Uint8, true)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();
    }

    static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosColorTexCoord0VertexA::ms_decl;

#define RENDERVIEW_DRAWSCENE_0_ID 1

GlobalRenderingData s_gData;

std::map<std::string, bgfx::TextureHandle> Mesh::m_textureCache;

static RenderState s_renderStates[RenderState::Count] =
{
    { // Default
        0
        | BGFX_STATE_WRITE_RGB
        | BGFX_STATE_WRITE_A
        | BGFX_STATE_DEPTH_TEST_LEQUAL
        | BGFX_STATE_WRITE_Z
        | BGFX_STATE_CULL_CCW
        | BGFX_STATE_MSAA
        , UINT32_MAX
        , BGFX_STENCIL_NONE
        , BGFX_STENCIL_NONE
    },
    { // ZPass
        0
        | BGFX_STATE_WRITE_Z
        | BGFX_STATE_DEPTH_TEST_LEQUAL
        | BGFX_STATE_CULL_CCW
        | BGFX_STATE_MSAA
        , UINT32_MAX
        , BGFX_STENCIL_NONE
        , BGFX_STENCIL_NONE
    },
    { // ZTwoSidedPass
        0
        | BGFX_STATE_WRITE_Z
        | BGFX_STATE_DEPTH_TEST_LEQUAL
        | BGFX_STATE_MSAA
        , UINT32_MAX
        , BGFX_STENCIL_NONE
        , BGFX_STENCIL_NONE
    },
    { // ColorPass
        0
        | BGFX_STATE_WRITE_RGB
        | BGFX_STATE_WRITE_A
        | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE)
        | BGFX_STATE_DEPTH_TEST_EQUAL
        | BGFX_STATE_CULL_CCW
        | BGFX_STATE_MSAA
        , UINT32_MAX
        , BGFX_STENCIL_NONE
        , BGFX_STENCIL_NONE
    },
    { // ColorAlphaPass
        0
        | BGFX_STATE_WRITE_RGB
        | BGFX_STATE_WRITE_A
        | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE)
        | BGFX_STATE_DEPTH_TEST_EQUAL
        | BGFX_STATE_MSAA
        , UINT32_MAX
        , BGFX_STENCIL_NONE
        , BGFX_STENCIL_NONE
    }
};

struct Programs
{
    void init()
    {
        // Misc.
        m_colorConstant = loadProgram("vs_arealights_color", "fs_arealights_color_constant");
        m_colorTextured = loadProgram("vs_arealights_textured", "fs_arealights_color_textured");

        // Pack depth.
        m_packDepth      = loadProgram("vs_arealights_packdepth", "fs_arealights_packdepth");
        m_packDepthLight = loadProgram("vs_arealights_packdepth", "fs_arealights_packdepth_light");
        
        m_blit = loadProgram("vs_arealights_blit", "fs_arealights_blit");

        // Color lighting.
        m_diffuseLTC      = loadProgram("vs_arealights_lighting", "fs_arealights_diffuse_ltc");
        m_diffuseGT       = loadProgram("vs_arealights_lighting", "fs_arealights_diffuse_gt");
        m_specularLTC     = loadProgram("vs_arealights_lighting", "fs_arealights_specular_ltc");
        m_specularGT      = loadProgram("vs_arealights_lighting", "fs_arealights_specular_gt");
    }

    void destroy()
    {
        // Color lighting.
        bgfx::destroy(m_diffuseLTC);
        bgfx::destroy(m_diffuseGT);
        bgfx::destroy(m_specularLTC);
        bgfx::destroy(m_specularGT);

        // Pack depth.
        bgfx::destroy(m_packDepth);
        bgfx::destroy(m_packDepthLight);

        // Blit.
        bgfx::destroy(m_blit);

        // Misc.
        bgfx::destroy(m_colorConstant);
        bgfx::destroy(m_colorTextured);
    }

    bgfx::ProgramHandle m_colorConstant;
    bgfx::ProgramHandle m_colorTextured;
    bgfx::ProgramHandle m_packDepth;
    bgfx::ProgramHandle m_packDepthLight;
    bgfx::ProgramHandle m_blit;
    bgfx::ProgramHandle m_diffuseLTC;
    bgfx::ProgramHandle m_diffuseGT;
    bgfx::ProgramHandle m_specularLTC;
    bgfx::ProgramHandle m_specularGT;
};
static Programs s_programs;

void screenSpaceQuad(bool _originBottomLeft = true, float zz = 0.0f, float _width = 1.0f, float _height = 1.0f)
{
    if (bgfx::getAvailTransientVertexBuffer(3, PosColorTexCoord0VertexA::ms_decl))
    {
        bgfx::TransientVertexBuffer vb;
        bgfx::allocTransientVertexBuffer(&vb, 3, PosColorTexCoord0VertexA::ms_decl);
        PosColorTexCoord0VertexA* vertex = (PosColorTexCoord0VertexA*)vb.data;

        const float minx = -_width;
        const float maxx =  _width;
        const float miny = 0.0f;
        const float maxy = _height*2.0f;

        const float minu = -1.0f;
        const float maxu =  1.0f;

        float minv = 0.0f;
        float maxv = 2.0f;

        if (_originBottomLeft)
        {
            std::swap(minv, maxv);
            minv -= 1.0f;
            maxv -= 1.0f;
        }

        vertex[0].m_x = minx;
        vertex[0].m_y = miny;
        vertex[0].m_z = zz;
        vertex[0].m_rgba = 0xffffffff;
        vertex[0].m_u = minu;
        vertex[0].m_v = minv;

        vertex[1].m_x = maxx;
        vertex[1].m_y = miny;
        vertex[1].m_z = zz;
        vertex[1].m_rgba = 0xffffffff;
        vertex[1].m_u = maxu;
        vertex[1].m_v = minv;

        vertex[2].m_x = maxx;
        vertex[2].m_y = maxy;
        vertex[2].m_z = zz;
        vertex[2].m_rgba = 0xffffffff;
        vertex[2].m_u = maxu;
        vertex[2].m_v = maxv;

        bgfx::setVertexBuffer(0, &vb);
    }
}

static float Halton(int index, float base)
{
    float result = 0.0f;
    float f = 1.0f/base;
    float i = float(index);
    for (;;)
    {
        if (i <= 0.0f)
            break;

        result += f*fmodf(i, base);
        i = floorf(i/base);
        f = f/base;
    }

    return result;
}

//static void Halton4D(Vec4 s[NUM_SAMPLES], int offset)
static void Halton4D(glm::vec4 s[NUM_SAMPLES], int offset)
{
    for (int i = 0; i < NUM_SAMPLES; i++)
    {
        s[i][0] = Halton(i + offset, 2.0f);
        s[i][1] = Halton(i + offset, 3.0f);
        s[i][2] = Halton(i + offset, 5.0f);
        s[i][3] = Halton(i + offset, 7.0f);
    }
}

glm::mat4 SetLightUniforms(const LightData& l)
{
    // setup light transform
    glm::mat4 identity(1.0f);

    glm::mat4 scale = glm::scale(identity, glm::vec3(0.5f*l.scale, 1.0f));
    glm::mat4 translate = glm::translate(identity, l.position);
    glm::mat4 rotateZ = glm::rotate(identity, glm::radians(l.rotation.z), glm::vec3(0, 0, -1));
    glm::mat4 rotateY = glm::rotate(identity, glm::radians(l.rotation.y), glm::vec3(0, -1, 0));
    glm::mat4 rotateX = glm::rotate(identity, glm::radians(l.rotation.x), glm::vec3(-1, 0, 0));

    glm::mat4 lightTransform = translate*rotateX*rotateY*rotateZ*scale;

    s_gData.s_uniforms.m_quadPoints[0] = lightTransform * glm::vec4(-1,  1, 0, 1);
    s_gData.s_uniforms.m_quadPoints[1] = lightTransform * glm::vec4( 1,  1, 0, 1);
    s_gData.s_uniforms.m_quadPoints[2] = lightTransform * glm::vec4( 1, -1, 0, 1);
    s_gData.s_uniforms.m_quadPoints[3] = lightTransform * glm::vec4(-1, -1, 0, 1);

    // setup light transform
    /*
    bx::mtxIdentity(identity);

    bx::mtxScale(identity, 0.5f*l.scale, 1.0f, 1.0f);
    bx::mtxTranslate(identity, l.position.x, l.position.y, l.position.z);

    glm::mat4 rotateZ = glm::rotate(identity, glm::radians(l.rotation.z), glm::vec3(0, 0, -1));
    glm::mat4 rotateY = glm::rotate(identity, glm::radians(l.rotation.y), glm::vec3(0, -1, 0));
    glm::mat4 rotateX = glm::rotate(identity, glm::radians(l.rotation.x), glm::vec3(-1, 0, 0));

    glm::mat4 lightTransform = translate*rotateX*rotateY*rotateZ*scale;

    bx::vec4MulMtx(s_gData.s_uniforms.m_quadPoints[0], Vec4(-1,  1, 0, 1), lightTransform);
    bx::vec4MulMtx(s_gData.s_uniforms.m_quadPoints[1], Vec4( 1,  1, 0, 1), lightTransform);
    bx::vec4MulMtx(s_gData.s_uniforms.m_quadPoints[2], Vec4( 1, -1, 0, 1), lightTransform);
    bx::vec4MulMtx(s_gData.s_uniforms.m_quadPoints[3], Vec4(-1, -1, 0, 1), lightTransform);
    */

    // set the light state
    s_gData.s_uniforms.m_lightIntensity = l.intensity;
    s_gData.s_uniforms.m_twoSided       = l.twoSided;
    
	s_gData.s_uniforms.submitPerLightUniforms();

	return lightTransform;
}

LightMaps LightColorMaps(uint32_t textureIdx)
{
    // TODO: stick everything into an array?
    switch (textureIdx)
    {
        case 0:  return s_gData.s_texStainedGlassMaps; break;
        case 1:  return s_gData.s_texWhiteMaps;        break;
        default: return s_gData.s_texWhiteMaps;        break;
    }
}

glm::vec2 jitterProjMatrix(float proj[16], int sampleCount, float jitterAASigma, float width, float height)
{
    // Per-frame jitter to camera for AA
    const int frameNum = sampleCount + 1; // Add 1 since otherwise first sample is an outlier

    float u1 = Halton(frameNum, 2.0f);
    float u2 = Halton(frameNum, 3.0f);
    
    // Gaussian sample
    float phi = 2.0f*pi<float>()*u2;
    float r = jitterAASigma*sqrtf(-2.0f*log(std::max(u1, 1e-7f)));
    float x = r*cos(phi);
    float y = r*sin(phi);

    proj[8] += x*2.0f/width;
    proj[9] += y*2.0f/height;

    return glm::vec2(x, y);
}

bgfx::ProgramHandle GetLightProgram(bool diffuse, bool groundTruth)
{
    if (diffuse)
    {
        if (groundTruth)
            return s_programs.m_diffuseGT;
        
        return s_programs.m_diffuseLTC;
    }
    else
    {
        if (groundTruth)
            return s_programs.m_specularGT;
        
        return s_programs.m_specularLTC;
    }
}

class ExampleAreaLights : public entry::AppI
{
public:
	ExampleAreaLights(const char* _name, const char* _description)
		: entry::AppI(_name, _description)
	{
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{

        Args args(_argc, _argv);

        m_viewState.width = _width;
        m_viewState.height = _height;
        m_viewState.debug = BGFX_DEBUG_TEXT;
        m_viewState.reset = BGFX_RESET_VSYNC;

        bgfx::Init init;

        init.type = args.m_type;
        init.vendorId = args.m_pciId;
        init.resolution.width = m_viewState.width;
        init.resolution.height = m_viewState.height;
        init.resolution.reset = m_viewState.reset;
        bgfx::init(init);

        // Enable m_debug text.
        bgfx::setDebug(m_viewState.debug);

        // Imgui.
        imguiCreate();

        // Uniforms.
        s_gData.s_uniforms.init();
        s_gData.s_uLTCMat           = bgfx::createUniform("s_texLTCMat",      bgfx::UniformType::Int1);
        s_gData.s_uLTCAmp           = bgfx::createUniform("s_texLTCAmp",      bgfx::UniformType::Int1);
        s_gData.s_uColorMap         = bgfx::createUniform("s_texColorMap",    bgfx::UniformType::Int1);
        s_gData.s_uFilteredMap      = bgfx::createUniform("s_texFilteredMap", bgfx::UniformType::Int1);

        s_gData.s_uDiffuseUniform   = bgfx::createUniform("s_albedoMap",      bgfx::UniformType::Int1);
        s_gData.s_uNmlUniform       = bgfx::createUniform("s_nmlMap",         bgfx::UniformType::Int1);
        s_gData.s_uRoughnessUniform = bgfx::createUniform("s_roughnessMap",   bgfx::UniformType::Int1);
        s_gData.s_umetallicUniform  = bgfx::createUniform("s_metallicMap",    bgfx::UniformType::Int1);

        // Programs.
        s_programs.init();

        bgfx::VertexDecl posDecl;
        posDecl.begin();
        posDecl.add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float);
        posDecl.end();

        PosColorTexCoord0VertexA::init();

        const uint32_t samplerFlags = BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP;

        // Textures.
        s_gData.s_texLTCMat = loadTexture("textures/ltc_mat.dds", samplerFlags);
        s_gData.s_texLTCAmp = loadTexture("textures/ltc_amp.dds", samplerFlags);

        s_gData.s_texStainedGlassMaps.colorMap    = loadTexture("textures/stained_glass.zlib",          samplerFlags | BGFX_SAMPLER_MIN_ANISOTROPIC);
        s_gData.s_texStainedGlassMaps.filteredMap = loadTexture("textures/stained_glass_filtered.zlib", samplerFlags);
        s_gData.s_texWhiteMaps.colorMap           = loadTexture("textures/white.png", samplerFlags);
        s_gData.s_texWhiteMaps.filteredMap        = loadTexture("textures/white.png", samplerFlags);
        
        // Init of primitive demo

        // Vertex declarations.
        bgfx::VertexDecl PosNormalTexcoordDecl;
        PosNormalTexcoordDecl.begin()
            .add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal,    4, bgfx::AttribType::Uint8, true, true)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        bgfx::VertexDecl PosNormalTangentTexcoordDecl;
        PosNormalTangentTexcoordDecl.begin()
            .add(bgfx::Attrib::Position,  3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Normal,    4, bgfx::AttribType::Uint8, true, true)
            .add(bgfx::Attrib::Tangent,   4, bgfx::AttribType::Uint8, true, true)
            .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
            .end();

        // mesh loading
        s_models[eBunny].loadModel("meshes/bunny.bin");
        s_models[eCube].loadModel("meshes/cube.bin");
        s_models[eHollowCube].loadModel("meshes/hollowcube.bin");
        s_models[eHPlane].loadModel(s_hplaneVertices, BX_COUNTOF(s_hplaneVertices), PosNormalTangentTexcoordDecl, s_planeIndices, BX_COUNTOF(s_planeIndices));

        s_lights[0].loadModel(s_vplaneVertices, BX_COUNTOF(s_vplaneVertices), PosNormalTexcoordDecl, s_planeIndices, BX_COUNTOF(s_planeIndices));

        // Setup instance matrices.   
        using namespace glm;
        const float floorScale = 550.0f;
        s_models[eHPlane].transform = scale(s_models[eHPlane].transform, vec3(floorScale, floorScale, floorScale));

        s_models[eBunny].transform = translate(s_models[eBunny].transform, vec3(15.0f, 5.0f, 0.0f));
        s_models[eBunny].transform = scale(s_models[eBunny].transform, vec3(5.0f, 5.0f, 5.0f));

        s_models[eHollowCube].transform = translate(s_models[eHollowCube].transform, vec3(0.0f, 10.0f, 0.0f));
        s_models[eHollowCube].transform = scale(s_models[eHollowCube].transform, vec3(2.5f, 2.5f, 2.5f));

        s_models[eCube].transform = translate(s_models[eCube].transform, vec3(-15.0f, 5.0f, 0.0f));
        s_models[eCube].transform = scale(s_models[eCube].transform, vec3(2.5f, 2.5f, 2.5f));

        // setup the lights
        s_lightData[0].rotation.z = 0.0f;
        s_lightData[0].scale = glm::vec2(64.0f);
        s_lightData[0].intensity = 4.0f;
        s_lightData[0].position = glm::vec3(0.0f, 0.0f, 40.0f);
        s_lightData[0].textureIdx = 2;

        // Setup instance matrices.
        /*
        const float floorScale = 550.0f;
        bx::mtxScale(s_models[eHPlane].transform, floorScale, floorScale, floorScale);

        bx::mtxTranslate(s_models[eBunny].transform, 15.0f, 5.0f, 0.0f);
        bx::mtxScale(s_models[eBunny].transform, 5.0f, 5.0f, 5.0f);

        bx::mtxTranslate(s_models[eHollowCube].transform, 0.0f, 10.0f, 0.0f);
        bx::mtxScale(s_models[eHollowCube].transform, 2.5f, 2.5f, 2.5f);

        bx::mtxTranslate(s_models[eCube].transform, -15.0f, 5.0f, 0.0f);
        bx::mtxScale(s_models[eCube].transform, 2.5f, 2.5f, 2.5f);

        // setup the lights
        s_lightData[0].rotation.z = 0.0f;
        s_lightData[0].scale = make_vec2(64.0f, 64.0f);
        s_lightData[0].intensity = 4.0f;
        s_lightData[0].position = glm::vec3(0.0f, 0.0f, 40.0f);
        s_lightData[0].textureIdx = 2;
        */

       // End init primitive demo.

        m_sceneSettings.diffColor[0]    =  1.0f;
        m_sceneSettings.diffColor[1]    =  1.0f;
        m_sceneSettings.diffColor[2]    =  1.0f;
        m_sceneSettings.roughness       =  1.0f;
        m_sceneSettings.reflectance     = 0.04f;
        m_sceneSettings.currLightIdx    =     0;
        m_sceneSettings.jitterAASigma   =  0.3f;
        m_sceneSettings.sampleCount     =     0;
        m_sceneSettings.demoIdx         =     1;
        m_sceneSettings.useGT           = false;
        m_sceneSettings.showDiffColor   = false;
        
        m_prevDiffColor[3] = { 0 };

        m_rtColorBuffer.idx = bgfx::kInvalidHandle;

        // Setup camera.
        //float initialPrimPos[3] = { 0.0f, 60.0f, -60.0f };
        //float initialPrimVertAngle = -0.45f;
        //float initialPrimHAngle = 0.0f;
        cameraCreate();

        // Set view and projection matrices.
        const float camFovy    = 60.0f;
        const float camAspect  = float(int32_t(m_viewState.width)) / float(int32_t(m_viewState.height));
        const float camNear    = 0.1f;
        const float camFar     = 2000.0f;
        bx::mtxProj(m_viewState.proj, camFovy, camAspect, camNear, camFar, m_caps->homogeneousDepth);
        cameraGetViewMtx(m_viewState.view);

    }

	virtual int shutdown() override
	{
        // shutdown
        for (int i = 0; i < eModelCount; ++i)
            s_models[i].unload();

        bgfx::destroy(s_gData.s_texLTCMat);
        bgfx::destroy(s_gData.s_texLTCAmp);

        s_gData.s_texStainedGlassMaps.destroyTextures();
        s_gData.s_texWhiteMaps.destroyTextures();

        bgfx::destroy(m_rtColorBuffer);

        s_programs.destroy();

        bgfx::destroy(s_gData.s_uLTCMat);
        bgfx::destroy(s_gData.s_uLTCAmp);
        bgfx::destroy(s_gData.s_uColorMap);
        bgfx::destroy(s_gData.s_uFilteredMap);

        s_gData.s_uniforms.destroy();

        cameraDestroy();
        imguiDestroy();

        // Shutdown bgfx.
        bgfx::shutdown();

        return 0;
	}

	bool update() override
	{
        if (!entry::processEvents(m_viewState.width, m_viewState.height, m_viewState.debug, m_viewState.reset, &m_mouseState))
        {
            RenderList rlist;
            rlist.models = &s_models[0];
            rlist.count = eModelCount;

            RenderList llist;
            llist.models = &s_lights[0];
            llist.count = eLightCount;

            LightData* lsettings = &s_lightData[0];

            bool rtRecreated = false;

            if (m_viewState.oldWidth  != m_viewState.width
            ||  m_viewState.oldHeight != m_viewState.height
            ||  m_viewState.oldReset  != m_viewState.reset)
            {
                m_viewState.oldWidth  = m_viewState.width;
                m_viewState.oldHeight = m_viewState.height;
                m_viewState.oldReset  = m_viewState.reset;

                if (bgfx::isValid(m_rtColorBuffer))
                {
                    bgfx::destroy(m_rtColorBuffer);
                }

                int w = m_viewState.width;
                int h = m_viewState.height;

                // Note: bgfx will cap the quality to the maximum supported
                uint64_t rtFlags = BGFX_TEXTURE_RT;

                bgfx::TextureHandle colorTextures[] =
                {
                    bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::RGBA32F, rtFlags),
                    bgfx::createTexture2D(w, h, false, 1, bgfx::TextureFormat::D24S8,   rtFlags)
                };
                m_rtColorBuffer = bgfx::createFrameBuffer(BX_COUNTOF(colorTextures), colorTextures, true);
                rtRecreated = true;
            }
            /*
            {
                // Imgui.
                imguiBeginFrame(m_mouseState.m_mx
                    , m_mouseState.m_my
                    , (m_mouseState.m_buttons[entry::MouseButton::Left]   ? IMGUI_MBUT_LEFT   : 0)
                    | (m_mouseState.m_buttons[entry::MouseButton::Right]  ? IMGUI_MBUT_RIGHT  : 0)
                    | (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0)
                    , m_mouseState.m_mz
                    , m_width
                    , m_height
                    );

    #define IMGUI_FLOAT_SLIDER(_name, _val) \
                    ImGui::Slider(_name \
                            , _val \
                            , *( ((float*)&_val)+1) \
                            , *( ((float*)&_val)+2) \
                            , *( ((float*)&_val)+3) \
                            )

                static int32_t leftScrollArea = 0;
                static bool editSceneParams = false;
                const uint32_t debugWindHeight = 700;
                ImGui::BeginScrollArea("", 10, 70, 256, editSceneParams ? debugWindHeight : 32, &leftScrollArea);

                ImGui::Bool("Scene Settings", editSceneParams);

                if (editSceneParams)
                {
                    ImGui::ColorWheel("Diffuse Color:", &settings.m_diffColor[0], settings.m_showDiffColor);

                    // -- disable material editing in sponza
                    if(settings.m_demoIdx != 1) 
                    {
                        uiChanged |= ImGui::Slider("Reflectance:", settings.m_reflectance, 0.01f, 1.0f, 0.001f);

                        uiChanged |= ImGui::Slider("Roughness:", settings.m_roughness, float(MIN_ROUGHNESS), 1.0f, 0.001f);
                    }

                    ImGui::SeparatorLine();

                    uiChanged |= ImGui::Bool("Ground Truth", settings.m_useGT);

                    ImGui::Separator();
                    ImGui::Label("Demo Scene");
                    {
                        ImGui::Indent(16);
                        cameraSetPosition(initialPrimPos);
                        cameraSetHorizontalAngle(initialPrimHAngle);
                        cameraSetVerticalAngle(initialPrimVertAngle);
                        settings.m_diffColor[0] = 0.5f;
                        settings.m_diffColor[1] = 0.5f;
                        settings.m_diffColor[2] = 0.5f;
                        settings.m_roughness = 0.15f;

                        settings.m_demoIdx = newIdx;
                        ImGui::Unindent(16);
                    }

    #undef IMGUI_FLOAT_SLIDER
                }

                ImGui::EndScrollArea();

                // light debug UI
                {
                    static int32_t rightScrollArea = 0;
                    static bool editLights = false;

                    uint32_t startY = (editSceneParams ? debugWindHeight : 32) + 70;
                    ImGui::BeginScrollArea("", 10, startY, 256, editLights ? debugWindHeight : 32, &rightScrollArea);
                    ImGui::Bool("Light Parameters", editLights);
                    if (editLights)
                    {
                        if (llist.count > 1)
                        {
                            float idx = (float)settings.m_currLightIdx;
                            uiChanged |= ImGui::Slider("Index", idx, 0.0f, (float)llist.count - 1, 1.0f);
                            settings.m_currLightIdx = (uint32_t)idx;
                            ImGui::SeparatorLine();
                        }

                        uint32_t lidx = settings.m_currLightIdx;

                        uiChanged |= ImGui::Slider("Intensity", lsettings[lidx].intensity, 0.0f, 30.0f, 0.01f);

                        uiChanged |= ImGui::Slider("Width",  lsettings[lidx].scale.x, 1.0f, 100.0f, 1.0f);
                        uiChanged |= ImGui::Slider("Height", lsettings[lidx].scale.y, 1.0f, 100.0f, 1.0f);

                        uiChanged |= ImGui::Bool("Two Sided", lsettings[lidx].twoSided);

                        ImGui::SeparatorLine();

                        uiChanged |= ImGui::Slider("Position (X)", lsettings[lidx].position.x, -150.0f, 150.0f, 0.1f);
                        uiChanged |= ImGui::Slider("Position (Y)", lsettings[lidx].position.y, -150.0f, 150.0f, 0.1f);
                        uiChanged |= ImGui::Slider("Position (Z)", lsettings[lidx].position.z, -150.0f, 150.0f, 0.1f);

                        ImGui::SeparatorLine();

                        uiChanged |= ImGui::Slider("Rotation (X)", lsettings[lidx].rotation.x, -180, 179.0f, 1.0f);
                        uiChanged |= ImGui::Slider("Rotation (Y)", lsettings[lidx].rotation.y, -180, 179.0f, 1.0f);
                        uiChanged |= ImGui::Slider("Rotation (Z)", lsettings[lidx].rotation.z, -180, 179.0f, 1.0f);

                        ImGui::Separator();
                        
                        ImGui::Label("Texture");
                        {
                            ImGui::Indent(16);
                            uint32_t newIdx = ImGui::Choose(lsettings[lidx].textureIdx, "Stained Glass", "White");
                            if (newIdx != lsettings[lidx].textureIdx)
                                uiChanged = true;
                            lsettings[lidx].textureIdx = newIdx;
                            ImGui::Unindent(16);
                        }
                    }
                    ImGui::EndScrollArea();
                }

                imguiEndFrame();
            }
            */

            // Time.
            int64_t now = bx::getHPCounter();
            static int64_t last = now;
            const int64_t frameTime = now - last;
            last = now;
            const double freq = double(bx::getHPFrequency());
            const double toMs = 1000.0/freq;
            const float deltaTime = float(frameTime/freq);

            // Use debug font to print information about this example.
            bgfx::dbgTextClear();
            bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/xx-arealights");
            bgfx::dbgTextPrintf(0, 2, 0x6f, "Description: Area lights example.");
            bgfx::dbgTextPrintf(0, 3, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

            // Update camera.
            cameraUpdate(deltaTime, m_mouseState);
            
            // Update view mtx.
            cameraGetViewMtx(m_viewState.view);

            if (memcmp(m_viewState.oldView, m_viewState.view, sizeof(m_viewState.view))
            ||  memcmp(m_prevDiffColor, m_sceneSettings.diffColor, sizeof(m_sceneSettings.diffColor))
            ||  rtRecreated)
            {
                memcpy(m_viewState.oldView, m_viewState.view, sizeof(m_viewState.view));
                memcpy(m_prevDiffColor, m_sceneSettings.diffColor, sizeof(m_sceneSettings.diffColor));
                m_sceneSettings.sampleCount = 0;
            }

            // Lights.
            Light pointLight =
            {
                { { 0.0f, 0.0f, 0.0f, 1.0f } }, //position
            };

            pointLight.m_position.m_x = 0.0f;
            pointLight.m_position.m_y = 0.0f;
            pointLight.m_position.m_z = 0.0f;

            // Update uniforms.
            s_gData.s_uniforms.m_roughness   = m_sceneSettings.roughness;
            s_gData.s_uniforms.m_reflectance = m_sceneSettings.reflectance;
            s_gData.s_uniforms.m_lightPtr    = &pointLight;

            // Setup uniforms.
            
            s_gData.s_uniforms.m_albedo = glm::vec4(
                powf(m_sceneSettings.diffColor[0], 2.2f),
                powf(m_sceneSettings.diffColor[1], 2.2f),
                powf(m_sceneSettings.diffColor[2], 2.2f),
                1.0f);
            
            // Clear color (maps to 1 after tonemapping)
            s_gData.s_uniforms.m_color = vec4(25.7f, 25.7f, 25.7f, 1.0f);

            float lightMtx[16];
            s_gData.s_uniforms.setPtrs(&pointLight, lightMtx);

            // Grab camera position
            bx::Vec3 viewPos;
            viewPos = cameraGetPosition();
            
            // Compute transform matrices.
            float screenProj[16];
            float screenView[16];
            bx::mtxIdentity(screenView);
            bx::mtxOrtho(screenProj, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, m_caps->homogeneousDepth);

            float proj[16];
            memcpy(proj, m_viewState.proj, sizeof(proj));
            bgfx::setViewTransform(0, m_viewState.view, proj);
    
            // Setup views and render targets.
            bgfx::setViewRect(0, 0, 0, m_viewState.width, m_viewState.height);

            // Clear backbuffer at beginning.
            bgfx::setViewClear(0
                            , BGFX_CLEAR_COLOR
                            | BGFX_CLEAR_DEPTH
                            , m_clearValues.clearRgba
                            , m_clearValues.clearDepth
                            , m_clearValues.clearStencil
                            );
            bgfx::touch(0);

            uint8_t passViewID = RENDERVIEW_DRAWSCENE_0_ID;

            {
                Halton4D(s_gData.s_uniforms.m_samples, int(m_sceneSettings.sampleCount));
                s_gData.s_uniforms.m_sampleCount = (float)m_sceneSettings.sampleCount;
                s_gData.s_uniforms.submitPerFrameUniforms(viewPos);
                
                // Set the jittered projection matrix
                memcpy(proj, m_viewState.proj, sizeof(proj));
                jitterProjMatrix(proj, m_sceneSettings.sampleCount/NUM_SAMPLES,
                                m_sceneSettings.jitterAASigma,
                                (float)m_viewState.width, (float)m_viewState.height);

                bgfx::setViewTransform(passViewID, m_viewState.view, proj);
                
                bgfx::setViewFrameBuffer(passViewID, m_rtColorBuffer);
                bgfx::setViewRect(passViewID, 0, 0, m_viewState.width, m_viewState.height);
                
                uint16_t flagsRT = BGFX_CLEAR_DEPTH;
                if (m_sceneSettings.sampleCount == 0)
                {
                    flagsRT |= BGFX_CLEAR_COLOR;
                }
                
                // Clear offscreen RT
                bgfx::setViewClear(passViewID
                                , flagsRT
                                , m_clearValues.clearRgba
                                , m_clearValues.clearDepth
                                , m_clearValues.clearStencil
                                );
                bgfx::touch(passViewID);
                
                // Render Depth
                for (uint64_t renderIdx = 0; renderIdx < rlist.count; ++renderIdx)
                {
                    rlist.models[renderIdx].submit(s_gData, passViewID,
                                                s_programs.m_packDepth, LightColorMaps(0), s_renderStates[RenderState::ZPass]);
                }
                
                for (uint64_t renderIdx = 0; renderIdx < llist.count; ++renderIdx)
                {
                    glm::mat4 lightTransform = SetLightUniforms(lsettings[renderIdx]);
                    llist.models[renderIdx].transform = lightTransform;
                    LightMaps colorMaps = LightColorMaps(lsettings[renderIdx].textureIdx);
            
                    llist.models[renderIdx].submit(s_gData, passViewID,
                                                s_programs.m_packDepthLight, colorMaps, s_renderStates[RenderState::ZTwoSidedPass]);
                }
                
                // Render passes
                for (uint32_t renderPassIdx = 0; renderPassIdx < 2; renderPassIdx++)
                {
                    // Render scene once for each light
                    for (uint32_t lightPassIdx = 0; lightPassIdx < llist.count; ++lightPassIdx)
                    {
                        const LightData& light = lsettings[lightPassIdx];
                        
                        // Setup light
                        SetLightUniforms(light);
                        LightMaps colorMaps = LightColorMaps(light.textureIdx);
            
                        bool diffuse = (renderPassIdx == 0);
                        bgfx::ProgramHandle progDraw = GetLightProgram(diffuse, m_sceneSettings.useGT);
                        
                        // Render scene models
                        for (uint64_t renderIdx = 0; renderIdx < rlist.count; ++renderIdx)
                        {
                            rlist.models[renderIdx].submit(s_gData, passViewID,
                                                        progDraw, colorMaps, s_renderStates[RenderState::ColorPass]);
                        }
                    }
                }
                
                // render light objects
                for (uint64_t renderIdx = 0; renderIdx < llist.count; ++renderIdx)
                {
                    // setup light
                    glm::mat4 lightTransform = SetLightUniforms(lsettings[renderIdx]);
                    LightMaps colorMaps = LightColorMaps(lsettings[renderIdx].textureIdx);
                    
                    llist.models[renderIdx].transform = lightTransform;
                    llist.models[renderIdx].submit(s_gData, passViewID,
                                                s_programs.m_colorTextured, colorMaps,
                                                s_renderStates[RenderState::ColorAlphaPass]);
                }
                
                m_sceneSettings.sampleCount += NUM_SAMPLES;
                ++passViewID;
            }

            // Blit pass
            uint8_t blitID = passViewID;
            bgfx::FrameBufferHandle handle = BGFX_INVALID_HANDLE;
            bgfx::setViewFrameBuffer(blitID, handle);
            bgfx::setViewRect(blitID, 0, 0, m_viewState.width, m_viewState.height);
            bgfx::setViewTransform(blitID, screenView, screenProj);
            
            bgfx::setTexture(2, s_gData.s_uColorMap, bgfx::getTexture(m_rtColorBuffer));
            bgfx::setState(BGFX_STATE_WRITE_RGB|BGFX_STATE_WRITE_A);
            screenSpaceQuad(m_caps->originBottomLeft);
            bgfx::submit(blitID, s_programs.m_blit);

            // Advance to next frame. Rendering thread will be kicked to
            // process submitted rendering primitives.
            bgfx::frame();
        }
        return true;
	}

    bgfx::FrameBufferHandle m_rtColorBuffer;

    //std::map<stl::string, bgfx::TextureHandle> m_textureCache;

    struct SceneSettings
    {
        float    diffColor[3];
        float    roughness;
        float    reflectance;
        float    jitterAASigma;
        uint32_t sampleCount;
        uint32_t demoIdx;
        uint32_t currLightIdx;
        bool     useGT;
        bool     showDiffColor;
    };
    SceneSettings m_sceneSettings;

    struct ViewState
    {
        ViewState(uint32_t _width = 1280, uint32_t _height = 720)
            : width(_width)
            , height(_height)
            , oldWidth(0)
            , oldHeight(0)
        {
            memset(oldView, 0, 16*sizeof(float));
        }

        uint32_t width;
        uint32_t height;
        uint32_t reset;
        uint32_t debug;

        uint32_t oldWidth;
        uint32_t oldHeight;
        uint32_t oldReset;

        float view[16];
        float proj[16];

        float oldView[16];
    };
    ViewState m_viewState;

    struct ClearValues
    {
        ClearValues(uint32_t _clearRgba  = 0x30303000
            , float    _clearDepth       = 1.0f
            , uint8_t  _clearStencil     = 0
            )
            : clearRgba(_clearRgba)
            , clearDepth(_clearDepth)
            , clearStencil(_clearStencil)
        {
        }

        uint32_t clearRgba;
        float    clearDepth;
        uint8_t  clearStencil;
    };
    ClearValues m_clearValues;

	int32_t m_scrollArea;

	entry::MouseState m_mouseState;

	const bgfx::Caps* m_caps;
	int64_t m_timeOffset;

    float m_prevDiffColor[3];

};

ENTRY_IMPLEMENT_MAIN(ExampleAreaLights, "41-arealights", "LTC Area Lights.");
