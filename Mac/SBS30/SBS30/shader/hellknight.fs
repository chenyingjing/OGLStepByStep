#version 330

uniform sampler2D materialTex;
//in vec2 fragTexCoord;
in vec2 TexCoord;
out vec4 FragColor;

void main() {
    FragColor = texture(materialTex, TexCoord);

    if (FragColor.r == 0 && FragColor.g == 0 && FragColor.b == 0) {
        discard;                                                                    
    }
}
