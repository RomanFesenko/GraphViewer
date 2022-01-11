
#ifndef _shaders_source_
#define _shaders_source_

///////////////////////////////////////////////////
//           Vertex, colored in iuniform color
///////////////////////////////////////////////////

const char* v_vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 vertex_org;

uniform  vec3 color;
uniform mat4 full_matrix=mat4(vec4(1.0, 0.0, 0.0, 0.0),
                              vec4(0.0, 1.0, 0.0, 0.0),
                              vec4(0.0, 0.0, 0.1, 0.0),
                              vec4(0.0, 0.0, 0.0, 1.0));
out vec3 out_color;



void main()
{
   out_color=color;
   gl_Position = full_matrix*vec4(vertex_org,1.0);
};
)";


//////////////////////////////////////////////////////
// Colored vertex
/////////////////////////////////////////////////////

const char* vc_vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 vertex_org;
layout (location = 1) in vec3 vertex_color;

out vec3 out_color;


uniform mat4 full_matrix=mat4(vec4(1.0, 0.0, 0.0, 0.0),
                              vec4(0.0, 1.0, 0.0, 0.0),
                              vec4(0.0, 0.0, 0.1, 0.0),
                              vec4(0.0, 0.0, 0.0, 1.0));



void main()
{
   gl_Position = full_matrix*vec4(vertex_org,1.0);
   out_color=vertex_color;
};
)";


//////////////////////////////////////////////////////
//      Reflect surface
/////////////////////////////////////////////////////

const char* specular_shader = R"(
#version 330 core
layout (location = 0) in vec3 vertex_org;
layout (location = 2) in vec3 vertex_normals;

struct CLightSource
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct material_t
{
    float ambient;
    float diffuse;
    float specular;
    float shininess;
};

uniform CLightSource light_source;
uniform material_t   material;
uniform int two_side_specular=0;
uniform vec3 view_org;

uniform mat4 model_matrix4;
uniform mat3 model_matrix3;
uniform mat4 full_matrix=mat4(vec4(1.0, 0.0, 0.0, 0.0),
                              vec4(0.0, 1.0, 0.0, 0.0),
                              vec4(0.0, 0.0, 0.1, 0.0),
                              vec4(0.0, 0.0, 0.0, 1.0));

out vec3 out_color;


void main()
{
    vec4 vertex4=vec4(vertex_org,1.0);
    vec3 transform_normal=model_matrix3*vertex_normals;
    vec4 transform_org=model_matrix4*vertex4;
    vec3 S=normalize(light_source.position-transform_org.xyz);
    vec3 color=light_source.ambient*material.ambient;

    if(two_side_specular==0)
    {
        float S_N=dot(S,transform_normal);
        if(S_N>0.0)
        {
            color+=light_source.diffuse*material.diffuse*S_N;
            float V_N=dot(reflect(-S,transform_normal),normalize(view_org-transform_org.xyz));
            color+=pow(max(V_N,0.0),material.shininess)*light_source.specular*material.specular;
        }

    }
    else
    {
        float S_N=dot(S,transform_normal);
        if(dot(view_org-transform_org.xyz,transform_normal)>0.0 == S_N>0.0)
        {
            float V_N=dot(reflect(-S,transform_normal),normalize(view_org-transform_org.xyz));
            color+=light_source.diffuse*material.diffuse*abs(S_N)+
                   pow(max(V_N,0.0),material.shininess)*light_source.specular*material.specular;
        }
    }

    out_color=color;
    gl_Position = full_matrix*vertex4;
};
)";

//////////////////////////////////////////////////////
//      Reflect colored surface
/////////////////////////////////////////////////////

const char* specular_colored_shader = R"(
#version 330 core
layout (location = 0) in vec3 vertex_org;
layout (location = 1) in vec3 vertex_colors;
layout (location = 2) in vec3 vertex_normals;

struct CLightSource
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform CLightSource light_source;
uniform float        shininess;
uniform int two_side_specular=0;
uniform vec3 view_org;


uniform mat4 model_matrix4;
uniform mat3 model_matrix3;
uniform mat4 full_matrix=mat4(vec4(1.0, 0.0, 0.0, 0.0),
                              vec4(0.0, 1.0, 0.0, 0.0),
                              vec4(0.0, 0.0, 0.1, 0.0),
                              vec4(0.0, 0.0, 0.0, 1.0));

out vec3 out_color;


void main()
{
    vec4 vertex4=vec4(vertex_org,1.0);
    vec3 transform_normal=model_matrix3*vertex_normals;
    vec4 transform_org=model_matrix4*vertex4;
    vec3 S=normalize(light_source.position-transform_org.xyz);
    vec3 color=light_source.ambient*vertex_colors;

    if(two_side_specular==0)
    {
        float S_N=dot(S,transform_normal);
        if(S_N>0.0)
        {
            color+=light_source.diffuse*vertex_colors*S_N;
            float V_N=dot(reflect(-S,transform_normal),normalize(view_org-transform_org.xyz));
            color+=pow(max(V_N,0.0),shininess)*light_source.specular*vertex_colors;
        }

    }
    else
    {
        float S_N=dot(S,transform_normal);
        if(dot(view_org-transform_org.xyz,transform_normal)>0.0 == S_N>0.0)
        {
            float V_N=dot(reflect(-S,transform_normal),normalize(view_org-transform_org.xyz));
            color+=light_source.diffuse*vertex_colors*abs(S_N)+
                   pow(max(V_N,0.0),shininess)*light_source.specular*vertex_colors;
        }
    }


    out_color=color;
    gl_Position = full_matrix*vertex4;
};
)";



//////////////////////////////////////////////
//  Base fragment shader
/////////////////////////////////////////////

const char* fragment_shader = R"(
#version 330 core
in vec3 out_color;
out vec4 frag_out_color;

void main() {

	frag_out_color=vec4(out_color,1.0);
}
)";

/** Fong shading */


/*
const char* fong_vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 vertex_org;
layout (location = 2) in vec3 vertex_normals;



uniform mat4 model_matrix4;
uniform mat3 model_matrix3;
uniform mat4 full_matrix=mat4(vec4(1.0, 0.0, 0.0, 0.0),
                              vec4(0.0, 1.0, 0.0, 0.0),
                              vec4(0.0, 0.0, 0.1, 0.0),
                              vec4(0.0, 0.0, 0.0, 1.0));

out vec3 out_transform_normal;
out vec3 out_transform_vertex;



void main()
{
    out_transform_normal=model_matrix3*vertex_normals;
    out_transform_vertex=(model_matrix4*vec4(vertex_org,1.0)).xyz;
    gl_Position = full_matrix*vec4(vertex_org,1.0);
};
)";

const char* fong_colored_vertex_shader = R"(
#version 330 core
layout (location = 0) in vec3 vertex_org;
layout (location = 1) in vec3 vertex_color;
layout (location = 2) in vec3 vertex_normals;



uniform mat4 model_matrix4;
uniform mat3 model_matrix3;
uniform mat4 full_matrix=mat4(vec4(1.0, 0.0, 0.0, 0.0),
                              vec4(0.0, 1.0, 0.0, 0.0),
                              vec4(0.0, 0.0, 0.1, 0.0),
                              vec4(0.0, 0.0, 0.0, 1.0));

out vec3 out_transform_normal;
out vec3 out_transform_vertex;
out vec3 out_color;



void main()
{
    out_transform_normal=model_matrix3*vertex_normals;
    out_transform_vertex=(model_matrix4*vec4(vertex_org,1.0)).xyz;
    gl_Position = full_matrix*vec4(vertex_org,1.0);
    out_color=vertex_color;
};
)";



const char* fong_fragment_shader = R"(
#version 330 core


struct CLightSource
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct material_t
{
    float ambient;
    float diffuse;
    float specular;
    float shininess;
};

uniform CLightSource light_source;
uniform material_t   material;
uniform int two_side_specular=0;
uniform vec3 view_org;

in vec3 out_transform_normal;
in vec3 out_transform_vertex;


void main()
{
    vec3 S=normalize(light_source.position-out_transform_vertex);
    vec3 color=light_source.ambient*material.ambient;

    if(two_side_specular==0)
    {
        float S_N=dot(S,out_transform_normal);
        if(S_N>0.0)
        {
            color+=light_source.diffuse*material.diffuse*S_N;
            float V_N=dot(reflect(-S,out_transform_normal),normalize(view_org-out_transform_vertex));
            color+=pow(max(V_N,0.0),material.shininess)*light_source.specular*material.specular;
        }

    }
    else
    {
        float S_N=dot(S,out_transform_normal);
        if(dot(view_org-out_transform_vertex,out_transform_normal)>0.0 == S_N>0.0)
        {
            float V_N=dot(reflect(-S,out_transform_normal),normalize(view_org-out_transform_vertex));
            color+=light_source.diffuse*material.diffuse*abs(S_N)+
                   pow(max(V_N,0.0),material.shininess)*light_source.specular*material.specular;
        }
    }

    gl_FragColor=vec4(color,1.0);
};
)";


const char* fong_colored_fragment_shader = R"(
#version 330 core

struct CLightSource
{
    vec3 position;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform CLightSource light_source;
uniform float        shininess;
uniform int two_side_specular=0;
uniform vec3 view_org;


in vec3 out_transform_vertex;
in vec3 out_color;
in vec3 out_transform_normal;

void main()
{
    vec3 S=normalize(light_source.position-out_transform_vertex);
    vec3 color=light_source.ambient*out_color;

    if(two_side_specular==0)
    {
        float S_N=dot(S,out_transform_normal);
        if(S_N>0.0)
        {
            color+=light_source.diffuse*out_color*S_N;
            float V_N=dot(reflect(-S,out_transform_normal),normalize(view_org-out_transform_vertex));
            color+=pow(max(V_N,0.0),shininess)*light_source.specular*out_color;
        }

    }
    else
    {
        float S_N=dot(S,out_transform_normal);
        if(dot(view_org-out_transform_vertex,out_transform_normal)>0.0 == S_N>0.0)
        {
            float V_N=dot(reflect(-S,out_transform_normal),normalize(view_org-out_transform_vertex));
            color+=light_source.diffuse*out_color*abs(S_N)+
                   pow(max(V_N,0.0),shininess)*light_source.specular*out_color;
        }
    }


    gl_FragColor=vec4(color,1.0);
};
)";
*/




















#endif
