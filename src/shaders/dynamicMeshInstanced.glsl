
#shader VERTEX


#version 430 core

//layout(std140) uniform; 

// per-vertex attributes
layout (location = 0) in vec3 v_pos_in;
layout (location = 1) in vec3 v_norm_in;
layout (location = 2) in vec2 v_tex_in;


// per-instance attributes
// layout (location = 3) in vec4 i_pos_scale_in;
// layout (location = 4) in vec4 i_dir_rot_in;
// layout (location = 5) in vec4 i_alpha_in;

layout (location = 3) in mat4 i_mat_in;

uniform mat4 mModel;
uniform mat4 mView;
uniform mat4 mProj;

// out vec4 vs_pos;
out vec3 vs_norm;
out vec2 vs_tex;
out float vs_alpha;


void main() {
	vec4 pos = vec4(v_pos_in, 1.0);
//	pos *= rotationMatrix(i_dir_rot_in.xyz, i_dir_rot_in.w);
//	pos *= vec4(i_pos_scale_in.www, 1);
//	pos += vec4(i_pos_scale_in.xyz, 0);
	
	gl_Position = (mProj * mView * i_mat_in) * (pos);// * i_scale_in;
	vs_norm = v_norm_in; // normalize(vec4(1,1,1,0));
	vs_tex = v_tex_in;
	vs_alpha = 1.0;//i_alpha_in.x;
}




#shader FRAGMENT

#version 400

// in vec4 vs_pos;
in vec3 vs_norm;
in vec2 vs_tex;
in float vs_alpha;

// fragment shader
uniform vec4 color;
uniform sampler2D sTexture;

layout(location = 0) out vec4 out_Color;
layout(location = 1) out vec4 out_Normal;


void main(void) {
	
	out_Color = texture(sTexture, vec2(vs_tex.x, 1-vs_tex.y)) * vec4(1,1,1, vs_alpha);//vs_norm;
	out_Normal = vec4((normalize(vs_norm.xzy) * .5) + .5, 1);
}
