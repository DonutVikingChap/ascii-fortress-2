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

const float wavelength = 0.1;
const float frequency = 5.0;
const float amplitude = 6.0;

const float pi = 3.14159;
const float redPhase = 2.0 * pi * 0.0 / 3.0;
const float greenPhase = 2.0 * pi * 1.0 / 3.0;
const float bluePhase = 2.0 * pi * 2.0 / 3.0;

void main() {
	float phase = 1.0 / wavelength * instanceOffset.x * scale.x;
	float t = frequency * time;
	vec3 waveColor = vec3(
		0.7 + 0.3 * sin(phase + redPhase + t),
		0.7 + 0.3 * sin(phase + greenPhase + t),
		0.7 + 0.3 * sin(phase + bluePhase + t)
	);
	vec2 waveOffset = vec2(0.0, amplitude * sin(phase + t));

	ioTextureCoordinates = instanceTextureOffset + inCoordinates * instanceTextureScale;
	ioColor = vec4(instanceColor.rgb * waveColor, instanceColor.a);
	gl_Position = vec4((waveOffset + offset + instanceOffset + inCoordinates * instanceScale) * scale, 0.0, 1.0);
}
