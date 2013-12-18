struct psinput
{
	float4 pos : SV_POSITION;
	float2 texc : TEXCOORD0;
	float3 normW : NORMAL;
};

cbuffer b : register(b0)
{
	float t;
	float3 cam_o;
	float3 ww; float wwww;
	float3 vv; float vvvv;
	float3 uu; float uuuu;
};

struct bvh_node
{
	float3 aabb_min;
	uint left_ptr; //-1 = right_ptr is ptr to triangle
	float3 aabb_max;
	uint right_ptr;
};

struct tri
{
	uint a, b, c;
	uint mesh_id;
};

struct vertex
{
	float3 pos;
	float3 norm;
	float2 tex;
};

struct mesh
{
	float4x4 inv_world;
	float4 color;
};

StructuredBuffer<bvh_node> scene_tree;
StructuredBuffer<vertex> scene_vertices;
StructuredBuffer<tri> scene_triangles;
StructuredBuffer<mesh> scene_meshes;

bool triangle_hit(uint triptr, in float3 ro, in float3 rd,
	inout float t, out float3 norm, out float2 tex)
{
	tri ttt = scene_triangles[triptr];
	float3 v0 = scene_vertices[ttt.a].pos;
		float3 v1 = scene_vertices[ttt.b].pos;
		float3 v2 = scene_vertices[ttt.c].pos;
		float u, v;
	float3 e1 = v1 - v0;
		float3 e2 = v2 - v0;
		float3 pv = cross(rd, e2);
		float det = dot(e1, pv);
	if (det == 0)
		return false;
	float idet = 1.f / det;
	float3 tv = ro - v0;
		u = dot(tv, pv) * idet;
	if (u < 0 || u > 1.f)
		return false;
	float3 qv = cross(tv, e1);
		v = dot(rd, qv) * idet;
	if (v < 0 || u + v > 1)
		return false;
	float nt = dot(e2, qv)*idet;
	if (nt > 0 && nt < t)
	{
		t = nt;
		float w = 1 - (u + v);
		norm = scene_vertices[ttt.a].norm*w + scene_vertices[ttt.b].norm*u + scene_vertices[ttt.c].norm*v;
		tex = scene_vertices[ttt.a].tex*w + scene_vertices[ttt.b].tex*v + scene_vertices[ttt.c].tex*u;
		return true;
	}
	return false;
	/*norm = float3(0, 0, 0);
	tex = float2(0, 0);
	return true;*/
}

bool inside_aabb(in float3 p, in float3 Min, in float3 Max)
{
	if (p.x >= Min.x && p.x <= Max.x &&
		p.y >= Min.y && p.y <= Max.y &&
		p.z >= Min.z && p.z <= Max.z)
		return true;
	return false;
}

bool hit_aabb(in float3 ro, in float3 rd, in float3 amin, in float3 amax)
{
	if (inside_aabb(ro, amin, amax)) return true;
	float3 rrd = 1.f / rd;
	float tx1 = (amin.x - ro.x)*rrd.x;
	float tx2 = (amax.x - ro.x)*rrd.x;
	float tmin = min(tx1, tx2);
	float tmax = max(tx1, tx2);
	float ty1 = (amin.y - ro.y)*rrd.y;
	float ty2 = (amax.y - ro.y)*rrd.y;
	tmin = min(tmin, min(ty1, ty2));
	tmax = max(tmax, max(ty1, ty2));
	float tz1 = (amin.z - ro.z)*rrd.z;
	float tz2 = (amin.z - ro.z)*rrd.z;
	tmin = min(tmin, min(tz1, tz2));
	tmax = max(tmax, max(tz1, tz2));
	return tmax >= tmin;
}

bool hit_aabb(in float3 ro, in float3 rd, in float3 amin, in float3 amax, out float t)
{
	if (inside_aabb(ro, amin, amax)) return true;
	float3 rrd = 1.f / rd;
		float tx1 = (amin.x - ro.x)*rrd.x;
	float tx2 = (amax.x - ro.x)*rrd.x;
	float tmin = min(tx1, tx2);
	float tmax = max(tx1, tx2);
	float ty1 = (amin.y - ro.y)*rrd.y;
	float ty2 = (amax.y - ro.y)*rrd.y;
	tmin = min(tmin, min(ty1, ty2));
	tmax = max(tmax, max(ty1, ty2));
	float tz1 = (amin.z - ro.z)*rrd.z;
	float tz2 = (amin.z - ro.z)*rrd.z;
	tmin = min(tmin, min(tz1, tz2));
	tmax = max(tmax, max(tz1, tz2));
	t = min(tmax, tmin);
	return tmax >= tmin;
}

static const int stack_size = 8;

bool scene_hit(in float3 ro, in float3 rd,
	inout float t, out float3 norm, out float2 tex, out uint mesh_id)
{
	norm = float3(0, 0, 0);
	tex = float2(0, 0);
	mesh_id = -1;
	uint stack[stack_size];
	uint stack_ptr = stack_size-1;
	stack[stack_ptr] = 0;
	stack_ptr--; //push root node
	while (stack_ptr != stack_size-1)
	{
		bvh_node n = scene_tree[stack[stack_ptr+1]];
		if (n.left_ptr == -1)
		{
			//add world transform
			uint nmesh_id = scene_triangles[n.right_ptr].mesh_id;
			//mesh m = scene_meshes[mesh_id];
			stack_ptr++;
			float nt = t;
			float3 nnorm = norm;
				float2 ntex = tex;
			if (triangle_hit(n.right_ptr, ro, rd, nt, nnorm, ntex))
			{
				t = nt;
				norm = nnorm;
				tex = ntex;
				mesh_id = nmesh_id;
				return true;
			}
		}
		//else
		//{
		//	float left_t = 0, right_t = 0;
		//	bool left_hit = hit_aabb(ro, rd, scene_tree[n.left_ptr].aabb_min, scene_tree[n.left_ptr].aabb_max, left_t);
		//	bool right_hit = hit_aabb(ro, rd, scene_tree[n.right_ptr].aabb_min, scene_tree[n.right_ptr].aabb_max, right_t);
		//	if (left_hit && right_hit)
		//	{
		//		if (left_t < right_t)
		//		{
		//			stack_ptr++;
		//			stack[stack_ptr] = n.left_ptr;
		//			stack_ptr--;
		//		}
		//		else
		//		{
		//			stack_ptr++;
		//			stack[stack_ptr] = n.right_ptr;
		//			stack_ptr--;
		//		}
		//	}
		//	else
		//	{
		//		if (left_hit)
		//		{
		//			stack_ptr++;
		//			stack[stack_ptr] = n.left_ptr;
		//			stack_ptr--;
		//		}
		//		else if (right_hit)
		//		{
		//			stack_ptr++;
		//			stack[stack_ptr] = n.right_ptr;
		//			stack_ptr--;
		//		}
		//	}
		//}
		else if (hit_aabb(ro, rd, n.aabb_min, n.aabb_max))
		{
			stack_ptr++;
			float left_t = -1;
			float right_t = -1;
			bool l = false;
			l = hit_aabb(ro, rd, scene_tree[n.left_ptr].aabb_min, scene_tree[n.left_ptr].aabb_max, left_t);
			bool r = false; 
			r = hit_aabb(ro, rd, scene_tree[n.right_ptr].aabb_min, scene_tree[n.right_ptr].aabb_max, right_t);

			if (l && r)
			{
				if (left_t < right_t)
				{
					stack[stack_ptr] = n.left_ptr;
					stack_ptr--;
				}
				else
				{
					stack[stack_ptr] = n.right_ptr;
					stack_ptr--;
				}
			}
			else
			{
				if (l)
				{
					stack[stack_ptr] = n.left_ptr;
					stack_ptr--;
				}
				else if (r)
				{
					stack[stack_ptr] = n.right_ptr;
					stack_ptr--;
				}
			}
		}
		//else if (hit_aabb(ro, rd, n.aabb_min, n.aabb_max))
		//{
		//	stack_ptr++; //pop this node
		//	stack[stack_ptr] = n.right_ptr;
		//	stack_ptr--;
		//	stack[stack_ptr] = n.left_ptr;
		//	stack_ptr--;
		//}
	}
	return false;
}

float4 main(psinput i) : SV_TARGET
{
	float2 p = -1.f + 2.f *i.texc;
	p.y *= -1;

	float3 ro = cam_o;
	float3 rd = normalize(p.x*uu + p.y*vv + 2.5*ww);
	float t = 10000;
	float3 norm = float3(0, 0, 0); float2 tex = float2(0, 0);
		uint mid = -1;
	if (scene_hit(ro, rd, t, norm, tex, mid))
	{
		mesh m = scene_meshes[mid];
		return float4(norm+m.color.xyz, 1);//m.color.xyz, 1);
	}
	else
	{
		return float4(0, 0, 0, 1);
	}


	//return float4((float2(sin(t)*4, cos(t)*4) + i.texc)*.5f*abs(sin(t)), 0.0f, 1.0f);
}