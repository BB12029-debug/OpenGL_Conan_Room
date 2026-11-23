#version 460 core
out vec4 FragColor;

in vec2 texCoord;

uniform vec3 objectColor;
uniform sampler2D texture_diffuse1;
uniform bool useDiscard;

void main()
{
    vec4 texColor = texture(texture_diffuse1, texCoord);
    
    // حذف السواد الخارجي الصافي فقط (0.015f) لمنع أي ثقوب داخل شعر كونان والكتب
    if (useDiscard && texColor.r < 0.015f && texColor.g < 0.015f && texColor.b < 0.015f)
        discard;
        
    FragColor = texColor * vec4(objectColor, 1.0);
}