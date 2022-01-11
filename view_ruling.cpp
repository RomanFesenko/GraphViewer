
#include "view_ruling.h"

// CFixedLookCamera - the camera axis passes fixedly
// through the graph origin and can only change
// its position

bool CFixedLookCamera::SetPosition(const point_t&pos)
{
    if(!m_CheckPosition(pos)) return false;
    m_position=pos;
    gmt::fixed_look(m_view_matrix,pos);
    return true;
}

bool CFixedLookCamera::IncreasePosition(coord_t direct,const point_t&delta)
{
    using namespace gmt;
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
    fixed_look(m_view_matrix,new_pos);
    return true;
}

/// CMoveableCamera - can change as position
// camera and its axis

// set position
void CMoveableCamera::SetPosition(const point_t&pos)
{
    m_position=pos;
    gmt::moveable_look(m_view_matrix,m_position,m_view_ort);
}

//set axis
bool CMoveableCamera::SetView(const point_t&view)
{
    if(!m_CheckView(view)) return false;
    m_view_ort=gmt::normalize_3D(view);
    gmt::moveable_look(m_view_matrix,m_position,m_view_ort);
    return true;
}


void CMoveableCamera::IncreasePosition(const point_t&delta)
{
    m_position+=delta;
    gmt::moveable_look(m_view_matrix,m_position,m_view_ort);
}

bool CMoveableCamera::IncreaseView(float delta_theta,float delta_fi)
{
    using namespace gmt;
    point_t polar=to_polar(m_view_ort);
    polar[1]+=delta_theta;polar[2]+=delta_fi;
    if(polar[1]<=0.0f||polar[1]>=(Mpi<float>)) return false;
    normalize_angle(polar[2]);
    point_t new_view=to_decart(polar);
    if(!m_CheckView(new_view)) return false;
    m_view_ort=new_view;
    moveable_look(m_view_matrix,m_position,m_view_ort);
    return true;
}
