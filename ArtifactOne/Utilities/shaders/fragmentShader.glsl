#version 330 core
out vec4 outFragmentColor;

in vec3 fragmentPosition;
in vec3 fragmentVertexNormal;
in vec2 fragmentTextureCoordinate;

uniform vec4 objectColor;
uniform sampler2D objectTexture;
uniform bool bUseTexture;
uniform bool bUseLighting;
uniform bool bUseBlinnPhong;
uniform float UVscale;

struct Light {
    bool bIsActive;
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
};

uniform Light light[10];
uniform int numLights;
uniform vec3 viewPosition;

void main()
{
    vec4 baseColor;

    if (bUseTexture)
        baseColor = texture(objectTexture, fragmentTextureCoordinate * UVscale);
    else
        baseColor = objectColor;

    vec3 lighting = vec3(0.0);

    if (bUseLighting)
    {
        for (int i = 0; i < numLights; i++)
        {
            if (!light[i].bIsActive)
                continue;

            vec3 lightDir = normalize(light[i].direction);
            float diff = max(dot(fragmentVertexNormal, lightDir), 0.0);

            vec3 diffuse = diff * light[i].color * light[i].intensity;

            vec3 viewDir = normalize(viewPosition - fragmentPosition);
            vec3 reflectDir = reflect(-lightDir, fragmentVertexNormal);

            float spec = 0.0;
            if (bUseBlinnPhong)
            {
                vec3 halfwayDir = normalize(lightDir + viewDir);
                spec = pow(max(dot(fragmentVertexNormal, halfwayDir), 0.0), 32.0);
            }
            else
            {
                spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
            }

            vec3 specular = spec * light[i].color * light[i].intensity;

            lighting += diffuse + specular;
        }
    }
    else
    {
        lighting = vec3(1.0);
    }

    vec3 finalColor = baseColor.rgb * lighting;
    outFragmentColor = vec4(finalColor, baseColor.a);
}
