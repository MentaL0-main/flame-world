#version 330 core

in vec3 vNormal;
in vec2 vUV;
in vec3 vWorldPos;

uniform sampler2D uAlbedo;
uniform int uUseTex;        // 0 = use uColor, 1 = use texture
uniform vec3 uColor;        // если нет текстуры
uniform vec3 uLightDir;     // направление света (в мировых координатах)
uniform vec3 uAmbient;      // ambient

out vec4 fragColor;

void main(){
    vec3 N = normalize(vNormal);
    vec3 L = normalize(-uLightDir);
    float diff = max(dot(N, L), 0.0);

    vec3 baseCol = (uUseTex == 1) ? texture(uAlbedo, vUV).rgb : uColor;
    vec3 col = uAmbient * baseCol + diff * baseCol;
    fragColor = vec4(col, 1.0);
}
