//This shader is for triangles/lines
@vs debugGeometryVS
uniform vsParams {
	mat4 mvp;
};
in vec2 position;
in vec4 color0;
out vec4 color;
void main() {
    gl_Position = mvp * vec4(position,0,1);
    color = color0;
}
@end

@fs debugGeometryFS
in vec4 color;
out vec4 fragColor;
void main() {
    fragColor = color;
}
@end

@vs debugPointVS
uniform vsParams {
	mat4 mvp;
};
in vec2 texcoord0;
in vec3 instance0;
in vec4 instance1;
out vec4 color;
void main() {
	float size = instance0.z;
	vec2 position = instance0.xy;
    gl_Position = mvp * vec4((position+texcoord0.xy*size),0,1);
    color = instance1;
}
@end

@vs texturedGeometryVS
uniform vsParams {
	mat4 mvp;
};
in vec2 position;
in vec2 texcoord0;
in vec4 color0;
out vec2 uv;
out vec4 color;
void main(){
	gl_Position = mvp * vec4(position,0,1);
	uv = texcoord0;
	color = color0;
}
@end
@fs texturedGeometryFS
uniform sampler2D tex;
in vec2 uv;
in vec4 color;
out vec4 fragColor;
void main() {
    fragColor = texture(tex, uv)*color;
}
@end

@program DebugGeometryShader debugGeometryVS debugGeometryFS
@program DebugPointShader debugPointVS debugGeometryFS
@program SpriteShader texturedGeometryVS texturedGeometryFS