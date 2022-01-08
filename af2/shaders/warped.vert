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

const float warpedness = 0.05;

void main() {
	vec2 position = (offset + instanceOffset + inCoordinates * instanceScale) * scale;
	float r = length(position);
	float w = 1.0 - warpedness + r * warpedness;
	ioTextureCoordinates = instanceTextureOffset + inCoordinates * instanceTextureScale;
	ioColor = instanceColor;
	gl_Position = vec4(position, 0.0, w);
}
