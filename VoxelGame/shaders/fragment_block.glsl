#version 330 core

out vec4 FragColor;
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform sampler2D ourTexture;

void main()
{
    vec3 lightPos = vec3(-500.0, 1000.0, 1000.0);
    //vec3 lightDir = normalize(vec3(-1.0, 1.0, 1.0));
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(Normal, lightDir), 0.0);
    vec3 diffuse = vec3(diff * 0.7);
    vec3 ambient = vec3(0.3);
    vec4 fragment4 = texture(ourTexture, TexCoord);
    vec3 fragment3 = vec3(fragment4);
    vec3 result = (ambient + diffuse) * fragment3;
    FragColor = vec4(result, fragment4.a);
}
