const char* general_pbr_vert = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
    mat3 TBN;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
    vec4 worldPosition = model * vec4(aPos, 1.0);
    mat3 normalMatrix = mat3(transpose(inverse(model)));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 rawT = normalMatrix * aTangent;
    vec3 rawB = normalMatrix * aBitangent;
    vec3 T;
    vec3 B;

    vec3 orthogonalT = rawT - N * dot(N, rawT);
    if (length(orthogonalT) < 0.001 || length(rawB) < 0.001) {
        vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
        T = normalize(cross(up, N));
        B = normalize(cross(N, T));
    }
    else {
        // Imported tangent/bitangent vectors are not guaranteed to remain
        // orthogonal after a non-uniform model transform.  Rebuild an
        // orthonormal, handed TBN basis before applying a normal map.
        T = normalize(orthogonalT);
        float handedness = dot(cross(N, T), rawB) < 0.0 ? -1.0 : 1.0;
        B = normalize(cross(N, T)) * handedness;
    }

    vs_out.FragPos = worldPosition.xyz;
    vs_out.Normal = N;
    vs_out.TexCoords = aTexCoords;
    vs_out.FragPosLightSpace = lightSpaceMatrix * worldPosition;
    vs_out.TBN = mat3(T, B, N);

    gl_Position = projection * view * worldPosition;
}
)";

const char* general_pbr_frag = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
    vec4 FragPosLightSpace;
    mat3 TBN;
} fs_in;

struct Direction_Light {
    vec3 direction;
    vec3 color;
    float intensity;
};
uniform Direction_Light light;

struct Point_Light {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float range;
};
uniform Point_Light point[4];
uniform int pointLightCount;

uniform vec3 viewPos;
uniform mat4 lightSpaceMatrix;

uniform sampler2D diffuseMap;
uniform sampler2D baseTexture;
uniform sampler2D normalMap;
uniform sampler2D specularMap;
uniform sampler2D reflectionMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D metallicRoughnessMap;
uniform sampler2D aoMap;
uniform sampler2D emissiveMap;
uniform sampler2D shadowMap;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

uniform bool hasDiffuseMap;
uniform bool hasNormalMap;
uniform bool hasSpecularMap;
uniform bool hasReflectionMap;
uniform bool hasMetallicMap;
uniform bool hasRoughnessMap;
uniform bool hasMetallicRoughnessMap;
uniform bool hasAoMap;
uniform bool hasEmissiveMap;
uniform bool receiveShadows;

uniform bool usesSpecularGlossinessWorkflow;
uniform bool hasIBL;

uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform vec3 emissiveColor;
uniform float metallic;
uniform float roughness;
uniform float opacity;
uniform float iblIntensity;

const float PI = 3.14159265359;

vec3 getBaseColor();
vec3 getNormal();
float getMetallic();
float getRoughness();
float getAmbientOcclusion();
vec3 getEmissiveColor();

vec3 getF0(vec3 albedo, float metalness);
float DirectionShadow(vec4 fragPosLightSpace, vec3 normal);
vec3 evaluatePBRLight(vec3 albedo, float metalness, float roughnessValue, vec3 N, vec3 V, vec3 L, vec3 radiance);
float DistributionGGX(vec3 N, vec3 H, float roughnessValue);
float GeometrySchlickGGX(float NdotV, float roughnessValue);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughnessValue);
vec3 FresnelSchlick(float cosTheta, vec3 F0);
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughnessValue);
vec3 evaluateIBL(vec3 albedo, float metalness, float roughnessValue, float ao, vec3 N, vec3 V);

void main() {
    vec3 albedo = getBaseColor();
    vec3 N = getNormal();
    vec3 V = normalize(viewPos - fs_in.FragPos);
    float metalness = getMetallic();
    float roughnessValue = getRoughness();
    float ao = getAmbientOcclusion();

    vec3 color = hasIBL ? evaluateIBL(albedo, metalness, roughnessValue, ao, N, V) : ambientColor * albedo * ao;

    if (light.intensity > 0.0) {
        vec3 L = normalize(-light.direction);
        vec3 radiance = light.color * light.intensity;
        float shadow = receiveShadows ? DirectionShadow(fs_in.FragPosLightSpace, N) : 0.0;
        color += (1.0 - shadow) * evaluatePBRLight(albedo, metalness, roughnessValue, N, V, L, radiance);
    }

    int lightCount = clamp(pointLightCount, 0, 4);
    for (int i = 0; i < lightCount; ++i) {
        vec3 lightVector = point[i].position - fs_in.FragPos;
        float distance = length(lightVector);
        float range = max(point[i].range, 0.01);
        if (distance > 0.001 && distance < range && point[i].intensity > 0.0) {
            vec3 L = lightVector / distance;
            float attenuationDenom = max(point[i].constant + point[i].linear * distance + point[i].quadratic * distance * distance, 0.001);
            float rangeFade = clamp(1.0 - pow(distance / range, 4.0), 0.0, 1.0);
            rangeFade *= rangeFade;
            vec3 radiance = point[i].color * point[i].intensity * rangeFade / attenuationDenom;
            color += evaluatePBRLight(albedo, metalness, roughnessValue, N, V, L, radiance);
        }
    }

    color += getEmissiveColor();
    FragColor = vec4(color, opacity);
}

vec3 getBaseColor() {
    vec3 baseColor = diffuseColor;
    if (hasDiffuseMap) {
        baseColor *= pow(texture(diffuseMap, fs_in.TexCoords).rgb, vec3(2.2));
    }
    return baseColor;
}

vec3 getNormal() {
    if (hasNormalMap) {
        vec3 tangentNormal = texture(normalMap, fs_in.TexCoords).rgb * 2.0 - 1.0;
        return normalize(fs_in.TBN * tangentNormal);
    }
    return normalize(fs_in.Normal);
}

float getMetallic() {
    float value = metallic;
    if (hasMetallicRoughnessMap) {
        value *= texture(metallicRoughnessMap, fs_in.TexCoords).b;
    }
    else if (hasMetallicMap) {
        value *= texture(metallicMap, fs_in.TexCoords).r;
    }
    return clamp(value, 0.0, 1.0);
}

float getRoughness() {
    float value = roughness;
    if (hasMetallicRoughnessMap) {
        value *= texture(metallicRoughnessMap, fs_in.TexCoords).g;
    }
    else if (hasRoughnessMap) {
        value *= texture(roughnessMap, fs_in.TexCoords).r;
    }
    return clamp(value, 0.04, 1.0);
}

float getAmbientOcclusion() {
    if (hasAoMap) {
        return texture(aoMap, fs_in.TexCoords).r;
    }
    return 1.0;
}

vec3 getEmissiveColor() {
    if (hasEmissiveMap) {
        return pow(texture(emissiveMap, fs_in.TexCoords).rgb, vec3(2.2)) * emissiveColor;
    }
    return emissiveColor;
}

vec3 getF0(vec3 albedo, float metalness) {

    vec3 dielectricF0 = vec3(0.04);

    if (usesSpecularGlossinessWorkflow) {
        dielectricF0 = specularColor;
        if (hasSpecularMap) {
            dielectricF0 *= texture(specularMap, fs_in.TexCoords).rgb;
        }
        dielectricF0 = clamp(dielectricF0, vec3(0.02), vec3(0.9));
    }

    return mix(dielectricF0, albedo, metalness);

}



vec3 evaluatePBRLight(vec3 albedo, float metalness, float roughnessValue, vec3 N, vec3 V, vec3 L, vec3 radiance) {
    vec3 H = normalize(V + L);
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    if (NdotL <= 0.0 || NdotV <= 0.0) {
        return vec3(0.0);
    }

    vec3 F0 = getF0(albedo, metalness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    float NDF = DistributionGGX(N, H, roughnessValue);
    float G = GeometrySmith(N, V, L, roughnessValue);

    vec3 numerator = NDF * G * F;
    float denominator = max(4.0 * NdotV * NdotL, 0.001);
    vec3 specularBRDF = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metalness);
    vec3 diffuseBRDF = kD * albedo / PI;

    return (diffuseBRDF + specularBRDF) * radiance * NdotL;
}

float DistributionGGX(vec3 N, vec3 H, float roughnessValue) {
    float a = roughnessValue * roughnessValue;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / max(PI * denom * denom, 0.001);
}

float GeometrySchlickGGX(float NdotV, float roughnessValue) {
    float r = roughnessValue + 1.0;
    float k = (r * r) / 8.0;
    return NdotV / max(NdotV * (1.0 - k) + k, 0.001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughnessValue) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughnessValue) * GeometrySchlickGGX(NdotL, roughnessValue);
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}
vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughnessValue) {
    return F0 + (max(vec3(1.0 - roughnessValue), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 evaluateIBL(vec3 albedo, float metalness, float roughnessValue, float ao, vec3 N, vec3 V) {
    vec3 F0 = getF0(albedo, metalness);
    float NdotV = max(dot(N, V), 0.0);
    vec3 F = FresnelSchlickRoughness(NdotV, F0, roughnessValue);

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metalness);

    vec3 irradiance = texture(irradianceMap, N).rgb;
    vec3 diffuseIBL = irradiance * albedo;

    vec3 R = reflect(-V, N);
    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(prefilterMap, R, roughnessValue * MAX_REFLECTION_LOD).rgb;
    if (hasReflectionMap) {
        prefilteredColor *= texture(reflectionMap, fs_in.TexCoords).rgb;
    }
    vec2 brdf = texture(brdfLUT, vec2(NdotV, roughnessValue)).rg;
    vec3 specularIBL = prefilteredColor * (F * brdf.x + brdf.y);

    return (kD * diffuseIBL + specularIBL) * ao * iblIntensity;
}
float DirectionShadow(vec4 fragPosLightSpace, vec3 normal) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 || projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    // Directional lights have a constant incident direction across the scene.
    vec3 lightDir = normalize(-light.direction);
    float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0005);
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += projCoords.z - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}
)";

const char* Vert_quad = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos.x, aPos.y, 0.0, 1.0); 
}  
)";

const char* Frag_quad = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float exposure;

void main()
{
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;
    // The scene framebuffer is HDR/linear. Map it before converting to the
    // display's sRGB response so bright light values do not clip.
    vec3 mappedColor = vec3(1.0) - exp(-hdrColor * max(exposure, 0.001));
    vec3 gammaCorrectedColor = pow(mappedColor, vec3(1.0 / 2.2));

    FragColor = vec4(gammaCorrectedColor, 1.0);
} 
)";

const char* Vertbasic_lighting = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

out vec3 FragPos;
out vec3 Normal;
out vec2 UV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = aNormal;
    UV = aUV;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* Fragbasic_lighting = R"(
#version 330 core
out vec4 FragColor;

in vec3 Normal; 
in vec3 FragPos;
in vec2 UV;
  
uniform sampler2D baseTexture;
uniform sampler2D normalTexture;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main()
{
    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;
  	
	vec4 baseColor = texture(baseTexture, UV);
	vec4 normalColor= texture(normalTexture, UV);
    // diffuse 
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0); 
    vec3 diffuse = diff * lightColor;
            
    vec3 result = (ambient + diffuse) * objectColor * baseColor.rgb*normalColor.rgb;
    FragColor = vec4(result, 1.0);
} 
)";

const char* Vertlight_cube = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

const char* Fraglight_cube = R"(
#version 330 core
out vec4 FragColor;

void main()
{
    FragColor = vec4(1.0); // set all 4 vector values to 1.0
}
)";

const char* Vert_depth_map = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}
)";

const char* Frag_depth_map = R"(
#version 330 core
void main()
{             
    // gl_FragDepth = gl_FragCoord.z;
}
)";

const char* Vertmodel_lighting = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

out VS_OUT{
   vec3 FragPos;
   vec3 Normal;
   vec2 UV;
   vec4 FragPosLightSpace;
}vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.Normal = aNormal;
    vs_out.UV = aUV;
    vs_out.FragPosLightSpace = lightSpaceMatrix * vec4(vs_out.FragPos, 1.0);
    gl_Position = projection * view * vec4(vs_out.FragPos, 1.0);
}
)";

const char* Fragmodel_lighting = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 UV;
    vec4 FragPosLightSpace;
} fs_in;
  
uniform sampler2D baseTexture;
uniform sampler2D shadowMap;
uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;

struct Direction_Light {
    vec3 direction;
    vec3 color;
    float intensity;
};
uniform Direction_Light light;

struct Point_Light {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;      
    float linear;         
    float quadratic;
    float range;
};
uniform Point_Light point[4];

uniform vec3 viewPos;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

uniform bool isGlass;
uniform bool isMirror;
uniform vec3 objectColor;

vec3 CalcDirLight(Direction_Light light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(Point_Light light, vec3 normal, vec3 fragPos, vec3 viewDir);
float DirectionShadow(vec4 fragPosLightSpace);

void main()
{      
     if(!isMirror){
        vec4 texColor = texture(baseTexture, fs_in.UV);
        texColor.rgb=pow(texColor.rgb, vec3(2.2));  
        vec3 norm = normalize(fs_in.Normal);
        vec3 viewDir = normalize(viewPos - fs_in.FragPos); 
        //ambient
        vec3 Ambient =ambient * texColor.rgb ;

        //Direction light  
        vec3 Direction = CalcDirLight(light, norm, viewDir)*texColor.rgb;
        float directionShadow = DirectionShadow(fs_in.FragPosLightSpace);
        Direction=( 1.0f - directionShadow ) * Direction ;

        //Point Light 
        vec3 Point = vec3(0.0);
        for(int i = 0; i < 4; i++) {
            Point +=  CalcPointLight(point[i], norm, fs_in.FragPos, viewDir)*texColor.rgb;
        }

        //lastColor
        vec3 result = Ambient +  Point +  Direction;
        FragColor=vec4 (result,1.0);
    }
    else{
        vec3 I = normalize( fs_in.FragPos-viewPos);
        vec3 R = reflect(I, normalize(fs_in.Normal));
        vec3 reflectColor = pow(texture(baseTexture, R.xy * 0.5 + 0.5).rgb, vec3(2.2)) * 0.3 + 0.2 * objectColor;
        reflectColor =min(reflectColor,1.0f);
        FragColor = vec4(reflectColor, 0.1);
    }
}

// calculates the color when using a directional light.
vec3 CalcDirLight(Direction_Light light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float spec =0;
    if(isGlass){
        vec3 halfwayDir = normalize(lightDir + viewDir); 
        spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    }
    // combine results
    vec3 diffuseDir = diffuse * diff;
    vec3 specularDir = specular * spec;
    vec3 resultLight=( diffuseDir + specularDir)* light.color * light.intensity;
    return resultLight;
}

// calculates the color when using a point light.
vec3 CalcPointLight(Point_Light light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);

    // specular shading
    float spec =0;
    if(isGlass){
        vec3 halfwayDir = normalize(lightDir + viewDir); 
        spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    }

    // attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    // combine results
    vec3 diffusePoint = diffuse * diff ;
    vec3 specularPoint = specular * spec;
    diffusePoint *= attenuation;
    specularPoint *= attenuation;
    vec3 resultLight=( diffusePoint + specularPoint)* light.color * light.intensity;
    return resultLight;
}

float DirectionShadow(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(shadowMap, projCoords.xy).r; 

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(fs_in.Normal);
    // Directional lights have a constant incident direction across the scene.
    vec3 lightDir = normalize(-light.direction);
    float bias = max(0.02 * (1.0 - dot(normal, lightDir)), 0.002);
    //float bias=0;
    // check whether current frag pos is in shadow
    // float shadow = currentDepth - bias > closestDepth  ? 1.0 : 0.0;
    // PCF
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;        
        }    
    }
    shadow /= 9.0;
    
    // keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)  shadow = 0.0;
        
    return shadow;
}
)";

const char* Fragmodel_cube = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;


in vec3 Normal; 
in vec3 FragPos;
in vec2 UV;

uniform vec3 lightColor;
uniform float lightVisualIntensity;

void main()
{
    FragColor = vec4(lightColor * lightVisualIntensity, 1.0);
}
)";

const char* Frag_highlight= R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;

void main()
{
    vec3 texColor = texture(scene, TexCoords).rgb;
    float brightness = dot(texColor, vec3(0.2126, 0.7152, 0.0722));
    if(brightness > 1.2)
        FragColor = vec4(texColor, 1.0);
    else
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
)";

const char* Frag_blur = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D image;

uniform bool horizontal;
float weight[] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 
                            0.0540540541, 0.0162162162, 0.0070000000, 
                            0.0030000000, 0.0010000000, 0.0005000000);
void main()
{             
     vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
     vec3 result = texture(image, TexCoords).rgb * weight[0];
      for(int i = 1; i < 9; ++i)
    {
        vec2 offset = horizontal ? vec2(tex_offset.x * i, 0.0) : vec2(0.0, tex_offset.y * i);
        
        result += texture(image, clamp(TexCoords + offset, vec2(0.0)+offset, vec2(1.0)-offset)).rgb * weight[i];
        result += texture(image, clamp(TexCoords - offset, vec2(0.0)+offset, vec2(1.0)-offset)).rgb * weight[i];
    }
     FragColor = vec4(result, 1.0);
}
)";


const char* Frag_bloom = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D scene;
uniform sampler2D bloomBlur;

void main()
{
    vec3 hdrColor = texture(scene, TexCoords).rgb;      
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    hdrColor += bloomColor;
    FragColor = vec4(hdrColor, 1.0);
}
)";

const char* Frag_Radialblur = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture; // Source scene texture
uniform vec2 center; 
uniform float strength;

void main() {
    vec2 dir = TexCoords - center; 
    vec4 color = texture(sceneTexture, TexCoords);

    //Avoid smearing caused by too bright light sources
    float threshold = 1.0; 
    color.rgb = min(color.rgb, vec3(threshold)); 

    float totalWeight = 1.0;
    for (float i = 1.0; i <= 10.0; i++) {
        vec2 offset = dir * (float(i) / 10 )* strength;
        vec4 sampleColor = texture(sceneTexture, TexCoords - offset);
        //Sampling as a blur color also has to be limited
        sampleColor = min(sampleColor, vec4(threshold)); 
        color += sampleColor * 0.1;
        totalWeight += 0.1;
    }

    FragColor = vec4(color.rgb / totalWeight,1.0); 
}
)";

const char* Frag_Motionblur = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture; // Source scene texture
uniform sampler2D lastTexture;  

void main() {
    vec3 color = texture(sceneTexture, TexCoords).rgb;
    vec3 blur  = texture(lastTexture, TexCoords).rgb;
     
    //Avoid smearing caused by too bright light sources
    float threshold = 1.0; 
    color = min(color, vec3(threshold)); 
    blur = min(blur, vec3(threshold));

    // Interpolate the current frame and the previous frame to achieve the effect of motion blur
    vec3 result = mix(color, blur, 0.9f); 

    FragColor=vec4(result,1.0);
}
)";

const char* Frag_cartoon = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture; // Source scene texture

void main()
{
    vec3 texColor = texture(sceneTexture, TexCoords).rgb;

    float edgeDetection[9] = float[](-1, -1, -1, 
                                      -1,  8, -1, 
                                      -1, -1, -1);

    vec2 tex_offset[9] = vec2[](
        vec2(-1.0,  1.0), vec2( 0.0,  1.0), vec2( 1.0,  1.0),
        vec2(-1.0,  0.0), vec2( 0.0,  0.0), vec2( 1.0,  0.0),
        vec2(-1.0, -1.0), vec2( 0.0, -1.0), vec2( 1.0, -1.0)
    );

    vec3 result = vec3(0.0);
    for(int i = 0; i < 9; i++)
    {
        vec3 sampleTex = texture(sceneTexture, TexCoords + tex_offset[i] / textureSize(sceneTexture, 0)).rgb;
        result += sampleTex * edgeDetection[i];
    }

    FragColor = vec4(result+texColor, 1.0);
}
)";

const char* Frag_ripple = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture; // Source scene texture
uniform vec2 rippleCenter;      // Ripple center in screen UV
uniform float time;             // Animation time
uniform float waveAmplitude;    // Wave amplitude
uniform float waveFrequency;    // Wave frequency
uniform float waveSpeed;        // Wave propagation speed

void main() {
    // Distance from current fragment to ripple center
    vec2 uv = TexCoords - rippleCenter;
    float dist = length(uv);
    // Use distance to calculate wave shape.
    float ripple = sin(dist * waveFrequency - time * waveSpeed) * waveAmplitude / (dist + 1.0);
    // Offset UV coordinates by the ripple amount.
    vec2 rippleTexCoords = TexCoords + uv * ripple;
    rippleTexCoords = clamp(rippleTexCoords, vec2(0.0), vec2(1.0));
    // Sample displaced scene color.
    vec4 sceneColor = texture(sceneTexture, rippleTexCoords);
    // Output final color.
    FragColor = sceneColor;
}
)";

const char* Frag_DownSample = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D u_texture;
uniform vec2 textureSize;
void main()
{
    vec3 result = vec3(0.0);
    //result +=  texture(u_texture, TexCoords).rgb;
    result += texture(u_texture, TexCoords + vec2(textureSize.x, 0.0)).rgb;
    result += texture(u_texture, TexCoords + vec2(-textureSize.x, 0.0)).rgb;
    result += texture(u_texture, TexCoords + vec2(0.0, textureSize.y)).rgb;
    result += texture(u_texture, TexCoords + vec2(0.0, -textureSize.y)).rgb;
    result /= 4.0;
    
    FragColor = vec4(result, 1.0);
}
)";

const char* Frag_UpSample = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D curMipDownSampletexture; 
uniform sampler2D lastMipUpSampletexture; 
uniform vec2 textureSize;

void main() {
   vec3 result = vec3(0.0);
    result += texture(lastMipUpSampletexture, TexCoords + vec2(textureSize.x, textureSize.y)).rgb;
    result += texture(lastMipUpSampletexture, TexCoords + vec2(textureSize.x, -textureSize.y)).rgb;
    result += texture(lastMipUpSampletexture, TexCoords + vec2(-textureSize.x, textureSize.y)).rgb;
    result += texture(lastMipUpSampletexture, TexCoords + vec2(-textureSize.x, -textureSize.y)).rgb;
    result += texture(curMipDownSampletexture, TexCoords).rgb ;
    result /= 5.0;
    
    FragColor = vec4(result, 1.0);
}
)";
