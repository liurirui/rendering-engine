
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

const char* Vertmodel_lighting = R"(
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

const char* Fragmodel_lighting = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec3 Normal; 
in vec3 FragPos;
in vec2 UV;
  
uniform sampler2D baseTexture;
struct Directiona_Light {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};
uniform Directiona_Light light;
struct Point_Light {
    vec3 Position;
    vec3 Color;
};

uniform Point_Light point[4];
uniform float shininess;
uniform vec3 viewPos;
uniform bool isGlass;
uniform bool isMirror;
uniform vec3 objectColor;
void main()
{      
     if(!isMirror){
        vec4 texColor = texture(baseTexture, UV);
        //ambient
        vec3 ambient = light.ambient * texColor.rgb ;

        // Direct light diffuse 
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(-light.direction);  
        float diff_straight = max(dot(norm, lightDir), 0.0);
        vec3 diffuse_straight = light.diffuse * diff_straight * texColor.rgb; 
        
        //Point Light diffuse 
        vec3 lighting = vec3(0.0);
        for(int i = 0; i < 4; i++)
        {
            // diffuse
            vec3 lightDir_point = normalize(point[i].Position - FragPos);
            float diff_point = max(dot(lightDir_point, norm), 0.0);
            vec3 diffuse_point = point[i].Color * diff_point * texColor.rgb;      
            // attenuation 
            float distance = length(FragPos - point[i].Position);
            diffuse_point *= 1.0 / (distance * distance) ;
            lighting += diffuse_point;
         }
        vec3 diffuse= lighting + diffuse_straight;
        // specular
        vec3 specular =vec3(0.0);
        //Only Glass have specular
        if(isGlass){
            vec3 viewDir = normalize(viewPos - FragPos);
            vec3 reflectDir = reflect(-lightDir, norm);  
            float spec = pow(max(dot(viewDir, reflectDir), 0.0),shininess);
            specular = light.specular * spec * texColor.rgb;  
        }
        else{
            specular =vec3(0.0);
        }
        //lastColor
        vec3 result = specular + diffuse + ambient;
        FragColor=vec4 (result,1.0);
    }
    else{
        vec3 I = normalize(FragPos - viewPos);
        vec3 R = reflect(I, normalize(Normal));
        vec3 reflectColor = texture(baseTexture, R.xy).rgb*0.2+0.3*objectColor;

        FragColor = vec4(reflectColor, 0.1);
    }
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

const char* Fragmodel_cartoon = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D cartoonTexture;

void main()
{
    vec3 texColor = texture(cartoonTexture, TexCoords).rgb;

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
        vec3 sampleTex = texture(cartoonTexture, TexCoords + tex_offset[i] / textureSize(cartoonTexture, 0)).rgb;
        result += sampleTex * edgeDetection[i];
    }

    FragColor = vec4(result+texColor, 1.0);
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
uniform float weight[9] = float[] (0.2270270270, 0.1945945946, 0.1216216216, 
                                   0.0540540541, 0.0162162162, 0.0070000000, 
                                   0.0030000000, 0.0010000000, 0.0005000000);

void main()
{             
     vec2 tex_offset = 1.0 / textureSize(image, 0); // gets size of single texel
     vec3 result = texture(image, TexCoords).rgb * weight[0];
     if(horizontal)
     { 
         for(int i = 1; i < 9; ++i)
         {
            result += texture(image, TexCoords + vec2(tex_offset.x * i , 0.0)).rgb * weight[i];
            result += texture(image, TexCoords - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
         }
     }
     else
     {
         for(int i = 1; i < 9; ++i)
         {
             result += texture(image, TexCoords + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
             result += texture(image, TexCoords - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
         }
     }
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
        vec2 offset = dir * (i / 10.0) * strength;
        color += texture(sceneTexture, TexCoords - offset) * 0.1;
        totalWeight += 0.1;
    }

    FragColor = color / totalWeight; 
}
)";

const char* Frag_Motionblur = R"(
#version 330 core
layout (location = 0) out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D sceneTexture;
uniform vec2 motionDirection; // 运动方向，(x, y) 偏移

void main() {
    vec4 color = texture(sceneTexture, TexCoords); // 当前像素的颜色

    // 模拟运动模糊，通过叠加多个样本
    float totalWeight = 1.0;
    for (float i = 1.0; i <= 10.0; i++) {
        vec2 offset = motionDirection * (i / 10.0);
        color += texture(sceneTexture, TexCoords - offset) * 0.1;
        totalWeight += 0.1;
    }

    FragColor = color / totalWeight; // 平均化结果
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