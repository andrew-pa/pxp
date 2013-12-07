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

class pxp_app : public dx_app
{
	mesh* q;
public:
	pxp_app() : dx_app(4, true)
	{
	}

	void load() override
	{
		q = create_ndc_quad(device);
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