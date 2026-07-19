#version 330 core
out vec4 fragmentColor;

in vec3 fragmentPosition;
in vec3 fragmentVertexNormal;
in vec2 fragmentTextureCoordinate;

struct Material {
    vec3 diffuseColor;
    vec3 specularColor;
    float shininess;
}; 

struct DirectionalLight {
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool bActive;
};

struct PointLight {
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool bActive;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    bool bActive;
};

#define TOTAL_POINT_LIGHTS 5

uniform bool bUseTexture = false;
uniform bool bUseLighting = false;
uniform vec4 objectColor = vec4(1.0f);
uniform vec3 viewPosition;
uniform DirectionalLight directionalLight;
uniform PointLight pointLights[TOTAL_POINT_LIGHTS];
uniform SpotLight spotLight;
uniform Material material;
uniform sampler2D objectTexture;
uniform vec2 UVscale = vec2(1.0f, 1.0f);

// Function prototypes
vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    if (bUseLighting)
    {
        vec3 norm = normalize(fragmentVertexNormal);
        vec3 viewDir = normalize(viewPosition - fragmentPosition);

        vec3 lightingResult = vec3(0.0f);

        if (directionalLight.bActive)
            lightingResult += CalcDirectionalLight(directionalLight, norm, viewDir);

        for (int i = 0; i < TOTAL_POINT_LIGHTS; i++)
            if (pointLights[i].bActive)
                lightingResult += CalcPointLight(pointLights[i], norm, fragmentPosition, viewDir);

        if (spotLight.bActive)
            lightingResult += CalcSpotLight(spotLight, norm, fragmentPosition, viewDir);

        if (bUseTexture)
        {
            vec4 texColor = texture(objectTexture, fragmentTextureCoordinate * UVscale);
            fragmentColor = vec4(lightingResult, texColor.a);
        }
        else
        {
            fragmentColor = vec4(lightingResult, objectColor.a);
        }
    }
    else
    {
        if (bUseTexture)
            fragmentColor = texture(objectTexture, fragmentTextureCoordinate * UVscale);
        else
            fragmentColor = objectColor;
    }
}

// ----------------------------
// Blinn-Phong Directional Light
// ----------------------------
vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 halfVector = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfVector), 0.0), material.shininess);

    vec3 baseColor = bUseTexture
        ? vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale))
        : vec3(objectColor);

    vec3 ambient  = light.ambient  * baseColor;
    vec3 diffuse  = light.diffuse  * diff * material.diffuseColor * baseColor;
    vec3 specular = light.specular * spec * material.specularColor;

    return ambient + diffuse + specular;
}

// ----------------------------
// Blinn-Phong Point Light
// ----------------------------
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 halfVector = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfVector), 0.0), material.shininess);

    vec3 baseColor = bUseTexture
        ? vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale))
        : vec3(objectColor);

    vec3 ambient  = light.ambient  * baseColor;
    vec3 diffuse  = light.diffuse  * diff * material.diffuseColor * baseColor;
    vec3 specular = light.specular * spec * material.specularColor;

    return ambient + diffuse + specular;
}

// ----------------------------
// Blinn-Phong Spot Light
// ----------------------------
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    float diff = max(dot(normal, lightDir), 0.0);

    vec3 halfVector = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfVector), 0.0), material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant +
                               light.linear * distance +
                               light.quadratic * distance * distance);

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 baseColor = bUseTexture
        ? vec3(texture(objectTexture, fragmentTextureCoordinate * UVscale))
        : vec3(objectColor);

    vec3 ambient  = light.ambient  * baseColor;
    vec3 diffuse  = light.diffuse  * diff * material.diffuseColor * baseColor;
    vec3 specular = light.specular * spec * material.specularColor;

    ambient  *= attenuation * intensity;
    diffuse  *= attenuation * intensity;
    specular *= attenuation * intensity;

    return ambient + diffuse + specular;
}
