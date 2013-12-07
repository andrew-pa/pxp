#pragma pack_matrix( row_major )


struct VertexShaderInput
{
	float3 pos : POSITION;
	float3 norm : NORMAL;
	float2 texc : TEXCOORD;
};

struct VertexShaderOutput
{
	float4 pos : SV_POSITION;
	float2 texc : TEXCOORD0;
	float3 normW : NORMAL;

};

VertexShaderOutput main(VertexShaderInput input)
{
	VertexShaderOutput output;
	float4 pos = float4(input.pos, 1.0f);
	output.pos = pos;
	output.texc = input.texc;
	output.normW = input.norm;
	return output;
}
