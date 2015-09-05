#ifdef GLES2
	precision mediump float;
#endif

uniform vec4 entityColor;
varying vec4 varColor;

void main() {
	gl_FragColor = varColor * entityColor;
}