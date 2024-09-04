
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
     if(horizontal)
     { 
         for(int i = 1; i < 9; ++i)
         {
            result += texture(image,  clamp(TexCoords + vec2(tex_offset.x * i , 0.0), 0.0, 1.0 )).rgb * weight[i];
            result += texture(image,  clamp(TexCoords - vec2(tex_offset.x * i,  0.0), 0.0, 1.0 )).rgb * weight[i];
         }
     }
     else
     {
         for(int i = 1; i < 9; ++i)
         {
             result += texture(image, clamp(TexCoords + vec2(0.0, tex_offset.y * i), 0.0, 1.0 )).rgb * weight[i];
             result += texture(image, clamp(TexCoords - vec2(0.0, tex_offset.y * i), 0.0, 1.0 )).rgb * weight[i];
         }
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