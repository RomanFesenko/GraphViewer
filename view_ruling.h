
#ifndef  _view_ruling_
#define  _view_ruling_

#include "opengl_iface.h"
#include "vecalg.h"
#include "transformation.h"



class CBaseCamera
{
    protected:
    using point_t=Eigen::Vector3f;
    using matrix3D_t=Eigen::Matrix4f;

    float m_aspect=1.0f;
    point_t m_position;
    matrix3D_t m_view_matrix;
    matrix3D_t m_perspective_matrix;
    const float m_precision=0.01f;
    public:
    CBaseCamera()
    {
        SetPerspective(gmt::to_radian(80.0f),m_aspect,0.1f,100.1f);
    }
    const matrix3D_t&ViewMatrix()const{return m_view_matrix;}
    const matrix3D_t&PerspectiveMatrix()const{return m_perspective_matrix;}
    const point_t& GetPosition()const{return m_position;}
    void SetPerspective(float vert_ang,
                        float aspect,
                        float near,
                        float far)
    {
        gmt::perspective_mx(m_perspective_matrix,vert_ang,aspect,
                                            near,far);
        m_aspect=aspect;
    }
    float Aspect()const{return m_aspect;}
};

class CFixedLookCamera:public CBaseCamera
{

    bool m_CheckPosition(const point_t&pos)
    {
        return (gmt::distance_3D(pos)>m_precision&&
                gmt::distance_2D(pos)>m_precision);
    }
    public:
    enum coord_t
    {
        id_polar,
        id_decart
    };
    CFixedLookCamera()
    {
        SetPosition(point_t(1.0f,0.0f,0.0f));
    }
    bool SetPosition(const point_t&pos);
    bool IncreasePosition(coord_t direct,const point_t&delta);
};
//inline CFixedLookCamera glCamera;


class CMoveableCamera:public CBaseCamera
{
    point_t m_view_ort;
    // нельзя смотреть строго вверх и строго вниз
    bool m_CheckView(const point_t&view_ort)
    {
        return (gmt::distance_3D(view_ort)>m_precision&&
                gmt::distance_2D(view_ort)>m_precision);
    }
    public:
    CMoveableCamera()
    {
        m_position={0.0f,0.0f,0.0f};
        SetView({1.0f,0.0f,0.0f});
        gmt::moveable_look(m_view_matrix,m_position,m_view_ort);
    }
    const point_t& GetViewOrt()const{return m_view_ort;}
    point_t GetRightOrt()const
    {
        return gmt::normalize_3D(gmt::cross_product_3D(m_view_ort,
                                            {0.0f,0.0f,1.0f}));
    }
    void SetPosition(const point_t&pos);
    bool SetView(const point_t&view);
    void IncreasePosition(const point_t&delta);
    bool IncreaseView(float delta_theta,float delta_fi);
};

#endif

