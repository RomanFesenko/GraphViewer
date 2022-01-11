
#include "rigid_transform.h"


CRigidTransform& CRigidTransform::DeltaOrg(float dx,float dy,float dz)
{
    m_trans(0,3)+=dx;
    m_trans(1,3)+=dy;
    m_trans(2,3)+=dz;
    return *this;
}

CRigidTransform& CRigidTransform::DeltaOrg(const point_t&dr)
{
    m_trans(0,3)+=dr[0];
    m_trans(1,3)+=dr[1];
    m_trans(2,3)+=dr[2];
    return *this;
}

CRigidTransform& CRigidTransform::SetOrg(float x,float y,float z)
{
    m_trans(0,3)=x;
    m_trans(1,3)=y;
    m_trans(2,3)=z;
    return *this;
}

CRigidTransform& CRigidTransform::SetOrg(const point_t&r)
{
    m_trans(0,3)=r[0];
    m_trans(1,3)=r[1];
    m_trans(2,3)=r[2];
    return *this;
}

CRigidTransform::point_t CRigidTransform::GetOrg()const
{
    return {m_trans(0,3),m_trans(1,3),m_trans(2,3)};
}












