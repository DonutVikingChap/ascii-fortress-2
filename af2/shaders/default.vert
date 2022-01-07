layout(location = 0) in vec2 inCoordinates;
layout(location = 1) in vec2 instanceOffset;
layout(location = 2) in vec2 instanceScale;
layout(location = 3) in vec2 instanceTextureOffset;
layout(location = 4) in vec2 instanceTextureScale;
layout(location = 5) in vec4 instanceColor;

out vec2 ioTextureCoordinates;
out vec4 ioColor;

uniform vec2 offset;
uniform vec2 scale;
uniform float time;

void main() {
	ioTextureCoordinates = instanceTextureOffset + inCoordinates * instanceTextureScale;
	ioColor = instanceColor;
	gl_Position = vec4((offset + instanceOffset + inCoordinates * instanceScale) * scale, 0.0, 1.0);
}
