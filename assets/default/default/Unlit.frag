#ifdef GLES2
	precision mediump float;
#endif

uniform sampler2D diffuse;
uniform vec4 entityColor;
varying vec2 texCoordVar;

void main() {
	gl_FragColor = texture2D(diffuse, texCoordVar) * entityColor;
}