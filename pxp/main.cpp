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
#include <data_buffer.h>

using namespace aldx;

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
#undef min
#undef max
struct aabb
{
	float3 min;
	float blah;
	float3 max;

	inline aabb& add_pnt(float3 p)
	{
		if (p.x > max.x) max.x = p.x;
		if (p.y > max.y) max.y = p.y;
		if (p.z > max.z) max.z = p.z;

		if (p.x < min.x) min.x = p.x;
		if (p.y < min.y) min.y = p.y;
		if (p.z < min.z) min.z = p.z;
		return *this;
	}

	aabb(float3 m, float3 x)
		: min(m), max(x) { }
	aabb()
		: min(0, 0, 0), max(0, 0, 0) { }
	aabb(aabb a, aabb b)
		: min(a.min), max(a.max)
	{
		add_pnt(b.max);
		add_pnt(b.min);
	}
};

struct bvh_node
{
	aabb bounds;
	uint left_ptr;
	uint right_ptr;
	uint is_left_leaf;
	uint is_right_leaf;
	bvh_node(aabb b, uint l, uint r, uint x, bool leafL, bool leafR)
		: bounds(b), left_ptr(l), right_ptr(r), is_left_leaf(leafL ? 1 : 0), is_right_leaf(leafR ? 1 : 0) {}
};

struct tri
{
	uint a, b, c;
	uint mesh_id;
	tri(uint aa, uint bb, uint cc, uint mmesh_id)
		: a(aa), b(bb), c(cc), mesh_id(mmesh_id)
	{}
};
bool operator ==(const tri& a, const tri& b)
{
	return (a.a == b.a) && (a.b == b.b) && (a.c == b.c) && (a.mesh_id == b.mesh_id);
}

struct vertex
{
	float3 pos;
	float3 norm;
	float2 tex;
	vertex() { }
	vertex(float3 p, float3 n, float2 t)
		: pos(p), norm(n), tex(t) {}
};
bool operator ==(const vertex& a, const vertex& b)
{
	return a.pos.x == b.pos.x && a.pos.y == b.pos.y && a.pos.z == b.pos.z &&
		a.norm.x == b.norm.x && a.norm.y == b.norm.y && a.norm.z == b.norm.z &&
		a.tex.x == b.tex.x && a.tex.y == b.tex.y;

}

struct smesh
{
	float4x4 inv_world;
	float4 color;
	smesh(float4x4 iw, float4 c)
		: inv_world(iw), color(c) { }
};

inline float3 readvec3(istream& i)
{
	float x, y, z;
	i >> x >> y >> z;
	return float3(x, y, z);
}


void build_mesh_bvh(vector<bvh_node>& td, vector<vertex>& vert, const vector<tri>& ts);
void load_obj(const string& fn, vector<bvh_node>& td, vector<tri>& trd, vector<vertex>& vertices,
	uint mesh_id)
{
	vector<float3> poss;
	vector<float3> norms;
	vector<float2> texcords;

	vector<uint> indices;

	ifstream inf(fn);
	char comm[256] = { 0 };

	while (inf)
	{
		inf >> comm;
		if (!inf) break;
		if (strcmp(comm, "#") == 0) continue;
		else if (strcmp(comm, "v") == 0)
			poss.push_back(readvec3(inf));
		else if (strcmp(comm, "vn") == 0)
			norms.push_back(readvec3(inf));
		else if (strcmp(comm, "vt") == 0)
		{
			float u, v;
			inf >> u >> v;
			texcords.push_back(float2(u, v));
		}
		else if (strcmp(comm, "f") == 0)
		{
			for (uint ifa = 0; ifa < 3; ++ifa)
			{
				vertex v;
				uint ip, in, it;
				inf >> ip;
				v.pos = poss[ip - 1];
				if ('/' == inf.peek())
				{
					inf.ignore();
					if ('/' != inf.peek())
					{
						inf >> it;
						v.tex = texcords[it - 1];
					}
					if ('/' == inf.peek())
					{
						inf.ignore();
						inf >> in;
						v.norm = norms[in - 1];
					}
				}

				auto iv = find(vertices.begin(), vertices.end(), v);
				if (iv == vertices.end())
				{
					indices.push_back(vertices.size());
					vertices.push_back(v);
				}
				else
				{
					indices.push_back(distance(vertices.begin(), iv));
				}
			}
		}
	}

	for (uint ix = 0; ix < indices.size(); ix += 3)
	{
		trd.push_back(tri(indices[ix], indices[ix + 1], indices[ix + 2], mesh_id));
	}

	build_mesh_bvh(td, vertices, trd);
}


float3 find_min_bound(vector<vertex>& vert, tri t)
{
	return float3(XMVectorMin(vert[t.a].pos, XMVectorMin(vert[t.b].pos, vert[t.c].pos)));
}
float3 find_max_bound(vector<vertex>& vert, tri t)
{
	return float3(XMVectorMax(vert[t.a].pos, XMVectorMax(vert[t.b].pos, vert[t.c].pos)));
}
float3 center(vector<vertex>& vert, tri t)
{
	static const float one_third = 1.f / 3.f;
	return one_third * (vert[t.a].pos + vert[t.b].pos + vert[t.c].pos);
}

inline float get_axis(int x, float3 v)
{
	if (x == 0) return v.x;
	else if (x == 1) return v.y;
	else if (x == 2) return v.z;
	throw exception("index out of range");
}

struct cret
{
	int ptr;
	bool is_leaf;
	cret(int p, bool l)
		: ptr(p), is_leaf(l) {}
};

cret construct_mesh_bvh_node(vector<bvh_node>& td, vector<vertex>& vert, 
	const vector<tri>& ts, vector<tri> yts, int axis, int parent)
{
	if (yts.size() == 0)
	{
		return cret(-1, true);
	}
	else if (yts.size() == 1)
	{
		return cret(distance(ts.begin(), find(ts.begin(), ts.end(), yts[0])), true);
		//int r = td.size();
		//float3 mn(FLT_MAX,FLT_MAX,FLT_MAX), mx(FLT_MIN,FLT_MIN,FLT_MIN);
		//aabb b(mn, mx);
		//b.add_pnt(vert[yts[0].a].pos);
		//b.add_pnt(vert[yts[0].b].pos);
		//b.add_pnt(vert[yts[0].c].pos);
		
		//td.push_back(bvh_node(b, distance(ts.begin(), find(ts.begin(), ts.end(), yts[0])), axis));
		//td.push_back(bvh_node(mn, mx, -1,
		//	distance(ts.begin(), find(ts.begin(), ts.end(), yts[0])), axis));
		//return r;
	}
	else 
	{
		int r = td.size();
		td.push_back(bvh_node(aabb(), -1, -1, 0, false, false));
		sort(yts.begin(), yts.end(), [&](tri a, tri b)
		{
			float3 ac = center(vert, a);
			float3 bc = center(vert, b);
			return get_axis(axis, ac) > get_axis(axis, bc);
		});
		auto half = yts.size() / 2;
		auto left_h = vector<tri>(yts.begin(), yts.begin() + half);
		auto right_h = vector<tri>(yts.begin() + half, yts.end());
		cret left = construct_mesh_bvh_node(td, vert, ts, left_h, (axis+1) % 3, r);
		cret right = construct_mesh_bvh_node(td, vert, ts, right_h, (axis+1) % 3, r);
		aabb bunds;
		if (left.is_leaf)
		{
			tri t = ts[left.ptr];
			bunds.add_pnt(vert[t.a].pos);
			bunds.add_pnt(vert[t.b].pos);
			bunds.add_pnt(vert[t.c].pos);
		}
		else
			bunds = aabb(bunds, td[left.ptr].bounds);
		if (right.is_leaf)
		{
			tri t = ts[right.ptr];
			bunds.add_pnt(vert[t.a].pos);
			bunds.add_pnt(vert[t.b].pos);
			bunds.add_pnt(vert[t.c].pos);
		}
		else
			bunds = aabb(bunds, td[right.ptr].bounds);

		td[r] = (bvh_node(bunds, left.ptr, right.ptr, axis, left.is_leaf, right.is_leaf));
		return cret(r, false);
	}
}

void build_mesh_bvh(vector<bvh_node>& td, vector<vertex>& vert, const vector<tri>& ts)
{
	vector<tri> yts = ts;
	cret r = construct_mesh_bvh_node(td, vert, ts, yts, 0, 0);
	//printf("%i", r.ptr);
	//blahasdf
}


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
	camera cam;

	data_buffer<bvh_node> scene_tree;
	data_buffer<vertex> scene_vertices;
	data_buffer<tri> scene_triangles;
	data_buffer<smesh> scene_meshes;

public:
	pxp_app() : dx_app(true),
		cam(float3(0,0,-2), float3(0, 0, 0), 0.1f, 1000.f, to_radians(45.f))
	{
	}

	void load() override
	{
		q = create_ndc_quad(device);
		s = shader(device, read_data_from_package(L"vs.cso"), read_data_from_package(L"raytrace_ps.cso"),
			posnormtex_layout, _countof(posnormtex_layout));
		buf = constant_buffer<b>(device, 0, { 0 });
		
		vector<bvh_node> bvhdata;
		//bvhdata.push_back(bvh_node(float3(-1, -1, -1), float3(1, 1, 1), 1, 2));
		//bvhdata.push_back(bvh_node(float3(-1, -1, -1), float3(1, 1, 1), -1, 0));
		//bvhdata.push_back(bvh_node(float3(-1, -1, -1), float3(1, 1, 1), -1, 1));
		vector<tri> tridata;
		//tridata.push_back(tri(0, 1, 2, 0));
		//tridata.push_back(tri(3, 4, 5, 0));
		vector<vertex> vertexdata;
		//vertexdata.push_back(vertex(float3(-.5f, .5f, 0), float3(0, 0, 1), float2(0, 0)));
		//vertexdata.push_back(vertex(float3(.5f, -.5f, 0), float3(0, 0, 1), float2(1, 1)));
		//vertexdata.push_back(vertex(float3(-.5f, -.5f, 0), float3(0, 0, 1), float2(0, 1)));

		//vertexdata.push_back(vertex(float3(-.5f, .5f, 0), float3(0, 0, 1), float2(0, 0)));
		//vertexdata.push_back(vertex(float3(.5f, -.5f, 0), float3(0, 0, 1), float2(1, 1)));
		//vertexdata.push_back(vertex(float3(.5f, .5f, 0), float3(0, 0, 1), float2(0, 1)));
		vector<smesh> meshdata;
		meshdata.push_back(smesh(float4x4::identity(), float4(1, .5f, 0, 0)));

		load_obj("blah.obj", bvhdata, tridata, vertexdata, 0);

		scene_tree = data_buffer<bvh_node>(device, bvhdata);
		scene_vertices = data_buffer<vertex>(device, vertexdata);
		scene_triangles = data_buffer<tri>(device, tridata);
		scene_meshes = data_buffer<smesh>(device, meshdata);
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
		buf.bind(context, shader_stage::Pixel);
		
		scene_tree.bind(context, shader_stage::Pixel, 0);
		scene_vertices.bind(context, shader_stage::Pixel, 1);
		scene_triangles.bind(context, shader_stage::Pixel, 2);
		scene_meshes.bind(context, shader_stage::Pixel, 3);

		q->draw(context);
		buf.unbind(context, shader_stage::Pixel);

		scene_tree.unbind(context, shader_stage::Pixel, 0);
		scene_vertices.unbind(context, shader_stage::Pixel, 1);
		scene_triangles.unbind(context, shader_stage::Pixel, 2);
		scene_meshes.unbind(context, shader_stage::Pixel, 3);

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