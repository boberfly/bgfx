/*
 * Copyright 2011-2015 Branimir Karadzic. All rights reserved.
 * License: http://www.opensource.org/licenses/BSD-2-Clause
 */

#include <stdio.h>

#include "common.h"
#include "bgfx_utils.h"

struct PosColorVertex
{
	float m_x;
	float m_y;
	float m_z;
	uint32_t m_abgr;

	static void init()
	{
		ms_decl
			.begin()
			.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
			.add(bgfx::Attrib::Color0,   4, bgfx::AttribType::Uint8, true)
			.end();
	};

	static bgfx::VertexDecl ms_decl;
};

bgfx::VertexDecl PosColorVertex::ms_decl;

//static PosColorVertex s_cubeVertices[8] =
//{
//	{-1.0f,  1.0f,  1.0f, 0xff000000 },
//	{ 1.0f,  1.0f,  1.0f, 0xff0000ff },
//	{-1.0f, -1.0f,  1.0f, 0xff00ff00 },
//	{ 1.0f, -1.0f,  1.0f, 0xff00ffff },
//	{-1.0f,  1.0f, -1.0f, 0xffff0000 },
//	{ 1.0f,  1.0f, -1.0f, 0xffff00ff },
//	{-1.0f, -1.0f, -1.0f, 0xffffff00 },
//	{ 1.0f, -1.0f, -1.0f, 0xffffffff },
//};

static PosColorVertex s_icoVertices[] =
{
	{ 0.000f, 0.000f, 1.000f, 0xff000000 },
	{ 0.894f, 0.000f, 0.447f, 0xff0000ff },
	{ 0.276f, 0.851f, 0.447f, 0xff00ff00 },
	{ -0.724f, 0.526f, 0.447f, 0xff00ffff },
	{ -0.724f, -0.526f, 0.447f, 0xffff0000 },
	{ 0.276f, -0.851f, 0.447f, 0xffff00ff },
	{ 0.724f, 0.526f, -0.447f, 0xffffff00 },
	{ -0.276f, 0.851f, -0.447f, 0xffffffff },
	{ -0.894f, 0.000f, -0.447f, 0xff000000 },
	{ -0.276f, -0.851f, -0.447f, 0xff0000ff },
	{ 0.724f, -0.526f, -0.447f, 0xff00ff00 },
	{ 0.000f, 0.000f, -1.000f, 0xff00ffff },
};



//static const uint16_t s_cubeIndices[36] =
//{
//	0, 1, 2, // 0
//	1, 3, 2,
//	4, 6, 5, // 2
//	5, 6, 7,
//	0, 2, 4, // 4
//	4, 2, 6,
//	1, 5, 3, // 6
//	5, 7, 3,
//	0, 4, 1, // 8
//	4, 5, 1,
//	2, 3, 6, // 10
//	6, 3, 7,
//};

static const uint16_t s_icoIndices[] = {
	2, 1, 0,
	3, 2, 0,
	4, 3, 0,
	5, 4, 0,
	1, 5, 0,

	11, 6,  7,
	11, 7,  8,
	11, 8,  9,
	11, 9,  10,
	11, 10, 6,

	1, 2, 6,
	2, 3, 7,
	3, 4, 8,
	4, 5, 9,
	5, 1, 10,

	2,  7, 6,
	3,  8, 7,
	4,  9, 8,
	5, 10, 9,
	1, 6, 10 };

class Tessellation : public entry::AppI
{
public:
	Tessellation(const char* _name, const char* _description)
		: entry::AppI(_name, _description)
	{
	}

	void init(int32_t _argc, const char* const* _argv, uint32_t _width, uint32_t _height) override
	{
		Args args(_argc, _argv);
		
		m_width  = _width;
		m_height = _height;
		m_debug  = BGFX_DEBUG_TEXT;
		m_reset  = BGFX_RESET_VSYNC;

		bgfx::init(args.m_type, args.m_pciId);
		bgfx::reset(m_width, m_height, m_reset);

		// Enable debug text.
		bgfx::setDebug(m_debug);

		// Set view 0 clear state.
		bgfx::setViewClear(0
				, BGFX_CLEAR_COLOR|BGFX_CLEAR_DEPTH
				, 0x303030ff
				, 1.0f
				, 0
				);

		const bgfx::Caps* caps = bgfx::getCaps();
		tesselationSupported = !!(caps->supported & BGFX_CAPS_TESSELLATION);

		printf("tesselation = %i\n", tesselationSupported);

		if (tesselationSupported)
		{

			// Create vertex stream declaration.
			PosColorVertex::init();

			// Create static vertex buffer.
			m_vbh = bgfx::createVertexBuffer(
				// Static data can be passed with bgfx::makeRef
				bgfx::makeRef(s_icoVertices, sizeof(s_icoVertices))
				, PosColorVertex::ms_decl
				);

			// Create static index buffer.
			m_ibh = bgfx::createIndexBuffer(
				// Static data can be passed with bgfx::makeRef
				bgfx::makeRef(s_icoIndices, sizeof(s_icoIndices))
				);


			u_TessLevel = bgfx::createUniform("TessLevel", bgfx::UniformType::Vec4);

			// Create program from shaders.
			m_program = loadProgram("vs_tessellation", "fs_tessellation", "hs_tessellation", "ds_tessellation");
		}

		m_timeOffset = bx::getHPCounter();
	}

	virtual int shutdown() override
	{
		// Cleanup.
		if (tesselationSupported)
		{
			bgfx::destroy(m_ibh);
			bgfx::destroy(m_vbh);
			bgfx::destroy(m_program);
		}

		// Shutdown bgfx.
		bgfx::shutdown();

		return 0;
	}

	bool update() override
	{
		if (!entry::processEvents(m_width, m_height, m_debug, m_reset) )
		{
			int64_t now = bx::getHPCounter();
			static int64_t last = now;
			const int64_t frameTime = now - last;
			last = now;
			const double freq = double(bx::getHPFrequency() );
			const double toMs = 1000.0/freq;

			float time = (float)( (now-m_timeOffset)/double(bx::getHPFrequency() ) );

			// Use debug font to print information about this example.
			bgfx::dbgTextClear();
			bgfx::dbgTextPrintf(0, 1, 0x4f, "bgfx/examples/27-tessellation");
			bgfx::dbgTextPrintf(0, 2, 0x6f, "Description: Rendering simple tessellated meshes with varying tessellation levels.");
			bgfx::dbgTextPrintf(0, 3, 0x0f, "Frame: % 7.3f[ms]", double(frameTime)*toMs);

			if (tesselationSupported)
			{
				float at[3] = { 0.0f, 0.0f,   0.0f };
				float eye[3] = { 0.0f, 0.0f, -12.0f };

				// Set view and projection matrix for view 0.
				const bgfx::HMD* hmd = bgfx::getHMD();
				if (NULL != hmd && 0 != (hmd->flags & BGFX_HMD_RENDERING))
				{
					float view[16];
					bx::mtxQuatTranslationHMD(view, hmd->eye[0].rotation, eye);

					float proj[16];
					bx::mtxProj(proj, hmd->eye[0].fov, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);

					bgfx::setViewTransform(0, view, proj);

					// Set view 0 default viewport.
					//
					// Use HMD's width/height since HMD's internal frame buffer size
					// might be much larger than window size.
					bgfx::setViewRect(0, 0, 0, hmd->width, hmd->height);
				}
				else
				{
					float view[16];
					bx::mtxLookAt(view, eye, at);

					float proj[16];
					bx::mtxProj(proj, 60.0f, float(m_width) / float(m_height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
					bgfx::setViewTransform(0, view, proj);

					// Set view 0 default viewport.
					bgfx::setViewRect(0, 0, 0, (uint16_t)m_width, (uint16_t)m_height);
				}

				// This dummy draw call is here to make sure that view 0 is cleared
				// if no other draw calls are submitted to view 0.
				bgfx::touch(0);

				// Submit 11x11 cubes.
				for (uint32_t yy = 0; yy < 4; ++yy)
				{
					for (uint32_t xx = 0; xx < 4; ++xx)
					{
						float mtx[16];
						bx::mtxRotateXY(mtx, time + xx*0.21f, time + yy*0.37f);
						mtx[12] = -5.0f + float(xx)*3.0f;
						mtx[13] = -7.0f + float(4 - yy)*3.0f;
						mtx[14] = 0.0f;

						// Set model matrix for rendering.
						bgfx::setTransform(mtx);

						// Set vertex and index buffer.
						bgfx::setVertexBuffer(0, m_vbh);
						bgfx::setIndexBuffer(m_ibh);

						// Set render states.
						bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_PATCH3);

						float tesselation[4] = { xx + 1.0f, yy + 1.0f, 1.0f, 1.0f };
						bgfx::setUniform(u_TessLevel, tesselation);

						// Submit primitive for rendering to view 0.
						bgfx::submit(0, m_program);
					}
				}
			}
			else 
			{
				bool blink = uint32_t(time*3.0f) & 1;
				bgfx::dbgTextPrintf(0, 5, blink ? 0x1f : 0x01, " Tesselation is not supported by GPU. ");
			}

			// Advance to next frame. Rendering thread will be kicked to
			// process submitted rendering primitives.
			bgfx::frame();

			return true;
		}

		return false;
	}

	bgfx::UniformHandle u_TessLevel;
	bool tesselationSupported;
	uint32_t m_width;
	uint32_t m_height;
	uint32_t m_debug;
	uint32_t m_reset;
	bgfx::VertexBufferHandle m_vbh;
	bgfx::IndexBufferHandle m_ibh;
	bgfx::ProgramHandle m_program;
	int64_t m_timeOffset;
};

ENTRY_IMPLEMENT_MAIN(Tessellation, "38-tessellation", "Tessellation.");
