attribute vec3 position;
attribute vec2 texCoord;
attribute vec3 normal;
uniform mat4 pvm;

varying vec2 uv;
varying vec3 modelNormal;

void main() {
	gl_Position = pvm * vec4(position, 1.0);
	uv = texCoord;
	modelNormal = normal;
}