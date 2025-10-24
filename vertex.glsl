#version 330 core
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

uniform mat4 MVP;
uniform mat4 model;

out vec3 vNormal;
out vec2 vUV;
out vec3 vWorldPos;

void main() {
    vec4 worldPos = model * vec4(inPos, 1.0);
    vWorldPos = worldPos.xyz;
    // корректная трансформация нормали
    mat3 normalMat = mat3(transpose(inverse(model)));
    vNormal = normalize(normalMat * inNormal);
    vUV = inUV;
    gl_Position = MVP * vec4(inPos, 1.0);
}
