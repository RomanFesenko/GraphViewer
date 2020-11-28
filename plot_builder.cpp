
#include "plot_builder.h"
//#include "figure.h"
#include "../BaseLibraries/ALGO_EXTEND/output_iterator.hpp"
#include "figure.h"

///************************************
///  Обновление и отрисовка графика - *
///  модель в схеме MCV               *
///************************************

// при изменении параметра [-1,1]
// цвет меняется от синего до желтого
point3D_t DefaultColoredFunc(float param)
{
    const static point3D_t A(0.5f,0.5f,-0.5f);
    const static point3D_t B(0.5f,0.5f,0.5f);
    return B+A*param;
}


void SetMaterialReflect(const point3D_t&refl)
{
    float _buff[4];
    to_homogenius_buffer(refl,_buff);

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, _buff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, _buff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,_buff);
    glMaterialf (GL_FRONT_AND_BACK, GL_SHININESS, 10.0);
}

// отрисовка прямоугольной сетки

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

// Определение цвета вершин
void CPlot3D::m_SetPointsColor()
{
    auto close_mesh_box=m_mesh.close_box();
    point3D_t delta_diag=close_mesh_box.second-close_mesh_box.first;
    point3D_t center=(close_mesh_box.second+close_mesh_box.first)/2.0f;
    assert(distance_3D(delta_diag)>0.001f);
    m_scale_coeffs[0]=2.0f/(delta_diag[0]);
    m_scale_coeffs[1]=2.0f/(delta_diag[1]);
    m_scale_coeffs[2]=2.0f/(delta_diag[2]);
    if(m_chars.m_color_functor==nullptr)
    {
        point3D_t delta(0.5f,0.5f,-0.5f);
        m_chars.m_color_functor=[delta](const point3D_t&coords)
        {
            return point3D_t(0.5f,0.5f,0.5f)+coords[2]*delta;
        };
    }
    m_colors.resize(m_mesh.rows(),m_mesh.columns());
    const auto&points=m_mesh.get_points();
    fill_matrix(m_colors,[&points,this,center](size_t i,size_t j){

        return m_chars.m_color_functor(scale_3D(points[i][j]-center,m_scale_coeffs));

    },m_colors.rows(),m_colors.columns());
}

/*
    После определения координат всех вершин
    нужно определить цвет,нормали вершин и линии уровня
    в зависимости от установленых флагов m_chars.type
*/
void CPlot3D::m_FillMeshData()
{
    if(m_chars.m_type&plot3D_chars_t::colored_id)
    {
        m_SetPointsColor();
    }
    if((m_chars.m_type&plot3D_chars_t::levels_id)&&m_chars.m_level_lines!=0)
    {
        m_mesh.clear_levels();
        auto set_levels=[this](int coord)
        {
            float down=m_mesh.close_box().first[coord];
            float delta=(m_mesh.close_box().second[coord]-down)/(m_chars.m_level_lines+1);
            down+=delta;
            for(int i=0;i<m_chars.m_level_lines;++i,down+=delta)
            {
                m_mesh.set_level_line(down,coord);
            }
        };
        if(m_chars.m_type&plot3D_chars_t::x_levels_id){set_levels(0);}
        if(m_chars.m_type&plot3D_chars_t::y_levels_id){set_levels(1);}
        if(m_chars.m_type&plot3D_chars_t::z_levels_id){set_levels(2);}
    }
    if(m_chars.m_type&plot3D_chars_t::specular_surface_id)
    {
        m_mesh.set_normals();
    }
}

void CPlot3D::Set(const plot3D_chars_t&chars,animate_chars_t animate)
{
    //assert(!((chars.m_type&plot3D_chars_t::colored_surfase_id)&&
    //        (chars.m_type&plot3D_chars_t::specular_surface_id)));
    m_chars=chars;
    m_animate_chars=animate;
    m_mesh.clear_levels();
    m_mesh.resize(chars.m_s_range,
                  chars.m_t_range,
                  chars.m_num_s_range,
                  chars.m_num_t_range);
    if(animate.m_type==animate_chars_t::no_animate_id)
    {
        m_mesh.fill(chars.m_coords_functor);
        m_FillMeshData();
    }
    else
    {
        assert(m_animate_chars.m_coords_functor);
        UpdateForTime(0.0f);
    }
}

//Обновление всех данных в зависимости от времени
//(для анимирующих графиков)

void CPlot3D::UpdateForTime(float time)
{
    auto get_fill_functor=[this](float time)
    {
        return [time,ftr=m_animate_chars.m_coords_functor](float s,float t)
        {
            return ftr(s,t,time);
        };
    };

    auto an_type=m_animate_chars.m_type;
    if(an_type==animate_chars_t::no_animate_id){return;}
    else if(an_type==animate_chars_t::finite_id)
    {
        if(time>m_animate_chars.m_time) return;
        m_mesh.fill(get_fill_functor(time));
    }
    else if(an_type==animate_chars_t::infinite_id)
    {
        m_mesh.fill(get_fill_functor(time));
    }
    else
    {
        float delta=time-floorf(time/m_animate_chars.m_time)*m_animate_chars.m_time;
        m_mesh.fill(get_fill_functor(delta));
    }
    m_FillMeshData();
}

//Отрисовка гарфика, вызывается в главном цикле
void CPlot3D::Draw()const
{
    using size_t=std::size_t;
    using ipair_t=std::pair<size_t,size_t>;
    using segment_t=std::pair<ipair_t,ipair_t>;
    const auto&points=m_mesh.get_points();
    const auto&normals=m_mesh.get_normals();
    int flags=m_chars.m_type;

    //только для цветной поверхности
    auto colored_cell_drawing=[this,&points](size_t i,size_t j)
    {
        for(auto [i,j]:quad_matrix(i,j))
        {
            glColor3fv(m_colors[i][j].data());
            glVertex3fv(points[i][j].data());
        }
    };
    //только для отражающей поверхности
    auto  reflect_cell_drawing=[&normals,&points](size_t i,size_t j)
    {
        for(auto [i,j]:quad_matrix(i,j))
        {
            glNormal3fv(normals[i][j].data());
            glVertex3fv(points[i][j].data());
        }
    };

    //для отражающей и цветной поверхности
    auto colored_and_reflect_cell_drawing=[this,&normals,&points](size_t i,size_t j)
    {
        for(auto [i,j]:quad_matrix(i,j))
        {
            glNormal3fv(normals[i][j].data());
            SetMaterialReflect(m_colors[i][j]);
            glVertex3fv(points[i][j].data());
        }
    };


    if(flags&plot3D_chars_t::close_box_id)
    {
        //отрисовка замыкающего параллелипипеда
        auto close_mesh_box=m_mesh.close_box();
        point3D_t center=(close_mesh_box.first+close_mesh_box.second)/2.0f;
        point3D_t delta=close_mesh_box.second-close_mesh_box.first;
        auto close_box=parallelepiped(center,delta[0],delta[1],delta[2]);
        const auto& edges=close_box.edges();
        if(flags&plot3D_chars_t::specular_surface_id)
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
    if(flags&plot3D_chars_t::levels_id)
    {
        //отрисовка линий уровня
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

        if(flags&plot3D_chars_t::specular_surface_id)
        {
            glDisable(GL_LIGHTING);
        }

        if(flags&plot3D_chars_t::x_levels_id){draw_rect(0);}
        if(flags&plot3D_chars_t::y_levels_id){draw_rect(1);}
        if(flags&plot3D_chars_t::z_levels_id){draw_rect(2);}
        {
            draw_guard dg(GL_LINES);
            if(flags&plot3D_chars_t::x_levels_id){draw_levels(0);}
            if(flags&plot3D_chars_t::y_levels_id){draw_levels(1);}
            if(flags&plot3D_chars_t::z_levels_id){draw_levels(2);}
        }
        glCheckError();
    }
    if(flags&plot3D_chars_t::surface_id)
    {
        // отрисовка поверхности
        if(flags&plot3D_chars_t::specular_surface_id)
        {
            glEnable(GL_LIGHTING);
            m_light_source.SetPosition(m_mesh.close_box().second);
            if(flags&plot3D_chars_t::colored_surfase_id)
            {
                draw_guard dg(GL_QUADS);
                rows_visit(colored_and_reflect_cell_drawing,points.rows()-1,points.columns()-1);
            }
            else
            {
                SetMaterialReflect(point3D_t(1.0f,1.0f,1.0f));
                draw_guard dg(GL_QUADS);
                rows_visit(reflect_cell_drawing,points.rows()-1,points.columns()-1);
            }
        }
        else
        {
            draw_guard dg(GL_QUADS);
            rows_visit(colored_cell_drawing,points.rows()-1,points.columns()-1);
        }
    }
    if(flags&plot3D_chars_t::mesh_id)
    {
        // отрисовка сетки
        if(m_chars.m_type&m_chars.surface_id)// поверх поверхности
        {
            auto draw_line=[&points](const segment_t& line)
            {
                glVertex3fv(points[line.first.first][line.first.second].data());
                glVertex3fv(points[line.second.first][line.second.second].data());
            };
            ext::fn_output_iterator<decltype(draw_line)> iter_draw(draw_line);
            glColor3f(0,0,0);
            if(flags&plot3D_chars_t::specular_surface_id)
            {
                glDisable(GL_LIGHTING);
            }
            glLineWidth(2);
            {
                draw_guard dg(GL_LINES);
                linear_plot(iter_draw,m_mesh.rows(),m_mesh.columns());
            }
            glLineWidth(1);
            glCheckError();
        }
        else // только цветная сетка
        {
            auto draw_line=[this,&points](const segment_t& line)
            {
                glColor3fv(m_colors[line.first.first][line.first.second].data());
                glVertex3fv(points[line.first.first][line.first.second].data());

                glColor3fv(m_colors[line.first.first][line.first.second].data());
                glVertex3fv(points[line.second.first][line.second.second].data());
            };
            ext::fn_output_iterator<decltype(draw_line)> iter_draw(draw_line);
            {
                draw_guard dg(GL_LINES);
                linear_plot(iter_draw,m_mesh.rows(),m_mesh.columns());
            }
            glCheckError();
        }
    }
}
