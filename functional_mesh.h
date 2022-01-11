
#ifndef  _functional_mesh__
#define  _functional_mesh__

#include <iostream>
#include <functional>
#include <array>
#include <type_traits>
#include <concepts>

#include <Eigen/Core>

#include "rigid_transform.h"


class CRenderingTraits
{
    public:
    enum flag:int
    {
        surfase_id=          1<<0,
        mesh_id=             1<<1,
        specular_id=         1<<2,
        two_side_specular_id=1<<3,
        colored_id=          1<<4,
        close_box_id=        1<<5,
        x_levels_id=         1<<6,
        y_levels_id=         1<<7,
        z_levels_id=         1<<8,
    };
    const static int specular_surface_id=surfase_id|specular_id;
    const static int levels_id=x_levels_id|y_levels_id|z_levels_id;

    private:
    int m_type;
    public:
    explicit CRenderingTraits(int i=mesh_id|two_side_specular_id):m_type(i){}

    bool IsSpecularSurface()const
    {
        return (m_type&specular_surface_id)==specular_surface_id;
    }
    bool IsTwoSideSpecular()const{return m_type&two_side_specular_id;}

    bool IsColored()const{return m_type&colored_id;}
    bool IsSurface()const{return m_type&surfase_id;}
    bool IsMesh()const{return m_type&mesh_id;}
    bool IsBox()const{return m_type&close_box_id;}
    bool IsLevelLines(int i)const
    {
        assert(i>=0&&i<3);
        return m_type&(x_levels_id<<i);
    }
    bool IsLevelsX()const{return IsLevelLines(0);}
    bool IsLevelsY()const{return IsLevelLines(1);}
    bool IsLevelsZ()const{return IsLevelLines(2);}

    CRenderingTraits& SetBox(bool _b)
    {
        _b? m_type|=close_box_id:m_type&=~close_box_id;
        return *this;
    }
    CRenderingTraits& SetSpecular(bool _b)
    {
        _b? m_type|=specular_id:m_type&=~specular_id;
        return *this;
    }
    CRenderingTraits& SetTwoSideSpecular(bool _b)
    {
        _b? m_type|=two_side_specular_id:m_type&=~two_side_specular_id;
        return *this;
    }
    CRenderingTraits& SetColored(bool _b)
    {
        _b? m_type|=colored_id:m_type&=~colored_id;
        return *this;
    }
    CRenderingTraits& SetSurface(bool _b)
    {
        _b? m_type|=surfase_id:m_type&=~surfase_id;
        return *this;
    }
    CRenderingTraits& SetMesh(bool _b)
    {
        _b? m_type|=mesh_id:m_type&=~mesh_id;
        return *this;
    }
    CRenderingTraits& SeLevels(int i,bool _b)
    {
        assert(i>=0&&i<3);
        _b? m_type|=(x_levels_id<<i):m_type&=~(x_levels_id<<i);
        return *this;
    }
    CRenderingTraits& SetLevelsX(bool _b){return SeLevels(0,_b);}
    CRenderingTraits& SetLevelsY(bool _b){return SeLevels(1,_b);}
    CRenderingTraits& SetLevelsZ(bool _b){return SeLevels(2,_b);}

    CRenderingTraits& SetFlag(int flag){m_type|=flag;return *this;}
    CRenderingTraits& DropFlag(int flag){m_type&=~flag;return *this;}
    bool              IsFlag(int flag)const{return m_type&flag;}

    friend class CFunctionalMesh;
    //void Clear(){m_type=0;}
};

struct material_t
{
    float ambient=1;
    float diffuse=1;
    float specular=1;
    float shininess=10;
};

namespace plot{

template<class func_t>
class cartesian
{
    func_t m_functor;
    public:
    cartesian(func_t f):m_functor(f){}
    template<class...params_t>
    requires std::invocable<func_t,float,float,params_t...>
    auto operator()(float s,float t,params_t...params)
    {
        return Eigen::Vector3f(s,t,m_functor(s,t,params...));
    }
};


template<class func_t>
class cylindrical
{
    func_t m_functor;
    public:
    cylindrical(func_t f):m_functor(f){}
    template<class...params_t>
    requires std::invocable<func_t,float,float,params_t...>
    auto operator()(float s,float t,params_t...params)
    {
        return Eigen::Vector3f(s*std::cos(t),s*std::sin(t),m_functor(s,t,params...));
    }
};

template<class func_t>
class revolve
{
    func_t m_functor;
    public:
    revolve(func_t f):m_functor(f){}
    template<class...params_t>
    requires std::invocable<func_t,float,float,params_t...>
    auto operator()(float phi,float z,params_t...params)
    {
        float r=m_functor(phi,z,params...);
        return Eigen::Vector3f(r*std::cos(phi),r*std::sin(phi),z);
    }
};

template<class func_t>
class spherical
{
    func_t m_functor;
    public:
    spherical(func_t f):m_functor(f){}
    template<class...params_t>
    requires std::invocable<func_t,float,float,params_t...>
    auto operator()(float teta,float phi,params_t...params)
    {
        float r=m_functor(teta,phi,params...);
        return Eigen::Vector3f(r*std::sin(teta)*cos(phi),
                               r*std::sin(teta)*sin(phi),
                               r*std::cos(teta));
    }
};

}// plot

class CFunctionalMesh
{
    static const std::size_t default_resolution=20;
    public:
    enum animation_t {dynamic_id,static_id,auto_define_id };
    using point_t=Eigen::Vector3f;
    using matrix_t=Eigen::Matrix<point_t,Eigen::Dynamic,Eigen::Dynamic>;
    //using matrix_t=Eigen::MatrixX<point_t>;
    using mesh_functor_t=std::function<point_t(float,float,float)>;
    using color_functor_t=std::function<void(const matrix_t&,matrix_t&)>;
    using size_t=std::size_t;
    struct grid_t
    {
        std::pair<float,float> s_range;
        std::pair<float,float> t_range;
        size_t s_resolution=default_resolution;
        size_t t_resolution=default_resolution;
        float s_delta()const{return (s_range.second-s_range.first)/(s_resolution);}
        float t_delta()const{return (t_range.second-t_range.first)/(t_resolution);}
        bool empty()const{return s_resolution==0||t_resolution==0;}
        float s(int i)const{return s_range.first+i*s_delta();}
        float t(int i)const{return t_range.first+i*t_delta();}
        bool operator==(const grid_t&other)const
        {
            return s_range==other.s_range&&t_range==other.t_range&&
                   s_resolution==other.s_resolution&&
                   t_resolution==other.t_resolution;
        }
        bool operator!=(const grid_t&other)const
        {
            return !(*this==other);
        }

        template<class func_t,class...mtxs_t>
        void points_visit(func_t funct,mtxs_t&...mtxs)const
        {
            for(size_t i=0;i<=s_resolution;++i)
            {
                for(size_t j=0;j<=t_resolution;++j)
                {
                    funct(mtxs(i,j)...);
                }
            }
        }

        template<class func_t,class...mtxs_t>
        void quad_visit(func_t funct,mtxs_t&...mtxs)const
        {
            for(size_t i=0;i<s_resolution;++i)
            {
                for(size_t j=0;j<t_resolution;++j)
                {
                    funct(mtxs(i,j)...);
                    funct(mtxs(i+1,j)...);
                    funct(mtxs(i+1,j+1)...);
                    funct(mtxs(i,j+1)...);
                }
            }
        }
        template<class func_t,class...mtxs_t>
        void trian_visit(func_t funct,mtxs_t&...mtxs)const
        {
            for(size_t i=0;i<s_resolution;++i)
            {
                for(size_t j=0;j<t_resolution;++j)
                {
                    funct(mtxs(i,j)...);
                    funct(mtxs(i+1,j)...);
                    funct(mtxs(i+1,j+1)...);

                    funct(mtxs(i+1,j+1)...);
                    funct(mtxs(i,j+1)...);
                    funct(mtxs(i,j)...);
                }
            }
        }

        template<class func_t,class...mtxs_t>
        void edge_visit(func_t funct,mtxs_t&...mtxs)const
        {
            for(size_t j=0;j<t_resolution;++j)
            {
                funct(mtxs(0,j)...); funct(mtxs(0,j+1)...);
            }
            for(size_t i=0;i<s_resolution;++i)
            {
                funct(mtxs(i,0)...); funct(mtxs(i+1,0)...);
                for(size_t j=0;j<t_resolution;++j)
                {
                    funct(mtxs(i+1,j)...); funct(mtxs(i+1,j+1)...);
                    funct(mtxs(i+1,j+1)...);funct(mtxs(i,j+1)...);
                }
            }
        }
    };
    using fill_functor_t=std::function<void(matrix_t&,const grid_t&,float)>;
    class CUpdateResult
    {
        enum type
        {
            update_grid=1<<0,update_points=1<<1,update_normals=1<<2,update_colors=1<<3,
            update_levels_x=1<<4,update_levels_y=1<<5,update_levels_z=1<<6,
            update_functor=1<<7
        };
        int  m_type;
        static int update_levels(int i){return update_levels_x<<i;}
        CUpdateResult(int i):m_type(i){}
        public:
        bool UpdateGrid()const{return m_type&update_grid;}
        bool UpdatePoints()const{return m_type&update_points;}
        bool UpdateNormals()const{return m_type&update_normals;}
        bool UpdateColors()const{return m_type&update_colors;}
        bool UpdateLevel(int i)const{return m_type&(update_levels_x<<i);}
        bool UpdateFunctor()const{return m_type&update_functor;}
        friend class CFunctionalMesh;
    };
    struct level_line_t
    {
        float m_constant;
        std::vector<std::pair<point_t,point_t>> m_lines;
        explicit level_line_t(float c):m_constant(c){}
        void clear(){m_lines.clear();}
        void push_back(const point_t&_1,const point_t&_2){ m_lines.push_back({_1,_2});}
    };
    private:
    int              m_index=-1;
    bool             m_functor_update=false;
    mesh_functor_t   m_points_functor;
    color_functor_t  m_colors_functor;
    fill_functor_t   m_fill_functor;
    mutable matrix_t m_points;
    mutable bool     m_valid_points=false;
    bool             m_is_dynamic=false;
    mutable matrix_t m_normals;
    mutable bool     m_valid_normals=false;
    mutable matrix_t m_colors;
    mutable bool     m_valid_colors=false;

    grid_t          m_grid;
    mutable std::array<uint32_t,3> m_num_levels={15,15,15};
    mutable std::array<std::vector<level_line_t>,3> m_levels;
    mutable std::array<bool,3> m_levels_valid={false,false,false};
    mutable std::pair<point_t,point_t> m_bounded_box;

    mutable float  m_last_update_time=0.0f;
    CRenderingTraits m_traits;
    material_t     m_material;

    // Motion data

    point_t         m_anchor={0,0,0};
    CRigidTransform m_rigid;
    std::function<void(CRigidTransform&,float)> m_moving;

    void m_InvalidateAll()const;
    void m_SetBoundedBox()const;
    void m_FillNormals()const;
    void m_SetLevelLines(int,float)const;
    public:
    CFunctionalMesh();

    // Set functions
    template<class f_t>
    CFunctionalMesh& SetMeshFunctor(f_t func,animation_t hint=auto_define_id)
    {
        using eigen_size_t=decltype(m_points.rows());
        constexpr bool binary=std::is_invocable_v<f_t,float,float>;
        constexpr bool trinary=std::is_invocable_v<f_t,float,float,float>;
        if constexpr(binary)
        {
            if(!(hint==dynamic_id&&trinary))
            {
                m_points_functor=[func](float s,float t,float time)mutable{ return func(s,t);};
                m_fill_functor=[func,this](matrix_t& mtx,const grid_t& grid,float)mutable
                {
                    float s_delta=grid.s_delta();
                    float t_delta=grid.t_delta();
                    for(eigen_size_t i_s=0;i_s<m_points.rows();++i_s)
                    {
                        float s=s_delta*i_s+grid.s_range.first;
                        for(eigen_size_t i_t=0;i_t<m_points.cols();++i_t)
                        {
                            mtx(i_s,i_t)=func(s,t_delta*i_t+grid.t_range.first);
                        }
                    }
                };
                m_is_dynamic=false;
                m_InvalidateAll();
                m_functor_update=true;
                return *this;
            }
        }
        if constexpr(trinary)
        {
           m_points_functor=func;
           m_fill_functor=[func,this](matrix_t& mtx,const grid_t& grid,float time)mutable
           {
                float s_delta=grid.s_delta();
                float t_delta=grid.t_delta();
                for(eigen_size_t i_s=0;i_s<m_points.rows();++i_s)
                {
                     float s=s_delta*i_s+grid.s_range.first;
                     for(eigen_size_t i_t=0;i_t<m_points.cols();++i_t)
                     {
                          mtx(i_s,i_t)=func(s,t_delta*i_t+grid.t_range.first,time);
                     }
                 }
            };
            m_is_dynamic=hint!=static_id;
            m_InvalidateAll();
            m_functor_update=true;
            return *this;
        }
    }

    template<class f_t>
    CFunctionalMesh& SetColorFunctor(f_t func)
    {
        using eig_size_t=decltype(m_points.rows());
        if constexpr( std::is_same_v<f_t,nullptr_t>)
        {
            m_colors_functor=[this](const matrix_t&points,matrix_t&colors)
            {
                assert(colors.rows()==points.rows()&&colors.cols()==points.cols());
                const point_t a(0.f,0.f,1.0f);
                const point_t b(1.f,1.f,-1.0f);
                const float max_dz=(m_bounded_box.second[2]-m_bounded_box.first[2])/2;
                for(eig_size_t i_s=0;i_s<points.rows();++i_s)
                {
                    for(eig_size_t i_t=0;i_t<points.cols();++i_t)
                    {
                        float relative_coord=(points(i_s,i_t)[2]-m_bounded_box.first[2])/max_dz;
                        colors(i_s,i_t)=a+relative_coord*b;
                    }
                }
            };
        }
        else
        {
            m_colors_functor=[moved=std::move(func)](const matrix_t&points,matrix_t&colors)mutable
            {
                assert(colors.rows()==points.rows()&&colors.cols()==points.cols());
                for(eig_size_t i_s=0;i_s<points.rows();++i_s)
                {
                    for(eig_size_t i_t=0;i_t<points.cols();++i_t)
                    {
                        auto&p=points(i_s,i_t);
                        colors(i_s,i_t)=moved(p[0],p[1],p[2]);
                    }
                }
            };
        }
        m_valid_colors=false;
        return *this;
    }
    template<class f_t>
    CFunctionalMesh& SetScalingColorFunctor(f_t func)
    {
        using eig_size_t=decltype(m_points.rows());
        if constexpr( std::is_same_v<f_t,nullptr_t>) return SetColorFunctor(nullptr);
        else
        {
            m_colors_functor=[moved=std::move(func),this](const matrix_t&points,matrix_t&colors)mutable
            {
                assert(colors.rows()==points.rows()&&colors.cols()==points.cols());
                const point_t diag=m_bounded_box.second-m_bounded_box.first;
                for(eig_size_t i_s=0;i_s<points.rows();++i_s)
                {
                    for(eig_size_t i_t=0;i_t<points.cols();++i_t)
                    {
                        const point_t delta=points(i_s,i_t)-m_bounded_box.first;
                        colors(i_s,i_t)=moved(delta[0]/diag[0],delta[1]/diag[1],delta[2]/diag[2]);
                    }
                }
            };
        }
        m_valid_colors=false;
        return *this;
    }

    CFunctionalMesh& SetUniformColor(const point_t&p);
    CFunctionalMesh& SetGrid(grid_t);
    CFunctionalMesh& SetResolution(size_t s_resol,size_t t_resol);
    CFunctionalMesh& SetRange(std::pair<float,float>,std::pair<float,float>);
    CFunctionalMesh& SetTraits(const CRenderingTraits&);
    CFunctionalMesh& SetNumberOfLevelsX(uint32_t);
    CFunctionalMesh& SetNumberOfLevelsY(uint32_t);
    CFunctionalMesh& SetNumberOfLevelsZ(uint32_t);
    CFunctionalMesh& SetAmbientReflection(float);
    CFunctionalMesh& SetDiffuseReflection(float);
    CFunctionalMesh& SetSpecularReflection(float);
    CFunctionalMesh& SetShininess(float);

    // Motion
    CFunctionalMesh& SetAnchor(const point_t&);
    CFunctionalMesh& DeltaOrg(float,float,float);
    CFunctionalMesh& DeltaOrg(const point_t&);
    CFunctionalMesh& SetOrg(float,float,float);
    CFunctionalMesh& SetOrg(const point_t&);

    template<class axis_t>
    CFunctionalMesh& Turn(const axis_t&axis,float ang)
    {
        m_rigid.Turn(axis,ang);
        return *this;
    }
    point_t GetOrg()const;
    const CRigidTransform::matrix_t&GetTransform()const;
    auto  GetRotate()const{return m_rigid.GetRotate();}
    auto  GetTranslate()const{return m_rigid.GetTranslate();}
    template<class functor_t>
    CFunctionalMesh& SetMoving(functor_t f)
    {
        m_moving=f;
        return *this;
    }

    void Clear();

    // Get function
    grid_t GetGrid()const;
    uint32_t GetNumberOfLevel(int)const;
    const matrix_t&Points()const;
    const matrix_t&Colors()const;
    const matrix_t&Normals()const;
    const std::pair<point_t,point_t>& BoundedBox()const;
    const std::vector<level_line_t>&Levels(int i)const{return m_levels[i];}
    bool IsDynamic()const{return m_is_dynamic;}
    bool Empty()const;
    float LastUpdateTime()const;
    CRenderingTraits&RenderingTraits();
    const CRenderingTraits&RenderingTraits()const;
    const material_t&GetMaterial()const;

    const point_t&Anchor()const{return m_anchor;}
    int   Index()const{return m_index;}


    CUpdateResult UpdateData(float);
    CUpdateResult UpdateData();

    bool ForceUpdatePoints(float)const;
    bool ForceUpdateNormals(float)const;
    bool ForceUpdateColors(float)const;

    // Set specific surface
    CFunctionalMesh& SetSphere(float r,size_t teta_resol=default_resolution,size_t phi_resol=default_resolution);
    CFunctionalMesh& SetTorus(float rad,float tubular,size_t rad_resol=default_resolution,size_t phi_resol=default_resolution);
    CFunctionalMesh& SetCylinder(float rad,float h,size_t phi_resol=default_resolution);
    CFunctionalMesh& SetCone(float rad,float h,size_t phi_resol=default_resolution);
    CFunctionalMesh& SetPlane(float dx,float dy,size_t x_resol=1,size_t y_resol=1);

    friend class CScene;
};

#endif

