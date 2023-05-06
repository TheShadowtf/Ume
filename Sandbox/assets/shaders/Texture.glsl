#type vertex
#version 330 core

layout(location = 0) in vec2 a_TexCoord;
layout(location = 1) in vec3 a_Position;
layout(location = 2) in vec4 a_Color;
layout(location = 3) in float a_TexIndex;
layout(location = 4) in float a_TexScale;

uniform mat4 u_ViewProjection;

out vec2 v_TexCoord;
out vec4 v_Color;
out float v_TexIndex;
out float v_TexScale;
			
void main()
{
	v_Color = a_Color;
	v_TexCoord = a_TexCoord;
	v_TexIndex = a_TexIndex;
	v_TexScale = a_TexScale;
	gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
}

#type fragment
#version 330 core
			
layout(location = 0) out vec4 color;

in vec2 v_TexCoord;
in vec4 v_Color;
in float v_TexIndex;
in float v_TexScale;

uniform sampler2D u_Textures[32];

void main()
{
	// TODO: u_TexScale
	color = texture(u_Textures[int(v_TexIndex)], v_TexCoord * v_TexScale) * v_Color;
	// color = vec4(a_TexScale, 0.0f, 0.0f, 1.0f);
}
