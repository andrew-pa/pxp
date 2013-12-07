#define LINK_DIRECTX
#include <helper.h>
#include <dx_app.h>
#include <shader.h>
#include <constant_buffer.h>
#include <texture2d.h>
#include <render_texture.h>
#include <states.h>

class pxp_app : public dx_app
{
public:
	pxp_app() : dx_app(4, true)
	{
	}

	void load() override
	{
	}

	void update(float t, float dt) override
	{
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