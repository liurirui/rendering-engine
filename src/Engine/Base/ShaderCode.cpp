
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
    FragColor = vec4(col, 1.0);
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
float ShadowCalculation(vec4 fragPosLightSpace);
void main()
{      
     if(!isMirror){
        vec4 texColor = texture(baseTexture, fs_in.UV);
        vec3 norm = normalize(fs_in.Normal);
        vec3 viewDir = normalize(viewPos - fs_in.FragPos); 
        //ambient
        vec3 Ambient =ambient * texColor.rgb ;

        //Direction light  
        vec3 Direction = CalcDirLight(light, norm, viewDir)*texColor.rgb;
        
        //Point Light 
        vec3 Point = vec3(0.0);
        for(int i = 0; i < 4; i++) Point+=CalcPointLight(point[i], norm, fs_in.FragPos, viewDir)*texColor.rgb;

        //lastColor
        //vec3 result = Point + Direction + Ambient;
        float shadow = ShadowCalculation(fs_in.FragPosLightSpace);
        vec3 result = (Ambient +  Point + (1.0 - shadow)* Direction);
         FragColor=vec4 (result,1.0);
    }
    else{
        vec3 I = normalize(fs_in.FragPos - viewPos);
        vec3 R = reflect(I, normalize(fs_in.Normal));
        vec3 reflectColor = texture(baseTexture, R.xy).rgb*0.2+0.3*objectColor;

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

float ShadowCalculation(vec4 fragPosLightSpace)
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
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);
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
    if(brightness > 0.8)
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
uniform float weight[] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 
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
    vec3 result = vec3(1.0) - exp(-hdrColor);
    FragColor = vec4(result, 1.0);
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

    float totalWeight = 1.0;
    for (float i = 1.0; i <= 10.0; i++) {
        vec2 offset = dir * (float(i) / 10 )* strength;
        color += texture(sceneTexture, TexCoords - offset) * 0.1;
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
    FragColor=vec4(0.1*color+0.9*blur,1.0);
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

uniform sampler2D sceneTexture;
uniform float iTime;
//create a uniform for resolution

//create a speed
float speed = 0.3;
vec2 iResolution=vec2(800,600);
#define MAX_RADIUS 2


//create a hash12 func
float hash12(vec2 p)
{
    vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 19.19);
    return fract((p3.x + p3.y) * p3.z);
}

//create a hash22 func
vec2 hash22(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy);
}


void main()
{
    //calculate the resolution for the grid cell division
    float resolution = 10;
    //normalize the fragCoord and multiply the resolution to extend the uv
    vec2 uv = TexCoords.xy / iResolution.y * resolution;
    //Grid Cell Division
    vec2 p0 = floor(uv);
    //create a finalCol to store the color
    vec3 finalCol = vec3(0.0);

    //create a v2 to store the circles's effect
    vec2 circles = vec2(0.);

    for(int j = -MAX_RADIUS;j<= MAX_RADIUS;++j)
    {
        for(int i = -MAX_RADIUS;i<= MAX_RADIUS;++i)
        {
            //move the p0 to the center of the cell
            vec2 pi = p0 + vec2(i, j);

            vec2 p = pi+hash22(pi);

            //create a time factor and distance calculation
            float t = fract(iTime * speed+hash12(pi));
            vec2 v = p - uv;

            float d = length(v) - (float(MAX_RADIUS)+1)*t;
            
            //d = sin(d*31)*smoothstep(-0.6, -0.3, d) * smoothstep(0., -0.3, d);
            //d *= (1 - t) * (1 - t);

            float h = 1e-3;
            float d1 = d - h;
            float d2 = d + h;
            float p1 = sin(31.*d1)*smoothstep(-0.6, -0.3, d1) * smoothstep(0., -0.3, d1);
            float p2 = sin(31.*d2)*smoothstep(-0.6, -0.3, d2) * smoothstep(0., -0.3, d2);
            float gradient = (p2 - p1) / (2 * h);
            d = gradient * (1 - t) * (1 - t);


            vec3 col = vec3(1.0);
            finalCol += col*d;

            circles += 0.5 * normalize(v) * d;

        }

    }

    //finalCol /= float((MAX_RADIUS*2+1)*(MAX_RADIUS*2+1));

    circles /= float((MAX_RADIUS*2+1)*(MAX_RADIUS*2+1));
    //cal the normal by the circles : x^2 + y^2 + z^2 = 1;
    vec3 normal = vec3(circles, sqrt(1.0 - dot(circles, circles)));
    //cal the instensity of the benduv
    float tempVal = smoothstep(0.1, 0.6, abs(fract(0.05*iTime+0.5)*2-1));
    float intensity = mix(0.01,0.15,tempVal);
    //cal the bend uv
    vec2 bendUV = uv/resolution - intensity*normal.xy;
    finalCol = texture(sceneTexture, bendUV).rgb;

    //cal the specular
    //create half vector
    vec3 halfVector = vec3(1.0, 0.7, 0.5);
    float specular = 5 * pow(clamp(dot(normal, normalize(halfVector)), 0., 1.), 6.);


    FragColor = vec4(finalCol, 1.0)+specular;
}
)";