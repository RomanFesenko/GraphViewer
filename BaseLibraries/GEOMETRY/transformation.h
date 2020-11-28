
#ifndef  _transformation_
#define  _transformation_

#include <algorithm>
#include "vecalg.h"
#include "../LINALG/linalg.h"

//преобразования на плоскости

template<class matrix_t,class array_t>
array_t transform_2D(const matrix_t& sm,const array_t&pt)
{
    return array_t
    (
        sm[0][0]*pt[0]+sm[0][1]*pt[1]+sm[0][2],
        sm[1][0]*pt[0]+sm[1][1]*pt[1]+sm[1][2]
    );
}

// матрица переноса


template<class matrix_t,class array_t>
matrix_t TranslateMx2D(const array_t& delta)
{
    matrix_t res;
    to_identity(res,3,3);
    res[0][2]=delta[0];
    res[1][2]=delta[1];
    return res;
}

// матрица масштабирования по данной неподвижной точке
// и масштабным коэффицентам

template<class matrix_t,class array_t,class _Val>
matrix_t ScaleMx2D(const array_t& immove,_Val _scx,_Val _scy)
{
    matrix_t res;
    res[0][1]=res[1][0]=0;
    res[2][0]=res[2][1]=0;res[2][2]=1;

    res[0][0]=_scx;res[0][2]=(1.0-_scx)*immove[0];
    res[1][1]=_scy;res[1][2]=(1.0-_scy)*immove[1];
    return res;
}


// получение матрицы масштабирования по 2 парам точек
// преобразующимся друг в друга

template<class matrix_t,class array_t>
matrix_t ScaleMx2D(const array_t&from1,
                   const array_t&from2,
                   const array_t&to1,
                   const array_t&to2)
{
    using real_t=decltype(from1[0]+0);
    real_t x12=from1[0]-from2[0];
    real_t y12=from1[1]-from2[1];
    matrix_t mx;
    mx[0][1]=mx[1][0]=0;
    mx[2][0]=mx[2][1]=0;mx[2][2]=1;

    mx[0][0]=(to1[0]-to2[0])/x12;
    mx[0][2]=(from1[0]*to2[0]-from2[0]*to1[0])/x12;

    mx[1][1]=(to1[1]-to2[1])/y12;
    mx[1][2]=(from1[1]*to2[1]-from2[1]*to1[1])/y12;
    return mx;
}

// Получение матрицы поворoта вокруг неподвижной точки на данный угол

template<class matrix_t,class array_t,class _Val>
matrix_t TurnMx2D(const array_t& immove,_Val angle)
{
    matrix_t mx;
    mx[2][0]=mx[2][1]=0;mx[2][2]=1;

    _Val cang=cos(angle); _Val sang=sin(angle);
    mx[0][0]=cang; mx[0][1]=-sang;
    mx[1][0]=sang; mx[1][1]=cang;
    mx[0][2]=immove[0]*(1-cang)+immove[1]*sang;
    mx[1][2]=immove[1]*(1-cang)-immove[0]*sang;
    return mx;
}



// Получение матрицы отражения относительно прямой проходящей
// через данную точку под данным углом

template<class matrix_t,class array_t,class _Val>
matrix_t ReflectMx2D(const array_t& line_point,_Val angle)
{
    matrix_t mx;
    mx[2][0]=mx[2][1]=0;mx[2][2]=1;

    _Val cang=cos(2.0*angle); _Val sang=sin(2.0*angle);
    mx[0][0]=cang; mx[0][1]=sang;
    mx[1][0]=sang; mx[1][1]=-cang;
    mx[0][2]=line_point[0]*(1-cang)-line_point[1]*sang;
    mx[1][2]=line_point[1]*(1+cang)-line_point[0]*sang;
    return mx;
}

//////////////////////////////////////////////////
// Преобразования в пространстве               ///
//////////////////////////////////////////////////


template<class matrix_t,class array_t>
array_t transform_3D(const matrix_t& sm,const array_t&pt)
{
    return array_t
    (
        sm[0][0]*pt[0]+sm[0][1]*pt[1]+sm[0][2]*pt[2]+sm[0][3],
        sm[1][0]*pt[0]+sm[1][1]*pt[1]+sm[1][2]*pt[2]+sm[1][3],
        sm[2][0]*pt[0]+sm[2][1]*pt[1]+sm[2][2]*pt[2]+sm[2][3]
    );
}

// матрица переноса
template<class matrix_t,class array_t>
matrix_t TranslateMx3D(const array_t& delta)
{
    matrix_t res;
    to_identity(res,4,4);
    res[0][3]=delta[0];
    res[1][3]=delta[1];
    res[2][3]=delta[2];
    return res;
}


// Получение матрицы поворoта вокруг данной оси проходящей
// через данную точку на данный угол

template<class matrix_t,class array_t,class _Val>
matrix_t TurnMx3D(const array_t& ax,_Val angle)
{
    matrix_t mx;
    mx[3][0]=mx[3][1]=mx[3][2]=mx[0][3]=mx[1][3]=mx[2][3]=0;
    mx[3][3]=1;


    _Val cang=cos(angle); _Val sang=sin(angle);
    array_t nax=normalize_3D(ax);
    mx[0][0]=cang+nax[0]*nax[0]*(1-cang);
    mx[0][1]=-nax[2]*sang+nax[0]*nax[1]*(1-cang);
    mx[0][2]=nax[1]*sang+nax[0]*nax[2]*(1-cang);

    mx[1][0]=mx[0][1]+2*nax[2]*sang;
    mx[1][1]=cang+nax[1]*nax[1]*(1-cang);
    mx[1][2]=-nax[0]*sang+nax[1]*nax[2]*(1-cang);

    mx[2][0]=mx[0][2]-2*nax[1]*sang;
    mx[2][1]=mx[1][2]+2*nax[0]*sang;
    mx[2][2]=cang+nax[2]*nax[2]*(1-cang);

    return mx;
}

template<class matrix_t,class array_t,class _Val>
matrix_t TurnMx3D(const array_t& ax,_Val angle,const array_t& immove)
{
    matrix_t mx=TurnMx3D(ax,angle);
    array_t nax=normalize_3D(ax);
    array_t temp=cross_product_3D(immove,nax);
    array_t temp2=cross_product_3D(nax,temp);
    _Val v1=1-cos(angle); _Val v2=sin(angle);
    mx[0][3]=v1*temp2[0]+v2*temp[0];
    mx[1][3]=v1*temp2[1]+v2*temp[1];
    mx[2][3]=v1*temp2[2]+v2*temp[2];
    return mx;
}

// Получение матрицы отражения относительно плоскости проходящей
// через данную точку с данной нормалью

template<class matrix_t,class array_t>
matrix_t ReflectMx3D(const array_t& norm_vec)
{
    matrix_t mx;
    mx[3][0]=mx[3][1]=mx[3][2]=mx[0][3]=mx[1][3]=mx[2][3]=0;
    mx[3][3]=1;

    array_t nvec=normalize_3D(norm_vec);
    mx[0][0]=1-2*nvec[0]*nvec[0];
    mx[0][1]=-2*nvec[0]*nvec[1];
    mx[0][2]=-2*nvec[0]*nvec[2];

    mx[1][0]=mx[0][1];
    mx[1][1]=1-2*nvec[1]*nvec[1];
    mx[1][2]=-2*nvec[1]*nvec[2];

    mx[2][0]=mx[0][2];
    mx[2][1]=mx[1][2];
    mx[2][2]=1-2*nvec[2]*nvec[2];
    return mx;
}

template<class matrix_t,class array_t>
matrix_t ReflectMx3D(const array_t& norm_vec,const array_t&immove)
{
    matrix_t mx=ReflectMx3D(norm_vec);
    array_t nvec=normalize_3D(norm_vec);
    auto dpr=2*dot_product_3D(nvec,immove);
    mx[0][3]=dpr*nvec[0]; mx[1][3]=dpr*nvec[1]; mx[2][3]=dpr*nvec[2];
    return mx;
}

//------------------------------------------------
///Преобразования матриц вида в OpenGL
//------------------------------------------------

// камера смотрит в центр мировых координат
// ее расположение задано вектором (также заданым в мировой ск)
// camera_pos не должна находиться строго на оси z
/*
 |-sin(fi)         cos(fi)         0       0 |
 |-cos(@)*cos(fi) -cos(@)*sin(fi)  sin(@)  0 |
 |sin(@)* cos(fi)  sin(@)*sin(fi)  cos(@)  -r|
 |0                0               0       1 |

 r,(@),fi -полярные координаты камеры

*/
template<class matrix_t,class array_t>
matrix_t fixed_look(const array_t& camera_pos)
{
    using value_t=decltype(camera_pos[0]+0);

    value_t dist3=distance_3D(camera_pos);
    value_t dist2=distance_2D(camera_pos);
    value_t sinfi=camera_pos[1]/dist2;
    value_t cosfi=camera_pos[0]/dist2;
    value_t sint=dist2/dist3;
    value_t cost=camera_pos[2]/dist3;

    matrix_t mx;
    mx[0][0]=-sinfi;
    mx[0][1]=cosfi;
    mx[0][2]=mx[0][3]=0;

    mx[1][0]=-cost*cosfi;
    mx[1][1]=-cost*sinfi;
    mx[1][2]=sint;
    mx[1][3]=0;

    mx[2][0]=sint*cosfi;
    mx[2][1]=sint*sinfi;
    mx[2][2]=cost;
    mx[2][3]=-dist3;

    mx[3][0]=mx[3][1]=mx[3][2]=0;mx[3][3]=1;

    return mx;
}

// Преобразование от подвижной камеры,
// находящееся в точке camera_pos
// смотрящей по направлению view_ort
// camera_pos и view_ort заданы в мировых координатах

template<class matrix_t,class array_t>
matrix_t moveable_look(const array_t& camera_pos,
                       const array_t& view_ort)
{
    using value_t=decltype(view_ort[0]+0);

    value_t dist3=distance_3D(view_ort);
    value_t dist2=distance_2D(view_ort);
    value_t sinfi=view_ort[1]/dist2;
    value_t cosfi=view_ort[0]/dist2;
    value_t sint=dist2/dist3;
    value_t cost=view_ort[2]/dist3;

    matrix_t mx;
    mx[0][0]=sinfi;
    mx[0][1]=-cosfi;
    mx[0][2]=mx[0][3]=0;

    mx[1][0]=-cost*cosfi;
    mx[1][1]=-cost*sinfi;
    mx[1][2]=sint;

    mx[2][0]=-sint*cosfi;
    mx[2][1]=-sint*sinfi;
    mx[2][2]=-cost;

    mx[0][3]=-camera_pos[0]*mx[0][0]-camera_pos[1]*mx[0][1]-camera_pos[2]*mx[0][2];
    mx[1][3]=-camera_pos[0]*mx[1][0]-camera_pos[1]*mx[1][1]-camera_pos[2]*mx[1][2];
    mx[2][3]=-camera_pos[0]*mx[2][0]-camera_pos[1]*mx[2][1]-camera_pos[2]*mx[2][2];

    mx[3][0]=mx[3][1]=mx[3][2]=0;mx[3][3]=1;
    return mx;
}

//перспективное преобразование,соответсвующее
// gluPerspective

template<class matrix_t,class value_t>
matrix_t perspective_mx(value_t vert_ang,
                        value_t aspect,
                        value_t near,value_t far)
{
    matrix_t mx;
    to_zero(mx,4,4);

    mx[1][1]=1.0f/tan(vert_ang/2);
    mx[0][0]=mx[1][1]/aspect;
    mx[2][2]=(near+far)/(near-far); mx[2][3]=2*far*near/(near-far);
    mx[3][2]=-1;
    return mx;
}

#endif



