const char* general_pbr_vert = R"(
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
#ifdef HAS_NORMAL_MAP
layout (location = 3) in vec3 aTangent;
layout (location = 4) in vec3 aBitangent;
#endif

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;
#ifdef HAS_NORMAL_MAP
out mat3 TBN;
#endif

layout (std140, binding = 0) uniform CameraData {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 cameraPosition;
};

uniform mat4 model;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    
    #ifdef HAS_NORMAL_MAP
    vec3 T = normalize(mat3(model) * aTangent);
    vec3 B = normalize(mat3(model) * aBitangent);
    vec3 N = normalize(mat3(model) * aNormal);
    TBN = mat3(T, B, N);
    #endif

    gl_Position = projectionMatrix * viewMatrix * vec4(FragPos, 1.0);
}
)";

const char* general_pbr_frag = R"(
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
#ifdef HAS_NORMAL_MAP
in mat3 TBN;
#endif

layout (std140, binding = 0) uniform CameraData {
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 cameraPosition;
};

struct PointLightData {
    vec4 position;
    vec4 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    vec2 padding;
};

const int MAX_LIGHTS = 4;
layout (std140, binding = 1) uniform LightData {
    // Direction light
    vec4 directionLightDirection;
    vec4 directionLightColor;
    float directionLightIntensity;
    vec3 padding1;
    // Point lights
    int numPointLights;
    vec3 padding2;
    PointLightData pointLights[MAX_LIGHTS];
};

#ifdef HAS_DIFFUSE_MAP
uniform sampler2D diffuseMap;
#endif
#ifdef HAS_NORMAL_MAP
uniform sampler2D normalMap;
#endif
#ifdef HAS_SPECULAR_MAP
uniform sampler2D specularMap;
#endif
#ifdef HAS_METALLIC_MAP
uniform sampler2D metallicMap;
#endif
#ifdef HAS_ROUGHNESS_MAP
uniform sampler2D roughnessMap;
#endif
#ifdef HAS_AO_MAP
uniform sampler2D aoMap;
#endif
#ifdef HAS_EMISSIVE_MAP
uniform sampler2D emissiveMap;
#endif

// ���ʻ�������
uniform vec3 ambientColor;
uniform vec3 diffuseColor;
uniform vec3 specularColor;
uniform vec3 emissiveColor;
uniform float metallic;
uniform float roughness;

// ��Ӱ
#ifdef RECEIVE_SHADOWS
uniform sampler2D shadowMap;
uniform mat4 lightSpaceMatrix;
#endif

#define PI 3.14159265359

// ��������
vec3 getDiffuseColor();
vec3 getNormal();
float getMetallic();
float getRoughness();
float getAmbientOcclusion();
vec3 getEmissiveColor();

#ifdef USE_PBR
vec3 calculatePBR();
#elif defined(USE_BLINN_PHONG)
vec3 calculateBlinnPhong();
#endif

// PBR
float DistributionGGX(vec3 N, vec3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness);
vec3 FresnelSchlick(float cosTheta, vec3 F0, float metalness);

void main() {
    vec3 albedo = getDiffuseColor();
    vec3 normal = getNormal();
    float metalness = getMetallic();
    float roughnessVal = getRoughness();
    float ao = getAmbientOcclusion();
    vec3 emissive = getEmissiveColor();
    
    #ifdef USE_PBR
    vec3 color = calculatePBR();
    #elif defined(USE_BLINN_PHONG)
    vec3 color = calculateBlinnPhong();
    #endif
    
    color += emissive;
    FragColor = vec4(color, 1.0);
}
 
vec3 getDiffuseColor() {
    #ifdef HAS_DIFFUSE_MAP
    return texture(diffuseMap, TexCoords).rgb * diffuseColor;
    #else
    return diffuseColor;
    #endif
}

vec3 getNormal() {
    #ifdef HAS_NORMAL_MAP
    vec3 tangentNormal = texture(normalMap, TexCoords).rgb * 2.0 - 1.0;
    return normalize(TBN * tangentNormal);
    #else
    return normalize(Normal);
    #endif
}

float getMetallic() {
    #ifdef HAS_METALLIC_MAP
    return texture(metallicMap, TexCoords).r * metallic;
    #else
    return metallic;
    #endif
}

float getRoughness() {
    #ifdef HAS_ROUGHNESS_MAP
    return texture(roughnessMap, TexCoords).r * roughness;
    #else
    return roughness;
    #endif
}

float getAmbientOcclusion() {
    #ifdef HAS_AO_MAP
    return texture(aoMap, TexCoords).r;
    #else
    return 1.0;
    #endif
}

vec3 getEmissiveColor() {
    #ifdef HAS_EMISSIVE_MAP
    return texture(emissiveMap, TexCoords).rgb * emissiveColor;
    #else
    return emissiveColor;
    #endif
}

// PBR 
#ifdef USE_PBR
vec3 calculatePBR() {
    vec3 N = getNormal();
    vec3 V = normalize(cameraPosition.xyz - FragPos);
    vec3 albedo = getDiffuseColor();
    float metalness = getMetallic();
    float roughnessVal = getRoughness();
    float ao = getAmbientOcclusion();

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metalness);
    vec3 Lo = vec3(0.0);

    // Directional light contribution (if directionLightIntensity > 0)
    if (directionLightIntensity > 0.0) {
        vec3 L = normalize(-directionLightDirection.xyz);
        vec3 H = normalize(V + L);

        // No distance attenuation for directional light
        vec3 radiance = directionLightColor.rgb * directionLightIntensity;

        float NDF = DistributionGGX(N, H, roughnessVal);
        float G = GeometrySmith(N, V, L, roughnessVal);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metalness;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + NDF * G * F / max(4 * max(dot(N, V), 0.0) * NdotL, 0.001)) * radiance * NdotL;
    }

    // Point lights contribution
    for (int i = 0; i < numPointLights; ++i) {
        vec3 L = normalize(pointLights[i].position.xyz - FragPos);
        vec3 H = normalize(V + L);

        float distance = length(pointLights[i].position.xyz - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * distance * distance);
        vec3 radiance = pointLights[i].color.rgb * attenuation * pointLights[i].intensity;

        float NDF = DistributionGGX(N, H, roughnessVal);
        float G = GeometrySmith(N, V, L, roughnessVal);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metalness;

        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + NDF * G * F / max(4 * max(dot(N, V), 0.0) * NdotL, 0.001)) * radiance * NdotL;
    }

    vec3 ambient = vec3(0.03) * albedo * ao;
    return ambient + Lo;
}

float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx1 = GeometrySchlickGGX(NdotV, roughness);
    float ggx2 = GeometrySchlickGGX(NdotL, roughness);
    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
#endif

// Blinn-Phong 
#ifdef USE_BLINN_PHONG
vec3 calculateBlinnPhong() {
    vec3 N = getNormal();
    vec3 V = normalize(cameraPosition.xyz - FragPos);
    vec3 albedo = getDiffuseColor();
    vec3 result = ambientColor * albedo;

    // Directional light contribution (if directionLightIntensity > 0)
    if (directionLightIntensity > 0.0) {
        vec3 L = normalize(-directionLightDirection.xyz);
        vec3 H = normalize(L + V);
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = directionLightColor.rgb * diff * albedo;
        float spec = pow(max(dot(N, H), 0.0), 64.0);
        vec3 specular = directionLightColor.rgb * spec * specularColor;
        // No distance attenuation for directional light
        result += (diffuse + specular) * directionLightIntensity;
    }

    // Point lights contribution
    for (int i = 0; i < numPointLights; ++i) {
        vec3 L = normalize(pointLights[i].position.xyz - FragPos);
        vec3 H = normalize(L + V);
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = pointLights[i].color.rgb * diff * albedo;
        float spec = pow(max(dot(N, H), 0.0), 64.0);
        vec3 specular = pointLights[i].color.rgb * spec * specularColor;
        float distance = length(pointLights[i].position.xyz - FragPos);
        float attenuation = 1.0 / (pointLights[i].constant + pointLights[i].linear * distance + pointLights[i].quadratic * distance * distance);
        result += (diffuse + specular) * pointLights[i].intensity * attenuation;
    }
    return result;
}
#endif
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

void main()
{
    vec3 col = texture(screenTexture, TexCoords).rgb;
    vec3 gammaCorrectedColor = pow(col, vec3(1.0 / 2.2));

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
};
uniform Point_Light point[4];

uniform vec3 lightPos;
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
    vec3 lightDir = normalize(lightPos - fs_in.FragPos);
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

void main()
{
    FragColor=vec4(lightColor,1.0);
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

uniform sampler2D sceneTexture;
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

uniform sampler2D sceneTexture;
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

uniform sampler2D sceneTexture;

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

uniform sampler2D sceneTexture; // ��ʼ��������
uniform vec2 rippleCenter;      // �������ģ�ͨ������Ļ���꣩
uniform float time;             // ʱ����������Ʋ�����չ
uniform float waveAmplitude;    // �������
uniform float waveFrequency;    // ����Ƶ��
uniform float waveSpeed;        // ������չ�ٶ�

void main() {
    // ���㵱ǰƬ�ξ��벨�����ĵľ���
    vec2 uv = TexCoords - rippleCenter;
    float dist = length(uv);

    // ���ھ�����㲨����״��ʹ�����Ҳ�ģ��
    float ripple = sin(dist * waveFrequency - time * waveSpeed) * waveAmplitude / (dist + 1.0); 

    // ���� UV �������꣬���ݲ���Ч����̬����
    vec2 rippleTexCoords = TexCoords + uv * ripple;
    rippleTexCoords = clamp(rippleTexCoords, vec2(0.0), vec2(1.0));  // ��ֹ�������곬����Χ

    // ������������
    vec4 sceneColor = texture(sceneTexture, rippleTexCoords);

    // ������յ���ɫ
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