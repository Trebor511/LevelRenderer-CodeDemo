//PixelShader
// an ultra simple hlsl pixel shader
// TODO: Part 3A 
// TODO: Part 4B 
// TODO: Part 4C 
// TODO: Part 4F 

struct PS_IN
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

float4 main(PS_IN input) : SV_TARGET
{
    //Cashed Results, Large use of Swizzlers
    float3 lightDir = normalize(-_lightDirection.xyz);
    float lightRatio = saturate(dot(lightDir, normalize(input.normW))); 
    float3 amblightRatio = saturate(material.Ka * _sunAmbient.xyz);
    float3 directionalLight = lightRatio * _lightColor.xyz;
    
    float3 viewDir = normalize(_cameraPos.xyz - input.posW);
    float3 halfVector = normalize(normalize(_lightDirection.xyz) + viewDir);
    float intensity = max(pow(saturate(dot(normalize(input.normW), halfVector)), material.Ns), 0);
    float3 reflectedLight = _lightColor.xyz * material.Ks * intensity;
    
    float3 result = saturate(directionalLight + amblightRatio) * material.Kd + reflectedLight + material.Ke;

    return float4(result, 0); 
}