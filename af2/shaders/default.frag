in vec2 ioTextureCoordinates;
in vec4 ioColor;

out vec4 outFragmentColor;

uniform sampler2D atlasTexture;

void main() {
	outFragmentColor = vec4(ioColor.rgb, ioColor.a * texture(atlasTexture, ioTextureCoordinates).r);
}
