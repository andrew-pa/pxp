#define LINK_DIRECTX
#include <helper.h>
#include <dx_app.h>
#include <shader.h>
#include <constant_buffer.h>
#include <texture2d.h>
#include <render_texture.h>
#include <states.h>
#include <mesh.h>

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
public:
	pxp_app() : dx_app(4, true)
	{
	}

	void load() override
	{
		q = create_ndc_quad(device);
		s = shader(device, read_data_from_package(L"vs.cso"), read_data_from_package(L"ps.cso"),
			posnormtex_layout, _countof(posnormtex_layout));
	}

	void update(float t, float dt) override
	{
		if (windowSizeChanged)
		{
			windowSizeChanged = false;
		}
	}

	void render(float t, float dt) override
	{
		dx_app::render(t, dt);
		s.bind(context);
		q->draw(context);
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