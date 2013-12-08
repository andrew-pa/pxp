#define LINK_DIRECTX
#include <helper.h>
#include <dx_app.h>
#include <shader.h>
#include <constant_buffer.h>
#include <texture2d.h>
#include <render_texture.h>
#include <states.h>
#include <mesh.h>
#include <camera.h>
#include <input.h>

mesh* create_ndc_quad(ComPtr<ID3D11Device> device)
{
	dvertex v[] = 
	{
		dvertex(-1, -1, 0, 0, 0, -1, 1, 0, 0, 0, 1),
		dvertex(-1,  1, 0, 0, 0, -1, 0, 1, 0, 0, 0),
		dvertex( 1,  1, 0, 0, 0, -1, 1, 0, 0, 1, 0),
		dvertex( 1, -1, 0, 0, 0, -1, 1, 0, 0, 1, 1),
	};
	uint i[] =
	{
		0, 1, 2, 0, 2, 3,
	};
	return new mesh(device, v, i, 6, 4, sizeof(dvertex), "NDC_QUAD");
}
static const D3D11_INPUT_ELEMENT_DESC posnormtex_layout[] =
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
};
class pxp_app : public dx_app
{
	mesh* q;
	shader s;
	struct b
	{
		float t;
		float3 cam_o;
		float3 ww; float wwww;
		float3 vv; float vvvv;
		float3 uu; float uuuu;
	};
	constant_buffer<b> buf;
	float3 camera_pos;
	float3 camera_targ;
	camera cam;

public:
	pxp_app() : dx_app(4, true),
		camera_pos(), camera_targ(5, 0, 0),
		cam(float3(0,0,0), float3(1, 0, 0), 0.1f, 1000.f, to_radians(45.f))
	{
	}

	void load() override
	{
		q = create_ndc_quad(device);
		s = shader(device, read_data_from_package(L"vs.cso"), read_data_from_package(L"ps.cso"),
			posnormtex_layout, _countof(posnormtex_layout));
		buf = constant_buffer<b>(device, 0, { 0 });
	}

	void update(float t, float dt) override
	{
		if (windowSizeChanged)
		{
			windowSizeChanged = false;
		}
		buf.data().t = t;
		//camera_pos = 2.1f * float3(2.5f*sinf(0.25f*t), 1.f + 1.f*cosf(t*.13f), 2.5f*cosf(0.25f*t));
		
		const float spd = .75f;
		if (keyboard::key_down('A'))
			cam.strafe(-spd*dt);
		else if (keyboard::key_down('D'))
			cam.strafe(spd*dt);
		if (keyboard::key_down('W'))
			cam.forward(spd*dt);
		else if (keyboard::key_down('S'))
			cam.forward(-spd*dt);
		if (keyboard::key_down('Q'))
			cam.position().y += spd*dt;
		else if (keyboard::key_down('E'))
			cam.position().y -= spd*dt;

		static const float d = XMConvertToRadians(1.f);
		if (keyboard::key_down(VK_LEFT))
			cam.rotate_worldY(-d);
		else if (keyboard::key_down(VK_RIGHT))
			cam.rotate_worldY(d);
		if (keyboard::key_down(VK_UP))
			cam.pitch(-d);
		else if (keyboard::key_down(VK_DOWN))
			cam.pitch(d);
		
		buf.data().cam_o = cam.position();
		auto ww = buf.data().ww = cam.look(); //normalize(camera_targ - cam.position());
		auto uu = buf.data().uu = normalize(cross(float3(0, 1, 0), ww));
		buf.data().vv = normalize(cross(ww, uu));
		buf.update(context);
	}

	void render(float t, float dt) override
	{
		dx_app::render(t, dt);
		s.bind(context);
		buf.bind(context, ShaderStage::Pixel);
		q->draw(context);
		buf.unbind(context, ShaderStage::Pixel);
		s.unbind(context);
	}
};

int CALLBACK WinMain(
	_In_  HINSTANCE inst,
	_In_  HINSTANCE pinst,
	_In_  LPSTR cmdLine,
	_In_  int cmds
	)
{
	pxp_app app;
	return app.run(inst, cmds, L"P X P", 640, 480);
}