
#ifndef  _rigid_transform_
#define  _rigid_transform_


#include <array>
#include <string>
#include <iostream>

#include <cmath>
#include <Eigen/Core>

#include "transformation.h"

class CAxis
{
    using point_t=Eigen::Vector3f;
    template<bool is_abs>
    struct disp_axis_t
    {
        point_t m_dir;
        point_t m_disp;
    };
    template<bool is_abs>
    struct axis_t
    {
        point_t m_dir;
        axis_t(float x,float y,float z):m_dir(x,y,z){}
        axis_t(const point_t&dir):m_dir(dir){}
        disp_axis_t<is_abs> Displaced(const point_t&p)const{return disp_axis_t<is_abs>{m_dir,p};}
        disp_axis_t<is_abs> Displaced(float x,float y,float z)const{return disp_axis_t<is_abs>{m_dir,point_t(x,y,z)};}
    };
    public:
    static axis_t<true> Abs(float x,float y,float z)
    {
        float norm=std::sqrt(x*x+y*y+z*z);
        return axis_t<true>(x/norm,y/norm,z/norm);
    }
    static axis_t<true> Abs(const point_t&dir)
    {
        return axis_t<true>(dir.normalized());
    }
    static axis_t<true> AbsX(){return axis_t<true>(1,0,0);}
    static axis_t<true> AbsY(){return axis_t<true>(0,1,0);}
    static axis_t<true> AbsZ(){return axis_t<true>(0,0,1);}

    static axis_t<false> Rel(float x,float y,float z)
    {
        float norm=std::sqrt(x*x+y*y+z*z);
        return axis_t<false>(x/norm,y/norm,z/norm);
    }
    static axis_t<false> Rel(const point_t&dir)
    {
        return axis_t<false>(dir.normalized());
    }
    static axis_t<false> RelX(){return axis_t<false>(1,0,0);}
    static axis_t<false> RelY(){return axis_t<false>(0,1,0);}
    static axis_t<false> RelZ(){return axis_t<false>(0,0,1);}

    friend class CMobileObject;
};

class CRigidTransform
{
    public:
    using point_t=Eigen::Vector3f;
    using matrix_t=Eigen::Matrix4f;
    private:
    matrix_t m_trans=Eigen::Matrix4f::Identity();
    public:
    CRigidTransform& DeltaOrg(float,float,float);
    CRigidTransform& DeltaOrg(const point_t&);
    CRigidTransform& SetOrg(float,float,float);
    CRigidTransform& SetOrg(const point_t&);

    template<bool is_abs>
    CRigidTransform& Turn(const CAxis::axis_t<is_abs>&axis,float ang)
    {
        Eigen::Matrix4f mtx;
        if constexpr(is_abs)
        {
            gmt::TurnMx3D(mtx,axis.m_dir,ang);
        }
        else
        {
            static_assert(is_abs);//dummy
        }
        mtx*=m_trans;
        m_trans=mtx;
        return *this;
    }
    template<bool is_abs>
    CRigidTransform& Turn(const CAxis::disp_axis_t<is_abs>&disp_axis,float ang)
    {
        Eigen::Matrix4f mtx;
        if constexpr(is_abs)
        {
            gmt::TurnMx3D(mtx,disp_axis.m_dir,ang,disp_axis.m_disp);
        }
        else
        {
            static_assert(is_abs);//dummy
        }
        mtx*=m_trans;
        m_trans=mtx;
        return *this;
    }
    point_t GetOrg()const;
    const matrix_t&GetTransform()const{return m_trans;}
    Eigen::Matrix3f GetRotate()const{return m_trans.block<3,3>(0,0);}
    point_t GetTranslate()const{return m_trans.block<3,1>(0,3);}
};

#endif

