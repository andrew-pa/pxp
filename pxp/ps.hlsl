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

float maxcomp(in float3 p)
{
	return max(p.x, max(p.y, p.z));
}
float sdbox(float3 p, float3 b)
{
	float3 di = abs(p) - b;
	float mc = maxcomp(di);
	return min(mc, length(max(di, 0)));
}

float sdtorus(float3 p, float2 t)
{
	float2 q = float2(length(p.xz) - t.x, p.y);
		return length(q) - t.y;
}

float smin(float a, float b, float k)
{
	float res = exp(-k*a) + exp(-k*b);
	return -log(res) / k;
}


float3 mod(float3 x, float3 y)
{
	return x - y * floor(x / y);
}

float asdf(float3 p)
{
	float3 q = p;// mod(p, float3(4, 4, 4)) - .5*float3(4, 4, 4);
		return sdtorus(q, float2(1.f, .25f));//smin(sdtorus(q, float2(1.f, .25f)), sdbox(q, float3(1.15f, 0.1f, 0.1f)), 32);
}

float4 map(in float3 p)
{
	//float cof = .1f*sin(t*.4f);
	//return float4(asdf(p)+
	//	sin(cof*p.x)*sin(cof*p.y)*sin(cof*p.z), 1, 0, 0);
	float d = sdbox(p, float3(1.0f,1.0f,1.f));
	float4 rs = float4(d, 1, 0, 0);

	float s = 1;
	for (int m = 0; m < 4; ++m)
	{
		float3 a = mod((p*s) , 2.f) - 1.f;
		s *= 3.f;
		float3 r = abs(1.f - 3.f * abs(a));
			float da = max(r.x, r.y);
		float db = max(r.y, r.z);
		float dc = max(r.z, r.x);
		float c = (min(da, min(db, dc)) - 1.0) / s;
		if (c > d)
		{
			d = c;
			rs = float4(d, min(rs.y, .2*da*db*dc), (1 + float(m)) / 4.0f, 0.f);
		}
	}
	return rs;
}

float4 intersect(in float3 ro, in float3 rd)
{
	float t = 0.f;
	for (int i = 0; i < 64; ++i)
	{
		float4 h = map(ro + rd*t);
		if (h.x < 0.002)
			return float4(t, h.y, h.z, h.w);
		t += h.x;
	}
	return float4(-1.f,-1.f,-1.f,-1.f);
}

float3 normal(in float3 pos)
{
	float3 eps = float3(.001f, 0.f, 0.f);
		float3 nor;
	nor.x = map(pos + eps.xyy).x - map(pos - eps.xyy).x;
	nor.y = map(pos + eps.yxy).x - map(pos - eps.yxy).x;
	nor.z = map(pos + eps.yyx).x - map(pos - eps.yyx).x;
	return normalize(nor);
}

float3 colormap(float mz)
{
	float3 col;
	col.x = .6f + .4f*cos(5.4f + 6.2831f*mz);
	col.y = .6f + .4f*cos(5.7f + 6.2831f*mz);
	col.z = .6f + .4f*cos(2.0f + 6.2831f*mz);
	return col;
}

float3 raycolor(in float3 ro, in float3 rd)
{
	float3 light = float3(1.f, .9f, .3f);

	float3 col = float3(0, 0, 0);

	float4 m = intersect(ro, rd);
	if (m.x > 0.0f)
	{
		float3 pos = ro + m.x*rd;
			float3 norm = normal(pos);
		float shf;
		float4 shadow = intersect(pos + light * 4, -light);
		if (shadow.x > 0.01 && shadow.x < (4 - 0.01))
			shf = 0.3f;
		else
			shf = 1.0f;
		col = shf * colormap(m.z) * max(0.1, dot(light, norm));
	}
	return col;
}

float4 main(psinput i) : SV_TARGET
{
	float2 p = -1.f + 2.f *i.texc;
	p.y *= -1;
//	p.x *= 1.33f;
	
	float3 ro = cam_o; //2.1f*float3(2.5f*sin(0.25f*t), 1.0 + 1.0*cos(t*.13f), 2.5f*cos(0.25f*t));
	//float3 ww = //normalize(float3(0, 0, 0) - ro);
	//float3 uu = //normalize(cross(float3(0, 1, 0), ww));
	//float3 vv = //normalize(cross(ww, uu));
	float3 rd = normalize(p.x*uu + p.y*vv + 2.5*ww);

	return float4(sqrt(raycolor(ro, rd)), 1);
}