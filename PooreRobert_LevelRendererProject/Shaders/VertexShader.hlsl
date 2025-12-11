//VertexShader
#pragma pack_matrix(row_major)
struct VS_IN
{
    float3 inputPos : POSITION;
    float3 inputUVW : UVW;
    float3 inputNormal : NORMAL;
};

struct VS_OUT
{
    float4 posH : SV_POSITION;
    float3 posW : WORLD;
    float3 normW : NORMAL;
};

struct _OBJ_ATTRIBUTES_
{
    float3 Kd; // diffuse reflectivity
    float d; // dissolve (transparency) 
    float3 Ks; // specular reflectivity
    float Ns; // specular exponent
    float3 Ka; // ambient reflectivity
    float sharpness; // local reflection map sharpness
    float3 Tf; // transmission filter
    float Ni; // optical density (index of refraction)
    float3 Ke; // emissive reflectivity
    uint illum; // illumination model
};

cbuffer SceneData : register(b0)
{
    vector _lightDirection, _lightColor, _sunAmbient, _cameraPos;
    matrix viewMatrix, projectionMatrix;
};


cbuffer MeshData : register(b1)
{
    matrix worldMatrix;
    _OBJ_ATTRIBUTES_ material;
};


VS_OUT main(VS_IN input) 
{
    VS_OUT output;
	
    output.posH = mul(float4(input.inputPos, 1), worldMatrix);
    output.posW = output.posH;
    output.posH = mul(output.posH, viewMatrix);
    output.posH = mul(output.posH, projectionMatrix);

    output.normW = mul(float4(input.inputNormal, 0), worldMatrix).xyz; 
	
    return output;
	
};