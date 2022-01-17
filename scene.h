
#ifndef  _scene_
#define  _scene_

#include <iostream>
#include <vector>

#include "functional_mesh.h"
#include "light_source.h"
#include "view_ruling.h"

#include "Shaders/vao_managment.h"
#include "Shaders/shader_programm.h"


class CMeshShaderData
{
    public:
    struct levels_t
    {
        CVao                 m_vao;
        std::vector<CBuffer> m_lines;
        void clear(){m_lines.clear();}
        CBuffer& push_back()
        {
            m_lines.push_back(CBuffer(float{0}));
            return m_lines.back();
        }
        auto Size()const{return m_lines.size();}
        CBuffer&Buffer(int i){ return m_lines[i];}
        void DrawAll()
        {
            for(auto&line:m_lines)
            {
                m_vao.EnableLayout(0,line,CBufferFormat::Solid(3));
                m_vao.DrawArraw(CVao::lines,0 ,line.Size());
            }
        }
        void Swap(levels_t&other)
        {
            m_vao.Swap(other.m_vao);
            m_lines.swap(other.m_lines);
        }
    };
    private:
    CBuffer      m_box_vertex_buffer;
    CBuffer      m_vertex_buffer;
    CBuffer      m_color_buffer;
    CBuffer      m_normals_buffer;
    CIndexBuffer m_edges;
    CIndexBuffer m_triangles;

    levels_t     m_levels[3];
    CVao         m_vao[4];
    CVao         m_box_vao;

    public:
    enum use_enum{use_vertex=1<<0,use_color=1<<1,use_normal=1<<2};

    CMeshShaderData();
    void   Swap(CMeshShaderData&other);

    CBuffer&Vertex(){return m_vertex_buffer;}
    CBuffer&BoxVertex(){return m_box_vertex_buffer;}
    CBuffer&Colors(){return m_color_buffer;}
    CBuffer&Normals(){return m_normals_buffer;}
    CIndexBuffer&Edges(){return m_edges;}
    CIndexBuffer&Trians(){return m_triangles;}
    levels_t&    Levels(int i){return m_levels[i];}

    void  DrawEdges(int)const;
    void  DrawTrians(int)const;
    void  DrawBox()const;
};

class CScene
{
    using point_t=Eigen::Vector3f;

    mutable std::vector<float>    m_floats_cashe;
    mutable std::vector<unsigned> m_ints_cashe;
    mutable std::vector<unsigned> m_meshes_cashe;

    std::vector<CFunctionalMesh*>   m_meshes;
    mutable std::vector<CMeshShaderData>    m_shader_data;
    CLightSource m_light_source;
    CMoveableCamera m_camera;

    // Shaders programms
    CShaderProgramm m_vert;
    CShaderProgramm m_colored_prog;
    CShaderProgramm m_specular_prog;
    CShaderProgramm m_specular_colored_prog;

    CShaderProgramm m_fong_shading;
    CShaderProgramm m_colored_fong_shading;
    CShaderProgramm*m_actual_specular;
    CShaderProgramm*m_actual_colored_specular;

    void m_UpdateMeshData(const CFunctionalMesh&mesh,size_t index,CFunctionalMesh::CUpdateResult up_result);
    void m_RenderMesh(const CFunctionalMesh&mesh,CMeshShaderData&data,const Eigen::Matrix4f&full_mtx)const;

    public:
    CScene();
    CScene(const CScene&)=delete;
    CScene&operator=(const CScene&)=delete;
    auto&Camera(){return m_camera;}
    bool AddMesh(CFunctionalMesh&m);
    bool RemoveMesh(CFunctionalMesh&m);
    bool IsMesh(const CFunctionalMesh&m)const;
    auto Meshes()const{return m_meshes.size();}
    CLightSource&LightSource(){return m_light_source;}
    const CLightSource&LightSource()const{return m_light_source;}
    void  LegacyRender(float t)const;
    void  Render(float t)const;
    void SetFongShading(bool);
    bool IsFongShading()const{return m_actual_specular==&m_fong_shading;}

    bool Empty()const{return m_meshes.empty();}
    void Clear();
};


#endif

