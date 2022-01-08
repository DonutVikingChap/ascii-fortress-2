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
const vec3 colorTint = vec3(0.9, 1.0, 0.8);

void main() {
	vec2 position = (offset + instanceOffset + inCoordinates * instanceScale) * scale;
	float r = length(position);
	float w = 1.0 - warpedness + r * warpedness;
	vec3 color = colorTint;
	if (instanceColor.r > 0.9 && instanceColor.g > 0.9 && instanceColor.b > 0.9) {
		color = vec3(0.3, 0.7, 0.3);
	}
	ioTextureCoordinates = instanceTextureOffset + inCoordinates * instanceTextureScale;
	ioColor = vec4(instanceColor.rgb * color, instanceColor.a);
	gl_Position = vec4(position, 0.0, w);
}
