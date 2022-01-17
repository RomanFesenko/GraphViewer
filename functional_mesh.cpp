#include <iostream>


#include <numbers>
#include <Eigen/Geometry>

#include "functional_mesh.h"


///////////////////////////////////////////////////////////////
//                    CFunctionalMesh
///////////////////////////////////////////////////////////////

CFunctionalMesh::CFunctionalMesh()
{
    SetColorFunctor(nullptr);
}

void CFunctionalMesh::m_InvalidateAll()const
{
    m_valid_points=m_valid_normals=m_valid_colors=false;
    m_levels_valid[0]=m_levels_valid[1]=m_levels_valid[2]=false;
}

void CFunctionalMesh::m_SetBoundedBox()const
{
    using eig_size_t=decltype(m_points.rows());
    m_bounded_box.first=m_bounded_box.second=m_points(0,0);
    for(eig_size_t i_s=0;i_s<m_points.rows();++i_s)
    {
        for(eig_size_t i_t=0;i_t<m_points.cols();++i_t)
        {
            for(size_t dim=0;dim<3;dim++)
            {
                const point_t& current=m_points(i_s,i_t);
                m_bounded_box.first[dim]=std::min(current[dim],m_bounded_box.first[dim]);
                m_bounded_box.second[dim]=std::max(current[dim],m_bounded_box.second[dim]);
            }
        }
    }
}

// normal to parametrically defined surface
// defined as || (dr / ds) x (dr / dt) ||
// derivatives are approximated by finite differences

void CFunctionalMesh::m_FillNormals()const
{
    using eig_size_t=decltype(m_points.rows());
    float s_delta=m_grid.s_delta();
    auto get_s_tangent=[this,&s_delta](size_t i,size_t j)->point_t
    {
        return (m_points(i+1,j)-m_points(i-1,j))/(2*s_delta);
    };
    auto get_s_top_bound=[&](size_t j)->point_t
    {
        return (m_points(1,j)-m_points(0,j))/(s_delta);
    };
    auto get_s_down_bound=[&](size_t j)->point_t
    {
        return (m_points(m_points.rows()-1,j)-m_points(m_points.rows()-2,j))/(s_delta);
    };

    float t_delta=m_grid.t_delta();
    auto get_t_tangent=[this,&t_delta](size_t i,size_t j)->point_t
    {
        return (m_points(i,j+1)-m_points(i,j-1) )/(2*t_delta);
    };
    auto get_t_left_bound=[&](size_t i)->point_t
    {
        return (m_points(i,1)-m_points(i,0))/(t_delta);
    };
    auto get_t_right_bound=[&](size_t i)->point_t
    {
        return (m_points(i,m_points.cols()-1)-m_points(i,m_points.cols()-2))/(t_delta);
    };

    m_normals.resize(m_points.rows(),m_points.cols());

    // internal domain
    for(eig_size_t i=1;i<m_normals.rows()-1;++i)
      for(eig_size_t j=1;j<m_normals.cols()-1;++j)
      {
          m_normals(i,j)=get_s_tangent(i,j).cross(get_t_tangent(i,j));
          m_normals(i,j).normalize();
      }
    // left bound
    for(eig_size_t i=1;i<m_normals.rows()-1;++i)
    {
        m_normals(i,0)=get_s_tangent(i,0).cross(get_t_left_bound(i));
        m_normals(i,0).normalize();
    }
    // right bound
    for(eig_size_t i=1;i<m_normals.rows()-1;++i)
    {
        m_normals(i,m_normals.cols()-1)=get_s_tangent(i,0).cross(get_t_right_bound(i));
        m_normals(i,m_normals.cols()-1).normalize();
    }

    // top bound
    for(eig_size_t i=1;i<m_normals.cols()-1;++i)
    {
        m_normals(0,i)=get_s_top_bound(i).cross(get_t_tangent(0,i));
        m_normals(0,i).normalize();
    }
    // down bound
    for(eig_size_t i=1;i<m_normals.cols()-1;++i)
    {
        m_normals(m_normals.rows()-1,i)=get_s_down_bound(i).cross(get_t_tangent(m_normals.rows()-1,i));
        m_normals(m_normals.rows()-1,i).normalize();
    }
    // corners
    m_normals(0,0)=get_s_top_bound(0).cross(get_t_left_bound(0));
    m_normals(0,0).normalize();

    m_normals(0,m_normals.cols()-1)=get_s_top_bound(m_normals.cols()-1).cross(get_t_right_bound(0));
    m_normals(0,m_normals.cols()-1).normalize();

    m_normals(m_normals.rows()-1,m_normals.cols()-1)=get_s_down_bound(m_normals.cols()-1).
                                                     cross(get_t_right_bound(m_normals.rows()-1));
    m_normals(m_normals.rows()-1,m_normals.cols()-1).normalize();

    m_normals(m_normals.rows()-1,0)=get_s_down_bound(0).cross(get_t_left_bound(m_normals.rows()-1));
    m_normals(m_normals.rows()-1,0).normalize();
}

void CFunctionalMesh::m_SetLevelLines(int index,float time)const
{
    assert(index==0||index==1||index==2);
    assert(m_valid_points);
    m_levels[index].clear();
    auto cell_pocess=[index,time,this](float level,int i,int j,int& lines)
    ->std::array<point_t,4>
    {
        auto is_same_sign=[](float _1,float _2){ return (_1>=0) == (_2>=0);};
        std::array<float,4> diffs;
        std::array<point_t,4> cell={m_points(i+1,j),m_points(i+1,j+1),m_points(i,j+1),m_points(i,j)};
        std::array<int,4>   intersections;
        int num_intersetions=0;
        auto intersection_point=[&](int side)
        {
            int next=(1+side)%4;
            return (cell[side]*diffs[next]-cell[next]*diffs[side])/(diffs[next]-diffs[side]);
        };
        diffs[0]=cell[0][index]-level;
        for(int i=0;i<4;++i)
        {
            int next_index=(i+1)%4;
            diffs[next_index]=cell[next_index][index]-level;
            if(!is_same_sign(diffs[i],diffs[next_index])) intersections[num_intersetions++]=i;
        }
        lines=num_intersetions/2;
        if(lines==0) return std::array<point_t,4>{};
        else if(num_intersetions==2)
        {
            return std::array<point_t,4>{intersection_point(intersections[0]),
                                         intersection_point(intersections[1]),
                                         point_t{},point_t{}};
        }
        else if(num_intersetions==4)
        {
            float center_diff=m_points_functor(m_grid.s(i)+m_grid.s_delta()/2,
                                               m_grid.t(j)+m_grid.t_delta()/2,
                                               time)[index]-level;
            if(is_same_sign(diffs[0],center_diff))
            {
                    return {
                                intersection_point(intersections[0]),
                                intersection_point(intersections[1]),
                                intersection_point(intersections[2]),
                                intersection_point(intersections[3])
                            };
            }
            else
            {
                    return {
                                intersection_point(intersections[1]),
                                intersection_point(intersections[2]),
                                intersection_point(intersections[3]),
                                intersection_point(intersections[0])
                            };
            }
        }
        else assert(false);
    };

    float delta=(m_bounded_box.second[index]-m_bounded_box.first[index])/(1+m_num_levels[index]);

    for(size_t k=0;k<m_num_levels[index];++k)
    {
        float level=delta*(k+1);
        m_levels[index].push_back(level_line_t(level));
        auto&current=m_levels[index].back();

        for(size_t i=0;i<m_grid.s_resolution;++i)
            for(size_t j=0;j<m_grid.t_resolution;++j)
            {
                int num_lines;
                auto lines=cell_pocess(level+m_bounded_box.first[index],i,j,num_lines);
                if(num_lines==0) continue;

                if(num_lines==1) current.push_back(lines[0],lines[1]);
                if(num_lines==2) current.push_back(lines[2],lines[3]);
            }
    }
}

CFunctionalMesh& CFunctionalMesh::SetUniformColor(const point_t&p)
{
    SetColorFunctor([p](float,float,float){return p;});
    m_traits.SetColored(true);
    m_valid_colors=false;
    assert(m_traits.IsColored());
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetTraits(const CRenderingTraits&traits)
{
    m_traits=traits;
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetAmbientReflection(float val)
{
    m_material.ambient=val;
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetDiffuseReflection(float val)
{
    m_material.diffuse=val;
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetSpecularReflection(float val)
{
    m_material.specular=val;
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetShininess(float val)
{
    m_material.shininess=val;
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetGrid(grid_t grid)
{
    assert(!grid.empty());
    if(m_grid!=grid) m_InvalidateAll();
    m_grid=grid;
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetResolution(size_t s_resol,size_t t_resol)
{
    if(s_resol==m_grid.s_resolution&&t_resol==m_grid.t_resolution) return*this;
    m_grid.s_resolution=s_resol;
    m_grid.t_resolution=t_resol;
    assert(!m_grid.empty());
    m_InvalidateAll();
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetRange(std::pair<float,float> s_range,std::pair<float,float> t_range)
{
    if(s_range==m_grid.s_range&&t_range==m_grid.t_range) return *this;
    m_grid.s_range=s_range;
    m_grid.t_range=t_range;
    m_InvalidateAll();
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetNumberOfLevelsX(uint32_t x)
{
    m_levels_valid[0]= x==m_num_levels[0];
    m_num_levels[0]=x;
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetNumberOfLevelsY(uint32_t y)
{
    m_levels_valid[1]= y==m_num_levels[1];
    m_num_levels[1]=y;
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetNumberOfLevelsZ(uint32_t z)
{
    m_levels_valid[2]= z==m_num_levels[2];
    m_num_levels[2]=z;
    return *this;
}


void CFunctionalMesh::Clear()
{
    m_points_functor=nullptr;
    m_InvalidateAll();
}


CFunctionalMesh::CUpdateResult CFunctionalMesh::UpdateData(float time)
{
    int update=0;
    if(Empty()) return CUpdateResult(0);
    if(IsDynamic())
    {
       if(time!=m_last_update_time) m_InvalidateAll();
    }
    else
    {
        time=0.0f;
    }

    if(!m_valid_points)
    {
        //std::cout<<"UPDATE POINTS\n";
        if(m_grid.s_resolution+1!=m_points.rows()||m_grid.t_resolution+1!=m_points.cols())
        {
            m_points.resize(m_grid.s_resolution+1,m_grid.t_resolution+1);
            update|=CUpdateResult::update_grid;
        }
        m_fill_functor(m_points,m_grid,time);
        m_SetBoundedBox();
        m_valid_points=true;
        update|=CUpdateResult::update_points;
    }
    if(!m_valid_normals&&m_traits.IsSpecularSurface())
    {
        //std::cout<<"UPDATE NORMALS\n";
        m_normals.resize(m_grid.s_resolution+1,m_grid.t_resolution+1);
        m_FillNormals();
        m_valid_normals=true;
        update|=CUpdateResult::update_normals;
    }
    if(!m_valid_colors&&m_traits.IsColored())
    {
        //std::cout<<"UPDATE COLORS\n";
        m_colors.resize(m_grid.s_resolution+1,m_grid.t_resolution+1);
        m_colors_functor(m_points,m_colors);
        m_valid_colors=true;
        update|=CUpdateResult::update_colors;
    }
    for(int i=0;i<3;++i)
    {
        if(!m_levels_valid[i]&&m_traits.IsLevelLines(i))
        {
            //std::cout<<"UPDATE LEVELS\n";
            m_SetLevelLines(i,time);
            m_levels_valid[i]=true;
            update|=CUpdateResult::update_levels(i);
        }
    }
    m_last_update_time=time;
    if(m_update_callback)
    {
        m_update_callback(*this,CUpdateResult(update));
    }
    return CUpdateResult(update);
}


CFunctionalMesh::CUpdateResult CFunctionalMesh::UpdateData()
{
    return UpdateData(m_last_update_time);
}

// Get functions

CFunctionalMesh::grid_t CFunctionalMesh::GetGrid()const
{
    return m_grid;
}

const material_t&CFunctionalMesh::GetMaterial()const
{
    return m_material;
}

uint32_t CFunctionalMesh::GetNumberOfLevel(int i)const
{
    return m_num_levels[i];
}

const CFunctionalMesh::matrix_t*CFunctionalMesh::Points()const
{
    return m_valid_points? &m_points:nullptr;
}

const CFunctionalMesh::matrix_t*CFunctionalMesh::Colors()const
{
    return m_valid_colors? &m_colors:nullptr;
}

const CFunctionalMesh::matrix_t*CFunctionalMesh::Normals()const
{
    return m_valid_normals? &m_normals:nullptr;
}

const std::pair<CFunctionalMesh::point_t,CFunctionalMesh::point_t>* CFunctionalMesh::BoundedBox()const
{
    return m_valid_points? &m_bounded_box:nullptr;
}

const std::vector<CFunctionalMesh::level_line_t>*  CFunctionalMesh::Levels(int i)const
{
    return m_levels_valid[i]? &m_levels[i]:nullptr;
}

bool CFunctionalMesh::Empty()const
{
    return m_points_functor==nullptr;
}

float CFunctionalMesh::LastUpdateTime()const
{
    return m_last_update_time;
}

CRenderingTraits&CFunctionalMesh::RenderingTraits()
{
    return m_traits;
}

const CRenderingTraits&CFunctionalMesh::RenderingTraits()const
{
    return m_traits;
}

// Set specific surface

CFunctionalMesh& CFunctionalMesh::SetSphere(float r,size_t teta_resol,size_t phi_resol)
{
    using namespace std::numbers;
    SetMeshFunctor(plot::spherical([r](float,float){return r;}));
    SetRange({0.00001,pi_v<float>-0.00001},{0.0f,2*pi_v<float>});
    SetResolution(teta_resol,phi_resol);
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetTorus(float rad,float tubular,size_t rad_resol,size_t tubular_resol)
{
    using namespace std::numbers;
    auto mesh_f=[rad,tubular](float teta,float phi)
    {
        float r=rad+tubular*std::cos(phi);
        return point_t(r*std::cos(teta),r*std::sin(teta),tubular*std::sin(phi));
    };
    SetMeshFunctor(mesh_f);
    SetRange({0.0f,2*pi_v<float>},{0.0f,2*pi_v<float>});
    SetResolution(rad_resol,tubular_resol);
    return *this;
}


CFunctionalMesh& CFunctionalMesh::SetCylinder(float rad,float h,size_t phi_resol)
{
    using namespace std::numbers;
    auto mesh_f=[rad](float phi,float z)
    {
        return point_t(rad*std::cos(phi),rad*std::sin(phi),z);
    };
    SetMeshFunctor(mesh_f);
    SetRange({0.0f,2*pi_v<float>},{0.0f,h});
    SetResolution(phi_resol,1);
    return *this;
}


CFunctionalMesh& CFunctionalMesh::SetCone(float bottom_rad,float h,size_t phi_resol)
{
    using namespace std::numbers;
    auto mesh_f=[bottom_rad,h](float phi,float z)
    {
        float rad=bottom_rad*(1.0f-z/h);
        return point_t(rad*std::cos(phi),rad*std::sin(phi),z);
    };
    SetMeshFunctor(mesh_f);
    SetRange({0.0f,2*pi_v<float>},{0.0f,h});
    SetResolution(phi_resol,1);
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetPlane(float dx,float dy,size_t x_resol,size_t y_resol)
{
    SetMeshFunctor([](float x,float y){return point_t(x,y,0);});
    SetRange({-dx/2,dx/2},{-dy/2,dy/2});
    SetResolution(x_resol,y_resol);
    return *this;
}


// Transformations

CFunctionalMesh& CFunctionalMesh::SetAnchor(const point_t&p)
{
    m_anchor=p;
    return *this;
}

CFunctionalMesh& CFunctionalMesh::DeltaOrg(float x,float y,float z)
{
    m_rigid.DeltaOrg(x,y,z);
    return *this;
}

CFunctionalMesh& CFunctionalMesh::DeltaOrg(const point_t&p)
{
    m_rigid.DeltaOrg(p);
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetOrg(float x,float y,float z)
{
    m_rigid.SetOrg(x,y,z);
    return *this;
}

CFunctionalMesh& CFunctionalMesh::SetOrg(const point_t&p)
{
    m_rigid.SetOrg(p);
    return *this;
}

CRigidTransform::point_t CFunctionalMesh::GetOrg()const
{
    return m_rigid.GetOrg();
}

const CRigidTransform::matrix_t& CFunctionalMesh::GetTransform()const
{
    return m_rigid.GetTransform();
}
