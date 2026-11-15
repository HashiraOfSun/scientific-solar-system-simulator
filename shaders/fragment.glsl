#version 410 core

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

out vec4 FragColor;

uniform sampler2D planetTexture;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform bool isSun;

void main()
{
    vec3 texColor = texture(planetTexture, TexCoord).rgb;

    if (isSun)
    {
        FragColor = vec4(texColor, 1.0);
        return;
    }

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float ambientStrength = 0.12;
    vec3 ambient = ambientStrength * texColor;

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * texColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 16.0);
    vec3 specular = 0.08 * spec * vec3(1.0);

    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0);
}