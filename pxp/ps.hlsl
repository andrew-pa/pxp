struct psinput
{
	float4 pos : SV_POSITION;
	float2 texc : TEXCOORD0;
	float3 normW : NORMAL;
};

float4 main(psinput i) : SV_TARGET
{
	return float4(i.texc, 0, 1);
}