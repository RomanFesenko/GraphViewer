
#ifndef  _transformation_
#define  _transformation_

#include <math.h>
#include <algorithm>
#include <utility>

#include "matrix_ref.h"
#include "vecalg.h"


// transformations on the plane
template<class array_t,class matrix_t,class c_array_t>
void transform_2D(array_t&arr,const matrix_t& _sm,const c_array_t&pt)
{
    lag::const_matrix_ref_t sm(_sm);
    arr[0]=sm(0,0)*pt[0]+sm(0,1)*pt[1]+sm(0,2);
    arr[1]=sm(1,0)*pt[0]+sm(1,1)*pt[1]+sm(1,2);
}

// transfer matrix

template<class matrix_t,class array_t>
void TranslateMx2D(matrix_t&_mx,const array_t& delta)
{
    lag::matrix_ref_t mx(_mx);
    for(int i=0;i<3;i++)
        for(int j=0;j<3;j++) mx(i,j)=0;
    for(int i=0;i<3;i++) mx(i,i)=1;
    mx(0,2)=delta[0];
    mx(1,2)=delta[1];
}

// scaling matrix for a given displacement
// for given point,and scale factors

template<class matrix_t,class array_t,class _Val>
void ScaleMx2D(matrix_t&_mx,
               const array_t& from,
               const array_t& to,
               std::pair<_Val,_Val> scaling)
{
  lag::matrix_ref_t res(_mx);
  res(0,1)=res(1,0)=0;
  res(2,0)=res(2,1)=0;res(2,2)=1;

  res(0,0)=scaling.first;
  res(1,1)=scaling.second;
  res(0,2)=to[0]-from[0]*scaling.first;
  res(1,2)=to[1]-from[1]*scaling.second;
}

// scaling matrix for a given fixed point
// and scale factors

template<class matrix_t,class array_t,class _Val>
void ScaleMx2D(matrix_t&_mx,const array_t& immove,_Val _scx,_Val _scy)
{
    lag::matrix_ref_t res(_mx);
    res(0,1)=res(1,0)=0;
    res(2,0)=res(2,1)=0;res(2,2)=1;

    res(0,0)=_scx;res(0,2)=(1.0-_scx)*immove[0];
    res(1,1)=_scy;res(1,2)=(1.0-_scy)*immove[1];
}


// get the scaling matrix by 2 pairs of points
// transform into each other

template<class matrix_t,class array_t>
void ScaleMx2D(matrix_t&_mx,
                const array_t&from1,
                const array_t&from2,
                const array_t&to1,
                const array_t&to2)
{
    using real_t=decltype(from1[0]+0);
    lag::matrix_ref_t mx(_mx);
    real_t x12=from1[0]-from2[0];
    real_t y12=from1[1]-from2[1];
    mx(0,1)=mx(1,0)=0;
    mx(2,0)=mx(2,1)=0;mx(2,2)=1;

    mx(0,0)=(to1[0]-to2[0])/x12;
    mx(0,2)=(from1[0]*to2[0]-from2[0]*to1[0])/x12;

    mx(1,1)=(to1[1]-to2[1])/y12;
    mx(1,2)=(from1[1]*to2[1]-from2[1]*to1[1])/y12;
}

// Get the matrix of rotation around a fixed point at a given angle

template<class matrix_t,class array_t,class _Val>
void TurnMx2D(matrix_t&_mx,const array_t& immove,_Val angle)
{
    lag::matrix_ref_t mx(_mx);
    mx(2,0)=mx(2,1)=0;mx(2,2)=1;

    _Val cang=cos(angle); _Val sang=sin(angle);
    mx(0,0)=cang; mx(0,1)=-sang;
    mx(1,0)=sang; mx(1,1)=cang;
    mx(0,2)=immove[0]*(1-cang)+immove[1]*sang;
    mx(1,2)=immove[1]*(1-cang)-immove[0]*sang;
}


// Get the reflection matrix relative to the straight line
// through a given point at a given angle

template<class matrix_t,class array_t,class _Val>
void ReflectMx2D(matrix_t&_mx,const array_t& line_point,_Val angle)
{
    lag::matrix_ref_t mx(_mx);
    mx(2,0)=mx(2,1)=0;mx(2,2)=1;

    _Val cang=cos(2.0*angle); _Val sang=sin(2.0*angle);
    mx(0,0)=cang; mx(0,1)=sang;
    mx(1,0)=sang; mx(1,1)=-cang;
    mx(0,2)=line_point[0]*(1-cang)-line_point[1]*sang;
    mx(1,2)=line_point[1]*(1+cang)-line_point[0]*sang;
}

//////////////////////////////////////////////////
// 3D transformations                          ///
//////////////////////////////////////////////////


template<class array_t,class matrix_t,class c_array_t>
void transform_3D(array_t&arr,const matrix_t& _sm,const c_array_t&pt)
{
    lag::matrix_ref_t sm(_sm);
    arr[0]=sm(0,0)*pt[0]+sm(0,1)*pt[1]+sm(0,2)*pt[2]+sm(0,3);
    arr[1]=sm(1,0)*pt[0]+sm(1,1)*pt[1]+sm(1,2)*pt[2]+sm(1,3);
    arr[2]=sm(2,0)*pt[0]+sm(2,1)*pt[1]+sm(2,2)*pt[2]+sm(2,3);
}

//transfer matrix
template<class matrix_t,class array_t>
void TranslateMx3D(matrix_t&_mx,const array_t& delta)
{
    lag::matrix_ref_t res(_mx);
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++) res(i,j)=0;
    for(int i=0;i<4;i++) res(i,i)=1;
    res(0,3)=delta[0];
    res(1,3)=delta[1];
    res(2,3)=delta[2];
}


// Get the rotation matrix around the given axis passing
// through a given point to a given angle

template<class matrix_t,class array_t,class _Val>
void TurnMx3D(matrix_t&_mx,const array_t& ax,_Val angle)
{
    lag::matrix_ref_t mx(_mx);
    mx(3,0)=mx(3,1)=mx(3,2)=mx(0,3)=mx(1,3)=mx(2,3)=0;
    mx(3,3)=1;

    _Val cang=cos(angle); _Val sang=sin(angle);
    array_t nax=normalize_3D(ax);
    mx(0,0)=cang+nax[0]*nax[0]*(1-cang);
    mx(0,1)=-nax[2]*sang+nax[0]*nax[1]*(1-cang);
    mx(0,2)=nax[1]*sang+nax[0]*nax[2]*(1-cang);

    mx(1,0)=mx(0,1)+2*nax[2]*sang;
    mx(1,1)=cang+nax[1]*nax[1]*(1-cang);
    mx(1,2)=-nax[0]*sang+nax[1]*nax[2]*(1-cang);

    mx(2,0)=mx(0,2)-2*nax[1]*sang;
    mx(2,1)=mx(1,2)+2*nax[0]*sang;
    mx(2,2)=cang+nax[2]*nax[2]*(1-cang);
}

template<class matrix_t,class array_t,class _Val>
void TurnMx3D(matrix_t&_mx,const array_t& ax,_Val angle,const array_t& immove)
{
    TurnMx3D(_mx,ax,angle);
    lag::matrix_ref_t mx(_mx);
    array_t nax=normalize_3D(ax);
    array_t temp=cross_product_3D(immove,nax);
    array_t temp2=cross_product_3D(nax,temp);
    _Val v1=1-cos(angle); _Val v2=sin(angle);
    mx(0,3)=v1*temp2[0]+v2*temp[0];
    mx(1,3)=v1*temp2[1]+v2*temp[1];
    mx(2,3)=v1*temp2[2]+v2*temp[2];
}

// Get the reflection matrix relative to the given plane
// through a given point with a given normal

template<class matrix_t,class array_t>
void ReflectMx3D(matrix_t&_mx,const array_t& norm_vec)
{
    lag::matrix_ref_t mx(_mx);
    mx(3,0)=mx(3,1)=mx(3,2)=mx(0,3)=mx(1,3)=mx(2,3)=0;
    mx(3,3)=1;

    array_t nvec=normalize_3D(norm_vec);
    mx(0,0)=1-2*nvec[0]*nvec[0];
    mx(0,1)=-2*nvec[0]*nvec[1];
    mx(0,2)=-2*nvec[0]*nvec[2];

    mx(1,0)=mx(0,1);
    mx(1,1)=1-2*nvec[1]*nvec[1];
    mx(1,2)=-2*nvec[1]*nvec[2];

    mx(2,0)=mx(0,2);
    mx(2,1)=mx(1,2);
    mx(2,2)=1-2*nvec[2]*nvec[2];
}

template<class matrix_t,class array_t>
void ReflectMx3D(matrix_t&_mx,const array_t& norm_vec,const array_t&immove)
{
    ReflectMx3D(_mx,norm_vec);
    lag::matrix_ref_t mx(_mx);
    array_t nvec=normalize_3D(norm_vec);
    auto dpr=2*dot_product_3D(nvec,immove);
    mx(0,3)=dpr*nvec[0]; mx(1,3)=dpr*nvec[1]; mx(2,3)=dpr*nvec[2];
}

//------------------------------------------------
///Transformations of matrices of the view in OpenGL
//------------------------------------------------

// camera looks at the center of world coordinates
// its location is given by a vector (also given in the world coordinate system)
// camera_pos should not be strictly on the z-axis
/*
 |-sin(fi)         cos(fi)         0       0 |
 |-cos(@)*cos(fi) -cos(@)*sin(fi)  sin(@)  0 |
 |sin(@)* cos(fi)  sin(@)*sin(fi)  cos(@)  -r|
 |0                0               0       1 |

 r,(@),fi -polar coordinates of the camera

*/
template<class matrix_t,class array_t>
void fixed_look(matrix_t&_mx,const array_t& camera_pos)
{
    using value_t=decltype(camera_pos[0]+0);
    lag::matrix_ref_t mx(_mx);
    value_t dist3=distance_3D(camera_pos);
    value_t dist2=distance_2D(camera_pos);
    value_t sinfi=camera_pos[1]/dist2;
    value_t cosfi=camera_pos[0]/dist2;
    value_t sint=dist2/dist3;
    value_t cost=camera_pos[2]/dist3;

    mx(0,0)=-sinfi; mx(0,1)=cosfi; mx(0,2)=mx(0,3)=0;

    mx(1,0)=-cost*cosfi;
    mx(1,1)=-cost*sinfi;
    mx(1,2)=sint;
    mx(1,3)=0;

    mx(2,0)=sint*cosfi;
    mx(2,1)=sint*sinfi;
    mx(2,2)=cost;
    mx(2,3)=-dist3;

    mx(3,0)=mx(3,1)=mx(3,2)=0;mx(3,3)=1;
}

// Transform from a moving camera,
// located at the point camera_pos
// looking in the direction of view_ort
// camera_pos and view_ort are in world coordinates

template<class matrix_t,class array_t>
void moveable_look(matrix_t&_mx,const array_t& camera_pos,
                       const array_t& view_ort)
{
    using value_t=decltype(view_ort[0]+0);
    lag::matrix_ref_t mx(_mx);
    value_t dist3=distance_3D(view_ort);
    value_t dist2=distance_2D(view_ort);
    value_t sinfi=view_ort[1]/dist2;
    value_t cosfi=view_ort[0]/dist2;
    value_t sint=dist2/dist3;
    value_t cost=view_ort[2]/dist3;

    mx(0,0)=sinfi;
    mx(0,1)=-cosfi;
    mx(0,2)=mx(0,3)=0;

    mx(1,0)=-cost*cosfi;
    mx(1,1)=-cost*sinfi;
    mx(1,2)=sint;

    mx(2,0)=-sint*cosfi;
    mx(2,1)=-sint*sinfi;
    mx(2,2)=-cost;

    mx(0,3)=-camera_pos[0]*mx(0,0)-camera_pos[1]*mx(0,1)-camera_pos[2]*mx(0,2);
    mx(1,3)=-camera_pos[0]*mx(1,0)-camera_pos[1]*mx(1,1)-camera_pos[2]*mx(1,2);
    mx(2,3)=-camera_pos[0]*mx(2,0)-camera_pos[1]*mx(2,1)-camera_pos[2]*mx(2,2);

    mx(3,0)=mx(3,1)=mx(3,2)=0;mx(3,3)=1;
}

// perspective transform corresponding to
// gluPerspective

template<class matrix_t,class value_t>
void perspective_mx(matrix_t&_mx,value_t vert_ang,
                    value_t aspect,
                    value_t near,value_t far)
{
    lag::matrix_ref_t mx(_mx);
    for(int i=0;i<4;i++)
        for(int j=0;j<4;j++) mx(i,j)=0;

    mx(1,1)=1.0f/tan(vert_ang/2);
    mx(0,0)=mx(1,1)/aspect;
    mx(2,2)=(near+far)/(near-far); mx(2,3)=2*far*near/(near-far);
    mx(3,2)=-1;
}

#endif


