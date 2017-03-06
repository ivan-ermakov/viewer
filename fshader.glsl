#version 450

uniform mat4 m_model_view;

uniform vec3 lightPos;
uniform vec4 lightColor;
uniform vec4 modelColor;

in vec3 fragNormal;
in vec3 fragVert;

out vec4 finalColor;

void main()
{
    mat3 normalMatrix = transpose(inverse(mat3(m_model_view)));
    vec3 normal = normalize(normalMatrix * fragNormal);

    //calculate the location of this fragment (pixel) in world coordinates
    vec3 fragPosition = vec3(m_model_view * vec4(fragVert, 1));

    //calculate the vector from this pixels surface to the light source
    vec3 surfaceToLight = lightPos - fragPosition;

    //calculate the cosine of the angle of incidence
    float brightness = dot(normal, surfaceToLight) / (length(surfaceToLight) * length(normal));
    brightness = abs(brightness);
    //brightness = clamp(brightness, 0., 1.);

    //calculate final color of the pixel, based on:
    // 1. The angle of incidence: brightness
    // 2. The color/intensities of the light: light.intensities
    // 3. The texture and texture coord: texture(tex, fragTexCoord)
    vec4 surfaceColor = modelColor; //texture(tex, fragTexCoord);
    //finalColor = vec4(brightness * lightColor * surfaceColor);
    //finalColor.a = 1.;
    finalColor = vec4(vec3(brightness * lightColor * surfaceColor), 1.);
    //finalColor = vec4(1.0, 1.0, 1.0, 1.0);
}
