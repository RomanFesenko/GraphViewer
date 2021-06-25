
#include "plot_3D.h"
#include "output_iterator.hpp"
#include "figure.h"


light_params_t gl_lights_params=
light_params_t(
    point3D_t(0,0,5),//
    std::array<point3D_t,3>{
            point3D_t{0.1,0.1,0.1},//
            point3D_t{0.5,0.5,0.5}, //
            point3D_t{1.0,1.0,1.0}  //
    },
    point3D_t(1.1,1.0,1.0) //
);

// when changing the parameter [-1,1]
// color changes from blue to yellow
point3D_t DefaultColoredFunc(float param)
{
    const static point3D_t A(0.5f,0.5f,-0.5f);
    const static point3D_t B(0.5f,0.5f,0.5f);
    return B+A*param;
}

///////////////////////////////////////
///  Updating and drawing the 3D graph
///////////////////////////////////////


void SetMaterialReflect(const point3D_t&refl)
{
    float _buff[4];
    to_homogenius_buffer(refl,_buff);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, _buff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, _buff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,_buff);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 10.0);
}

// vizualization of normals (for debug)

void DrawNormals(const data_mesh<point3D_t,float>& mesh)
{
  const auto&p=mesh.get_points();
  const auto&n=mesh.get_normals();
  glColor3f(1,1,1);
  {
    draw_guard dg(GL_LINES);
    for(size_t i=0;i<mesh.rows();++i)
      for(size_t j=0;j<mesh.cols();++j)
      {
          glVertex3fv(p(i,j).data());
          Eigen::Vector3f p2=p(i,j)+n(i,j)*0.1f;
          glVertex3fv(p2.data());
      }
  }
}


// drawing a rectangular grid

void DrawRectangleMesh(const rect_mesh_3d_t<point3D_t>& mesh)
{
    using size_t=std::size_t;
    glColor3f(1.0f,1.0f,1.0f);
    {
        draw_guard dg(GL_LINES);
        for(size_t i=0;i<=mesh.rows();++i)
        {
            glVertex3fv(mesh.point(i,0).data());
            glVertex3fv(mesh.point(i,mesh.columns()).data());
        }
        for(size_t i=0;i<=mesh.columns();++i)
        {
            glVertex3fv(mesh.point(0,i).data());
            glVertex3fv(mesh.point(mesh.rows(),i).data());
        }
    }
}


rendering_traits_t::rendering_traits_t():
m_type(mesh_id|colored_surfase_id),
m_level_lines(15),
m_num_s_range(40),
m_num_t_range(40)
{}


CPlot3D::CPlot3D()
{
  m_light_source.SetParams(gl_lights_params);
}

void CPlot3D::m_LightingSwitch(bool _on)const
{
    if(_on)
    {
        glShadeModel(GL_SMOOTH);
        glEnable(GL_LIGHTING);
        glEnable(GL_LIGHT0);
        glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    }
    else
    {
        glDisable(GL_LIGHT0);
        glDisable(GL_LIGHTING);
    }
}

//Defining the color of the vertices

void CPlot3D::m_SetPointsColor()
{
    auto close_mesh_box=m_mesh.close_box();
    point3D_t delta_diag=close_mesh_box.second-close_mesh_box.first;
    point3D_t center=(close_mesh_box.second+close_mesh_box.first)/2.0f;
    assert(delta_diag.norm()>0.001f);
    m_scale_coeffs=2.0f/delta_diag.array();
    if(m_traits.m_color_functor==nullptr)
    {
        point3D_t delta(0.5f,0.5f,-0.5f);
        m_traits.m_color_functor=[delta](const point3D_t&coords)
        {
            return point3D_t::Constant(0.5f)+coords[2]*delta;
        };
    }
    m_colors.resize(m_mesh.rows(),m_mesh.cols());
    const auto&points=m_mesh.get_points();
    for(int i=0;i<m_colors.rows();++i)
      for(int j=0;j<m_colors.cols();++j)
        m_colors(i,j)=m_traits.m_color_functor
                ((points(i,j)-center).array()*m_scale_coeffs.array());
}

/*
    After determining the coordinates of all vertices
    need to define the color, vertex normals and level lines
    depending on the flags seted in m_traits.type
*/
void CPlot3D::m_FillMeshData(float time)
{
    if(m_traits.m_type&rendering_traits_t::colored_id)
    {
        m_SetPointsColor();
    }
    if((m_traits.m_type&rendering_traits_t::levels_id)&&m_traits.m_level_lines!=0)
    {
        m_mesh.clear_levels();
        auto set_levels=[this,time](int coord)
        {
            float down=m_mesh.close_box().first[coord];
            float delta=(m_mesh.close_box().second[coord]-down)/(m_traits.m_level_lines+1);
            down+=delta;
            for(int i=0;i<m_traits.m_level_lines;++i,down+=delta)
            {
                m_mesh.set_level_line(bind_time(m_plot_functor,time),down,coord);
            }
        };
        if(m_traits.m_type&rendering_traits_t::x_levels_id){set_levels(0);}
        if(m_traits.m_type&rendering_traits_t::y_levels_id){set_levels(1);}
        if(m_traits.m_type&rendering_traits_t::z_levels_id){set_levels(2);}
    }
    if(m_traits.m_type&rendering_traits_t::specular_surface_id)
    {
        m_mesh.set_normals();
    }
}

bool CPlot3D::IsFlag(int flags)const
{
  return m_traits.m_type&flags;
}

void CPlot3D::SetFlag(int flag,float time)
{
  m_traits.m_type|=flag;
  m_LightingSwitch(IsFlag(rendering_traits_t::specular_surface_id));
  if(!Empty()) m_FillMeshData(time);
}

void CPlot3D::DropFlag(int flags,float time)
{
  m_traits.m_type&=~flags;
  m_LightingSwitch(IsFlag(rendering_traits_t::specular_surface_id));
  if(!Empty()) m_FillMeshData(time);
}

void CPlot3D::SetTraits(const rendering_traits_t&traits,float time)
{
    //assert(!((chars.m_type&plot3D_chars_t::colored_surfase_id)&&
    //        (chars.m_type&plot3D_chars_t::specular_surface_id)));
    assert(m_plot_functor);
    if(traits.m_num_s_range!=m_traits.m_num_s_range||
       traits.m_num_t_range!=m_traits.m_num_t_range)
      {
        m_mesh.clear_levels();
        m_mesh.fill(bind_time(m_plot_functor,time),
                    m_mesh.m_s_range,m_mesh.m_t_range,
                    traits.m_num_s_range,traits.m_num_t_range);
      }
    m_traits=traits;
    m_LightingSwitch(IsFlag(rendering_traits_t::specular_surface_id));
    m_FillMeshData(time);
}

// Refresh all data, depend of time
// (for animated graphs)

void CPlot3D::UpdateForTime(float time)
{
    if(!m_is_dynamic) return;
    m_mesh.fill(bind_time(m_plot_functor,time));
    m_FillMeshData(time);
}

// Render the graph, called in the main loop
void CPlot3D::Draw()const
{
    using size_t=std::size_t;
    using ipair_t=std::pair<size_t,size_t>;
    using segment_t=std::pair<ipair_t,ipair_t>;
    const auto&points=m_mesh.get_points();
    const auto&normals=m_mesh.get_normals();
    int flags=m_traits.m_type;
    auto rows_visit=[](auto&mx,int r,int c)
    {
        for(int i=0;i<r;++i)
           for(int j=0;j<c;++j) mx(i,j);
    };
    // only for colored surface
    auto colored_cell_drawing=[this,&points](size_t i,size_t j)
    {
        for(auto [i,j]:quad_matrix(i,j))
        {
            glColor3fv(m_colors(i,j).data());
            glVertex3fv(points(i,j).data());
        }
    };
    // only for reflective surface
    auto  reflect_cell_drawing=[&normals,&points](size_t i,size_t j)
    {
        for(auto [i,j]:quad_matrix(i,j))
        {
            glNormal3fv(normals(i,j).data());
            glVertex3fv(points(i,j).data());
        }
    };

    // for reflective and colored surfaces
    auto colored_and_reflect_cell_drawing=[this,&normals,&points](size_t i,size_t j)
    {
        for(auto [i,j]:quad_matrix(i,j))
        {
            glNormal3fv(normals(i,j).data());
            SetMaterialReflect(m_colors(i,j));
            glVertex3fv(points(i,j).data());
        }
    };


    if(flags&rendering_traits_t::close_box_id)
    {
        // draw the closing box
        auto close_mesh_box=m_mesh.close_box();
        point3D_t center=(close_mesh_box.first+close_mesh_box.second)/2.0f;
        point3D_t delta=close_mesh_box.second-close_mesh_box.first;
        auto close_box=parallelepiped(center,delta[0],delta[1],delta[2]);
        const auto& edges=close_box.edges();
        if(flags&rendering_traits_t::specular_surface_id)
        {
            glDisable(GL_LIGHTING);
        }
        glColor3f(1.0,1.0,1.0);
        {
            draw_guard dg(GL_LINES);
            for(const auto& line:edges)
            {
                glVertex3fv(line.first.data());
                glVertex3fv(line.second.data());
            }
        }
        glCheckError();
    }
    if(flags&rendering_traits_t::levels_id)
    {
        // draw level lines
        const point3D_t down=m_mesh.close_box().first;
        const point3D_t center=(m_mesh.close_box().second+m_mesh.close_box().first)/2.0f;
        const point3D_t length=m_mesh.close_box().second-m_mesh.close_box().first;
        const point3D_t disp_lev=m_mesh.close_box().first-length*Levels_displace;
        auto draw_levels=[&disp_lev,&center,&length,this](int coord)
        {
            const auto& levs=m_mesh.levels()[coord];

            for(const auto& level:levs)
            {
                auto color=DefaultColoredFunc((level.m_constant-center[coord])/length[coord]);
                glColor3fv(color.data());
                for(auto [fst,snd] :level.m_lines)
                {
                    fst[coord]=snd[coord]=disp_lev[coord];
                    glVertex3fv(fst.data());
                    glVertex3fv(snd.data());
                }
            }
        };
        auto draw_rect=[&disp_lev,&length,&down](int coord)mutable
        {
            point3D_t _down=down;
            _down[coord]=disp_lev[coord];
            int next_1=(coord+1)%3;
            int next_2=(coord+2)%3;
            rect_mesh_3d_t<point3D_t> rmesh(_down,
                                            _down+ort_3D<point3D_t>(next_1,length[next_1]),
                                            _down+ort_3D<point3D_t>(next_2,length[next_2]),10,10);
            DrawRectangleMesh(rmesh);
        };

        if(flags&rendering_traits_t::specular_surface_id)
        {
            glDisable(GL_LIGHTING);
        }

        if(flags&rendering_traits_t::x_levels_id){draw_rect(0);}
        if(flags&rendering_traits_t::y_levels_id){draw_rect(1);}
        if(flags&rendering_traits_t::z_levels_id){draw_rect(2);}
        {
            draw_guard dg(GL_LINES);
            if(flags&rendering_traits_t::x_levels_id){draw_levels(0);}
            if(flags&rendering_traits_t::y_levels_id){draw_levels(1);}
            if(flags&rendering_traits_t::z_levels_id){draw_levels(2);}
        }
        glCheckError();
    }
    if(flags&rendering_traits_t::surface_id)
    {
        // render the surface
        if(flags&rendering_traits_t::specular_surface_id)
        {
            //DrawNormals(m_mesh);
            m_LightingSwitch(true);
            m_light_source.SetParams(gl_lights_params);
            m_light_source.SetPosition(m_mesh.close_box().second);
            if(flags&rendering_traits_t::colored_surfase_id)
            {
                draw_guard dg(GL_QUADS);
                rows_visit(colored_and_reflect_cell_drawing,points.rows()-1,points.cols()-1);
            }
            else
            {
                SetMaterialReflect(point3D_t(1.0f,1.0f,1.0f));
                draw_guard dg(GL_QUADS);
                rows_visit(reflect_cell_drawing,points.rows()-1,points.cols()-1);
            }
        }
        else
        {
            draw_guard dg(GL_QUADS);
            rows_visit(colored_cell_drawing,points.rows()-1,points.cols()-1);
        }
    }
    if(flags&rendering_traits_t::mesh_id)
    {
        // draw the grid
        if(m_traits.m_type&rendering_traits_t::surface_id)// поверх поверхности
        {
            auto draw_line=[&points](const segment_t& line)
            {
                glVertex3fv(points(line.first.first,line.first.second).data());
                glVertex3fv(points(line.second.first,line.second.second).data());
            };
            ext::fn_output_iterator<decltype(draw_line)> iter_draw(draw_line);
            glColor3f(0,0,0);
            if(flags&rendering_traits_t::specular_surface_id)
            {
                glDisable(GL_LIGHTING);
            }
            glLineWidth(2);
            {
                draw_guard dg(GL_LINES);
                linear_plot(iter_draw,m_mesh.rows(),m_mesh.cols());
            }
            glLineWidth(1);
            glCheckError();
        }
        else //color grid only
        {
            auto draw_line=[this,&points](const segment_t& line)
            {
                glColor3fv(m_colors(line.first.first,line.first.second).data());
                glVertex3fv(points(line.first.first,line.first.second).data());

                glColor3fv(m_colors(line.first.first,line.first.second).data());
                glVertex3fv(points(line.second.first,line.second.second).data());
            };
            ext::fn_output_iterator<decltype(draw_line)> iter_draw(draw_line);
            {
                draw_guard dg(GL_LINES);
                linear_plot(iter_draw,m_mesh.rows(),m_mesh.cols());
            }
            glCheckError();
        }
    }
}
