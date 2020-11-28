
#include "view_ruling.h"

///CFixedLookCamera - ось камеры фиксировано проходит
// через начало координат и может изменяться только
// ее положение

bool CFixedLookCamera::SetPosition(const point_t&pos)
{
    if(!m_CheckPosition(pos)) return false;
    m_position=pos;
    m_view_matrix=fixed_look<matrix3D_t>(pos);
    return true;
}

bool CFixedLookCamera::IncreasePosition(coord_t direct,const point_t&delta)
{
    point_t new_pos;
    if(direct==id_decart)
    {
        new_pos=m_position+delta;
    }
    else
    {
        point_t polar=to_polar(m_position)+delta;
        normalize_angle(polar[2]);
        if(polar[1]<=0.0f||polar[1] >= Mpi<float>) return false;
        new_pos=to_decart(polar);
    }
    if(!m_CheckPosition(new_pos)) return false;
    m_position=new_pos;
    m_view_matrix=fixed_look<matrix3D_t>(new_pos);
    return true;
}

///CMoveableCamera - может изменяться как положение
// камеры так и ее ось

// установить положение
void CMoveableCamera::SetPosition(const point_t&pos)
{
    m_position=pos;
    m_view_matrix=moveable_look<matrix3D_t>(m_position,m_view_ort);
}

//установить ось
bool CMoveableCamera::SetView(const point_t&view)
{
    if(!m_CheckView(view)) return false;
    m_view_ort=normalize_3D(view);
    m_view_matrix=moveable_look<matrix3D_t>(m_position,m_view_ort);
    return true;
}


void CMoveableCamera::IncreasePosition(const point_t&delta)
{
    m_position+=delta;
    m_view_matrix=moveable_look<matrix3D_t>(m_position,m_view_ort);
}

bool CMoveableCamera::IncreaseView(float delta_theta,float delta_fi)
{
    point_t polar=to_polar(m_view_ort);
    polar[1]+=delta_theta;polar[2]+=delta_fi;
    if(polar[1]<=0.0f||polar[1]>=(Mpi<float>)) return false;
    normalize_angle(polar[2]);
    point_t new_view=to_decart(polar);
    if(!m_CheckView(new_view)) return false;
    m_view_ort=new_view;
    m_view_matrix=moveable_look<matrix3D_t>(m_position,m_view_ort);
    return true;
}
