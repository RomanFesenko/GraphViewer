
#include <iostream>
#include <algorithm>
#include <memory>

#include "opengl_iface.h"
#include "legacy_render.h"
#include "scene.h"

#include "Shaders/shaders_source.h"

static void MakeContiniousBuffer(const CFunctionalMesh::matrix_t&mtx,std::vector<float>&data)
{
    data.resize(mtx.size()*3);
    decltype(mtx.size()) index=0;
    for(int i=0;i<mtx.cols();++i)
    {
        for(int j=0;j<mtx.rows();++j)
        {
            for(int k=0;k<3;++k)
            {
                data[index++]=mtx(j,i)[k];
            }

        }
    }
    assert(index==mtx.size()*3);
}

static void MakeEigesIndexes(int rows,int cols,std::vector<unsigned>&data)
{
    data.resize(2*rows*cols);
    int index=0;
    int pos=0;
    for(int i=0;i<cols;++i)// by cols
    {
        if(i&1)
        {
            for(int j=0;j<rows;++j)
            {
                data[pos++]=index--;
            }
            index+=rows+1;
        }
        else
        {
            for(int j=0;j<rows;++j)
            {
                data[pos++]=index++;
            }
            index+=rows-1;
        }
    }
    int delta;
    if(cols&1)
    {
        delta=-1;
        index=rows*cols-1;
    }
    else
    {
        delta=1;
        index=rows*(cols-1);
    }
    for(int i=0;i<rows;++i)// by rows
    {
        if(i&1)
        {
            for(int j=0;j<cols;++j)
            {
                data[pos++]=index;
                index+=rows;
            }
            index+=delta-rows;
        }
        else
        {
            for(int j=0;j<cols;++j)
            {
                data[pos++]=index;
                index-=rows;
            }
            index+=delta+rows;
        }
    }
    assert(pos==2*rows*cols);
}

static void MakeTriansIndexes(int rows,int cols,std::vector<unsigned>&data)
{
    data.resize((rows-1)*(cols-1)*6);

    int pos=0;
    for(int i=0;i<rows-1;++i)
    {
        for(int j=0;j<cols-1;++j)
        {
            const int index=i+j*rows;
            data[pos++]=index;
            data[pos++]=index+1;
            data[pos++]=index+1+rows;

            data[pos++]=index+1+rows;
            data[pos++]=index+rows;
            data[pos++]=index;
        }
    }
    assert(pos==(rows-1)*(cols-1)*6);
}

static void MakeBoxEdge(const Eigen::Vector3f&min,const Eigen::Vector3f&max,std::vector<float>&data)
{
    data.resize(72);
    typename std::vector<float>::size_type pos=0;

    for(int i=0;i<4;++i)
    {
        Eigen::Vector3f pivot=max;
        if(i==3)
        {
            pivot=min;
        }
        else
        {
            pivot[i]=min[i];
        }
        for(int j=0;j<3;++j)
        {
            for(int k=0;k<3;++k) data[pos++]=pivot[k];
            Eigen::Vector3f temp=pivot;
            temp[j]=(temp[j]==max[j])? min[j]:max[j];
            for(int k=0;k<3;++k) data[pos++]=temp[k];
        }
    }
}

static void MakeLevelsBuffer(int dir_index,
                             const std::vector<std::pair<Eigen::Vector3f,Eigen::Vector3f>>&lines,
                             std::vector<float>&data)
{

    data.resize(3*2*lines.size());
    typename std::vector<float>::size_type pos=0;
    for(auto&[_1,_2]:lines)
    {
        data[pos++]=_1[0];
        data[pos++]=_1[1];
        data[pos++]=_1[2];

        data[pos++]=_2[0];
        data[pos++]=_2[1];
        data[pos++]=_2[2];
    }
    assert(pos==3*2*lines.size());
}

///////////////////////////////////////////////////////
//                      CMeshShaderData
///////////////////////////////////////////////////////


CMeshShaderData::CMeshShaderData():
m_box_vertex_buffer(float{0}),
m_vertex_buffer(float{0}),
m_color_buffer(float{0}),
m_normals_buffer(float{0}),
m_edges(unsigned{0}),
m_triangles(unsigned{0})
{
    glCheckError();
    m_vao[0].EnableLayout(0,m_vertex_buffer,CBufferFormat::Solid(3));

    m_vao[1].EnableLayout(0,m_vertex_buffer,CBufferFormat::Solid(3)).
             EnableLayout(1,m_color_buffer,CBufferFormat::Solid(3));

    m_vao[2].EnableLayout(0,m_vertex_buffer,CBufferFormat::Solid(3)).
             EnableLayout(2,m_normals_buffer,CBufferFormat::Solid(3));

    m_vao[3].EnableLayout(0,m_vertex_buffer,CBufferFormat::Solid(3)).
             EnableLayout(1,m_color_buffer,CBufferFormat::Solid(3)).
             EnableLayout(2,m_normals_buffer,CBufferFormat::Solid(3));


    m_box_vao.EnableLayout(0,m_box_vertex_buffer,CBufferFormat::Solid(3));
}

void  CMeshShaderData::Swap(CMeshShaderData&other)
{
    for(int i=0;i<4;++i) m_vao[i].Swap(other.m_vao[i]);

    m_box_vao.Swap(other.m_box_vao);
    m_box_vertex_buffer.Swap(other.m_box_vertex_buffer);

    m_edges.Swap(other.m_edges);
    m_triangles.Swap(other.m_triangles);

    for(int i=0;i<3;++i) m_levels[i].Swap(other.m_levels[i]);
}

void  CMeshShaderData::DrawEdges(int _i)const
{
    int i=_i/2;
    assert(i>=0 && i<4 && (_i&1) );

    m_vao[i].DrawElement(m_edges,CVao::line_strip);
}


void  CMeshShaderData::DrawTrians(int _i)const
{
    int i=_i/2;
    assert(i>=0 && i<4 && (_i&1) );

    m_vao[i].DrawElement(m_triangles,CVao::triangles);
}


void  CMeshShaderData::DrawBox()const
{
    m_box_vao.DrawArraw(CVao::lines,0 ,m_box_vertex_buffer.Size());
}

///////////////////////////////////////////////////////
//                      CScene
///////////////////////////////////////////////////////

CScene::CScene()
{
    auto v_frag=CShader::Compile(CShader::fragment,fragment_shader);

    m_vert=CShaderProgramm::Link(CShader::Compile(CShader::vertex,v_vertex_shader),v_frag);
    m_colored_prog=CShaderProgramm::Link(CShader::Compile(CShader::vertex,vc_vertex_shader),v_frag);
    m_specular_prog=CShaderProgramm::Link(CShader::Compile(CShader::vertex,specular_shader),v_frag);
    m_specular_colored_prog=CShaderProgramm::Link(CShader::Compile(CShader::vertex,specular_colored_shader),v_frag);



    m_fong_shading=CShaderProgramm::Link(CShader::Compile(CShader::vertex,fong_vertex_shader),
                                         CShader::Compile(CShader::fragment,fong_fragment_shader));

    m_colored_fong_shading=CShaderProgramm::Link(CShader::Compile(CShader::vertex,fong_colored_vertex_shader),
                                                 CShader::Compile(CShader::fragment,fong_colored_fragment_shader));

    m_actual_specular=&m_fong_shading;
    m_actual_colored_specular=&m_colored_fong_shading;

    assert(m_vert.Valid());
    assert(m_colored_prog.Valid());
    assert(m_specular_prog.Valid());
    assert(m_specular_colored_prog.Valid());
    assert(m_fong_shading.Valid());
    assert(m_colored_fong_shading.Valid());
}


bool CScene::AddMesh(CFunctionalMesh&mesh)
{
    glCheckError();
    assert(m_meshes.size()==m_shader_data.size());
    if(std::find(m_meshes.begin(),m_meshes.end(),&mesh)!=m_meshes.end()) return false;
    m_meshes.push_back(&mesh);
    mesh.m_index=m_meshes.size()-1;
    m_shader_data.push_back({});
    CMeshShaderData&data=m_shader_data.back();

    if(mesh.Points())
    {
        auto&pts=*mesh.Points();
        MakeEigesIndexes(pts.rows(),pts.cols(),m_ints_cashe);
        data.Edges().Write(m_ints_cashe,CBuffer::dynamic_draw);

        MakeTriansIndexes(pts.rows(),pts.cols(),m_ints_cashe);
        data.Trians().Write(m_ints_cashe,CBuffer::dynamic_draw);

        MakeContiniousBuffer(pts,m_floats_cashe);
        data.Vertex().Write(m_floats_cashe,CBuffer::dynamic_draw);
        MakeBoxEdge(mesh.BoundedBox()->first,mesh.BoundedBox()->second,m_floats_cashe);
        data.BoxVertex().Write(m_floats_cashe,CBuffer::dynamic_draw);
    }
    if(mesh.Colors())
    {
        MakeContiniousBuffer(*mesh.Colors(),m_floats_cashe);
        data.Colors().Write(m_floats_cashe,CBuffer::dynamic_draw);
    }
    if(mesh.Normals())
    {
        MakeContiniousBuffer(*mesh.Normals(),m_floats_cashe);
        data.Normals().Write(m_floats_cashe,CBuffer::dynamic_draw);
    }
    for(int i=0;i<3;++i)
    {
        if(mesh.Levels(i))
        {
            data.Levels(i).clear();
            for(auto&level: *mesh.Levels(i))
            {
                MakeLevelsBuffer(i,level.m_lines,m_floats_cashe);
                data.Levels(i).push_back().Write(m_floats_cashe,CBuffer::dynamic_draw);
            }
        }
    }
    mesh.SetUpdateCallback([this,i=m_shader_data.size()-1](const CFunctionalMesh&mesh,CFunctionalMesh::CUpdateResult result)
    {
        m_UpdateMeshData(mesh,i,result);
    });
    return true;
}

bool CScene::RemoveMesh(CFunctionalMesh&m)
{
    assert(m_meshes.size()==m_shader_data.size());
    auto iter=std::find(m_meshes.begin(),m_meshes.end(),&m);
    if(iter==m_meshes.end()) return false;
    if(m_meshes.size()>1)
    {
        m_meshes.back()->m_index=(*iter)->m_index;
        (*iter)->m_index=-1;
        std::swap(m_meshes.back(),*iter);
        m_shader_data[iter-m_meshes.begin()].Swap(m_shader_data.back());
    }
    m_meshes.back()->SetUpdateCallback(nullptr);
    m_meshes.pop_back();
    m_shader_data.pop_back();
    return true;
}

bool CScene::IsMesh(const CFunctionalMesh&m)const
{
    return std::find(m_meshes.begin(),m_meshes.end(),&m)!=m_meshes.end();
}

void CScene::LegacyRender(float t)const
{
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(m_camera.ViewMatrix().data());
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(m_camera.PerspectiveMatrix().data());

    bool need_light=false;
    for(auto m:m_meshes)
    {
        auto traits=m->RenderingTraits();
        if(traits.IsSpecularSurface())
        {
            need_light=true;
            break;
        }
    }
    if(need_light)
    {
        glShadeModel(GL_SMOOTH);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightfv(GL_LIGHT0,GL_POSITION,m_light_source.Position().data());
        glLightfv(GL_LIGHT0,GL_AMBIENT,m_light_source.Ambient().data());
        glLightfv(GL_LIGHT0,GL_DIFFUSE,m_light_source.Diffuse().data());
        glLightfv(GL_LIGHT0,GL_SPECULAR,m_light_source.Specular().data());

        glLightf(GL_LIGHT0,GL_CONSTANT_ATTENUATION,m_light_source.Attenuation()[0]);
        glLightf(GL_LIGHT0,GL_LINEAR_ATTENUATION,m_light_source.Attenuation()[1]);
        glLightf(GL_LIGHT0,GL_QUADRATIC_ATTENUATION,m_light_source.Attenuation()[2]);

        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
    }
    else
    {
        glDisable(GL_LIGHTING);
    }
    glCheckError();

    for(auto m:m_meshes)
    {
        if(!m->Empty())
        {
            m->UpdateData(t);
            glMatrixMode(GL_MODELVIEW);
            glPushMatrix();
            glMultMatrixf(m->GetTransform().data());
            ::LegacyRender(*m);
            glPopMatrix();
        }
    }
    glDisable(GL_LIGHTING);
}

void CScene::m_UpdateMeshData(const CFunctionalMesh&mesh,size_t index,CFunctionalMesh::CUpdateResult up_result)
{
    // Update all buffers if its necessary

    auto& data=m_shader_data[index];
    if(up_result.UpdatePoints())
    {
        assert(mesh.Points()&&mesh.BoundedBox());

        MakeContiniousBuffer(*mesh.Points(),m_floats_cashe);
        data.Vertex().Write(m_floats_cashe,CBuffer::dynamic_draw);

        MakeBoxEdge(mesh.BoundedBox()->first,mesh.BoundedBox()->second,m_floats_cashe);
        data.BoxVertex().Write(m_floats_cashe,CBuffer::dynamic_draw);
    }
    if(up_result.UpdateColors())
    {
        assert(mesh.Colors());
        MakeContiniousBuffer(*mesh.Colors(),m_floats_cashe);
        data.Colors().Write(m_floats_cashe,CBuffer::dynamic_draw);
    }
    if(up_result.UpdateNormals())
    {
        assert(mesh.Normals());
        MakeContiniousBuffer(*mesh.Normals(),m_floats_cashe);
        data.Normals().Write(m_floats_cashe,CBuffer::dynamic_draw);
    }
    if(up_result.UpdateGrid())
    {
        //std::cout<<"UPDATE GRID\n";
        auto&pts=*mesh.Points();
        MakeEigesIndexes(pts.rows(),pts.cols(),m_ints_cashe);
        data.Edges().Write(m_ints_cashe,CBuffer::dynamic_draw);

        MakeTriansIndexes(pts.rows(),pts.cols(),m_ints_cashe);
        data.Trians().Write(m_ints_cashe,CBuffer::dynamic_draw);
    }
    for(int i=0;i<3;++i)
    {
        if(up_result.UpdateLevel(i))
        {
            //std::cout<<"UPDATE LEVELS\n";
            assert(mesh.Levels(i));
            data.Levels(i).clear();
            auto&levels_pool=*mesh.Levels(i);
            for(auto&level:levels_pool)
            {
                MakeLevelsBuffer(i,level.m_lines,m_floats_cashe);
                data.Levels(i).push_back().Write(m_floats_cashe,CBuffer::dynamic_draw);
            }

        }
    }
}

void CScene::m_RenderMesh(const CFunctionalMesh&mesh,CMeshShaderData&data,const Eigen::Matrix4f&cam_matrix)const
{
    CRenderingTraits traits=mesh.RenderingTraits();
    const auto use_vertex=CMeshShaderData::use_vertex;
    const auto use_color=CMeshShaderData::use_color;
    const auto use_normal=CMeshShaderData::use_normal;

    const Eigen::Matrix4f full_mtx=cam_matrix*mesh.GetTransform();
    // Draw box
    if(traits.IsBox())
    {
        m_vert.Use();
        m_vert.get_uniform<mat4>("full_matrix")=full_mtx;
        m_vert.get_uniform<vec3>("color")=Eigen::Vector3f(1.0,1.0,1.0);

        data.DrawBox();
    }

    // Draw level lines
    for(int i=0;i<3;++i)
    {
        glLineWidth(traits.IsSurface()? 2:1);
        if(traits.IsLevelLines(i))
        {
            m_vert.Use();
            m_vert.get_uniform<mat4>("full_matrix")=full_mtx;
            m_vert.get_uniform<vec3>("color")=traits.IsSurface()? point_t(0,0,0):point_t(1,1,1);
            data.Levels(i).DrawAll();
        }
        glLineWidth(1);
    }

    // Draw the surface
    const float alpha=1.f-mesh.Transparency();

    int surf_param=traits.IsSurface();
    surf_param|=traits.IsColored()<<1;
    surf_param|=traits.IsSpecularSurface()<<2;
    switch(surf_param)
    {
        case 1:// pure surface, uniform grey
        {
            m_vert.Use();
            m_vert.get_uniform<mat4>("full_matrix")=full_mtx;
            m_vert.get_uniform<vec3>("color")=Eigen::Vector3f(0.5,0.5,0.5);
            m_vert.get_uniform<float>("alpha")=alpha;

            data.DrawTrians(use_vertex);
        }
        break;

        case 3:// color surface
        {
            m_colored_prog.Use();
            m_colored_prog.get_uniform<mat4>("full_matrix")=full_mtx;
            m_colored_prog.get_uniform<float>("alpha")=alpha;

            data.DrawTrians(use_vertex|use_color);
        }
        break;

        case 5:// specular surface
        {
            m_actual_specular->Use();
            // Matrix
            m_actual_specular->get_uniform<mat4>("full_matrix")=full_mtx;
            m_actual_specular->get_uniform<mat4>("model_matrix4")=mesh.GetTransform();
            m_actual_specular->get_uniform<mat3>("model_matrix3")=mesh.GetRotate();
            m_actual_specular->get_uniform<vec3>("view_org")=m_camera.GetPosition();
            //light source
            m_actual_specular->get_uniform<vec3>("light_source.position")=m_light_source.Position();
            m_actual_specular->get_uniform<vec3>("light_source.ambient")=m_light_source.Ambient();
            m_actual_specular->get_uniform<vec3>("light_source.diffuse")=m_light_source.Diffuse();
            m_actual_specular->get_uniform<vec3>("light_source.specular")=m_light_source.Specular();

            //material
            auto&material=mesh.GetMaterial();
            m_actual_specular->get_uniform<float>("material.ambient")=material.ambient;
            m_actual_specular->get_uniform<float>("material.diffuse")=material.diffuse;
            m_actual_specular->get_uniform<float>("material.specular")=material.specular;
            m_actual_specular->get_uniform<float>("material.shininess")=material.shininess;
            m_actual_specular->get_uniform<int>("two_side_specular")=traits.IsTwoSideSpecular();
            m_actual_specular->get_uniform<float>("alpha")=alpha;

            data.DrawTrians(use_vertex|use_normal);
        }
        break;

        case 7:// colored and specular surface
        {
            m_actual_colored_specular->Use();
            // Matrix
            m_actual_colored_specular->get_uniform<mat4>("full_matrix")=full_mtx;
            m_actual_colored_specular->get_uniform<mat4>("model_matrix4")=mesh.GetTransform();
            m_actual_colored_specular->get_uniform<mat3>("model_matrix3")=mesh.GetRotate();
            m_actual_colored_specular->get_uniform<vec3>("view_org")=m_camera.GetPosition();
            //light source
            m_actual_colored_specular->get_uniform<vec3>("light_source.position")=m_light_source.Position();
            m_actual_colored_specular->get_uniform<vec3>("light_source.ambient")=m_light_source.Ambient();
            m_actual_colored_specular->get_uniform<vec3>("light_source.diffuse")=m_light_source.Diffuse();
            m_actual_colored_specular->get_uniform<vec3>("light_source.specular")=m_light_source.Specular();
            m_actual_colored_specular->get_uniform<int>("two_side_specular")=traits.IsTwoSideSpecular();

            m_actual_colored_specular->get_uniform<float>("shininess")=mesh.GetMaterial().shininess;
            m_actual_colored_specular->get_uniform<float>("alpha")=alpha;

            data.DrawTrians(use_vertex|use_normal|use_color);
        }
        break;

        default: assert((surf_param&1)==0);

    }

    //Draw mesh
    int param=traits.IsMesh();
    param|=traits.IsSurface()<<1;
    param|=traits.IsColored()<<2;

    switch(param)
    {
        case 1:// pure mesh, uniform white
        {
            m_vert.Use();
            m_vert.get_uniform<mat4>("full_matrix")=full_mtx;
            m_vert.get_uniform<vec3>("color")=Eigen::Vector3f(1.0,1.0,1.0);

            data.DrawEdges(use_vertex);
        }
        break;

        case 3:// mesh over unpainted surface
        case 7:// mesh over painted surface
        {
            glLineWidth(2);
            m_vert.Use();
            m_vert.get_uniform<mat4>("full_matrix")=full_mtx;
            m_vert.get_uniform<vec3>("color")=Eigen::Vector3f(0.0,0.0,0.0);

            data.DrawEdges(use_vertex);
            glLineWidth(1);
        }
        break;

        case 5:// colored mesh
        {
            m_colored_prog.Use();
            m_colored_prog.get_uniform<mat4>("full_matrix")=full_mtx;

            data.DrawEdges(use_vertex|use_color);
        }
        break;

        default:assert((param&1)==0);
    }
}


void CScene::Render(float t)const
{
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    if(m_meshes.empty()) return;
    const auto cam_matrix=m_camera.PerspectiveMatrix()*m_camera.ViewMatrix();
    m_meshes_cashe.clear();
    for(decltype(m_meshes.size()) i=0;i<m_meshes.size();++i)
    {
        if (m_meshes[i]->Empty()) continue;
        CFunctionalMesh& mesh=*m_meshes[i];
        CMeshShaderData& data=m_shader_data[i];
        mesh.UpdateData(t);
        if(m_meshes[i]->Transparency())
        {
            m_meshes_cashe.push_back(i);
            continue;
        }
        m_RenderMesh(mesh,data,cam_matrix);// Draw only no transparency meshes
    }
    if(m_meshes_cashe.empty()) return;

    // Draw transparency meshes
    glEnable(GL_BLEND);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(auto index:m_meshes_cashe)
    {
        m_RenderMesh(*m_meshes[index],m_shader_data[index],cam_matrix);
    }
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void CScene::SetFongShading(bool is_fong)
{
    if(is_fong)
    {
        m_actual_specular=&m_fong_shading;
        m_actual_colored_specular=&m_colored_fong_shading;
    }
    else
    {
        m_actual_specular=&m_specular_prog;
        m_actual_colored_specular=&m_specular_colored_prog;
    }
}


void CScene::Clear()
{
    while(!Empty())
    {
        RemoveMesh(*m_meshes.back());
    }
}




























