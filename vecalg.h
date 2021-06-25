
#ifndef  _vecalg_
#define  _vecalg_

#include<math.h>
#include<type_traits>


template<class T>
constexpr T Mpi = T(3.1415926535897932385L);

template<class T>
constexpr T Degree=T(0.017453292519943);

// requirements for the array_t type - the presence of the [int] operator
// and constructor by coordinates

template<class array_t,class value_t>
array_t ort_2D(int index,value_t length)
{
    array_t res(0,0);
    res[index]=length;
    return res;
}

template<class array_t>
auto distance_2D(const array_t& pt)->decltype(pt[0]+0)
{
    return  sqrt((pt[0])*(pt[0])+(pt[1])*(pt[1]));
}

template<class array_t>
array_t normalize_2D(const array_t& pt)
{
    return pt/distance_2D(pt);
}

template<class array_t>
array_t scale_2D(const array_t& pt,const array_t& scale_coeffs)
{
    return array_t(pt[0]*scale_coeffs[0],pt[1]*scale_coeffs[1]);
}

// Turns clockwise and counterclockwise
template<class array_t>
array_t turn_clockwise(const array_t& pt)
{
    return array_t(pt[1],-pt[0]);
}

template<class array_t>
array_t turn_versus_clockwise(const array_t& pt)
{
    return array_t( -pt[1],pt[0]);
}

template<class array_t>
auto dot_product_2D(const array_t& pt1,const array_t& pt2)
->decltype(pt1[0]+0)
{
    return pt1[0]*pt2[0]+pt1[1]*pt2[1];
}

template<class array_t>
auto cross_product_2D(const array_t& pt1,const array_t& pt2)
->decltype(pt1[0]+0)
{
    return pt1[0]*pt2[1]-pt1[1]*pt2[0];
}

template<class array_t>
auto cos_angle_2D(const array_t& pt1,const array_t& pt2)
->decltype(pt1[0]+0)
{
    return dot_product_2D(pt1,pt2)/(distance_2D(pt1)*distance_2D(pt2));
}

// may be less than 0

template<class array_t>
auto sin_angle_2D(const array_t& pt1,const array_t& pt2)
->decltype(pt1[0]+0)
{
    return cross_product_2D(pt1,pt2)/(distance_2D(pt1)*distance_2D(pt2));
}


template<class array_t>
auto cos_sin_angle_2D(const array_t& pt1,const array_t& pt2)
->std::pair<decltype(pt1[0]+0),decltype(pt1[0]+0)>
{
    decltype(pt1[0]+0) temp=distance_2D(pt1)*distance_2D(pt2);
    return std::make_pair(dot_product_2D(pt1,pt2)/temp,
                          cross_product_2D(pt1,pt2)/temp);
}

// angle_2D (array_t, array_t) -the angle between vectors in the interval (-pi, + pi)
// counted from the 1st vector counterclockwise

template<class array_t>
auto angle_2D(const array_t& pt1,const array_t& pt2)
->decltype(pt1[0]+0)
{
    auto cos_sin=cos_sin_angle_2D(pt1,pt2);
    decltype(pt1[0]+0) res=acos(cos_sin.first);
    return (cos_sin.second>=0)? res:(-res);
}

// angle_2D (array_t) - angle between abscissa axis and vector
// in the range (-pi, + pi)

template<class array_t>
auto angle_2D(const array_t& pt1)
->decltype(pt1[0]+0)
{
    decltype(pt1[0]+0) res=acos(pt1[0]/distance_2D(pt1));
    return (pt1[1]>=0)? res:(-res);
}

template<class _Val>
void normalize_angle(_Val& vl)
{
    _Val _2_Mpi(2*Mpi<_Val>);
    while(vl>(Mpi<_Val>)){vl-=_2_Mpi;}
    while(vl<-(Mpi<_Val>)){vl+=_2_Mpi;}
}


//////////////////////////////////////
// Vector algebra in 3D            ///
//////////////////////////////////////

template<class array_t,class value_t>
array_t ort_3D(int index,value_t length)
{
    array_t res(0,0,0);
    res[index]=length;
    return res;
}

template<class array_t>
auto distance_3D(const array_t& pt)
->decltype(pt[2]+0)
{
    return  sqrt((pt[0])*(pt[0])+(pt[1])*(pt[1])+(pt[2])*(pt[2]));
}

template<class array_t>
array_t normalize_3D(const array_t& pt)
{
    decltype(pt[0]+0) dist=distance_3D(pt);
    return array_t(pt[0]/dist,pt[1]/dist,pt[2]/dist);
}

template<class array_t>
array_t scale_3D(const array_t& pt,const array_t& scale_coeffs)
{
    return array_t(pt[0]*scale_coeffs[0],pt[1]*scale_coeffs[1],pt[2]*scale_coeffs[2]);
}


template<class array_t>
auto dot_product_3D(const array_t& pt1,const array_t& pt2)
->decltype(pt1[2]+0)
{
    return pt1[0]*pt2[0]+pt1[1]*pt2[1]+pt1[2]*pt2[2];
}

template<class array_t>
array_t cross_product_3D(const array_t& pt1,const array_t& pt2)
{
    return array_t( pt1[1]*pt2[2]-pt1[2]*pt2[1],
                    pt1[2]*pt2[0]-pt1[0]*pt2[2],
                    pt1[0]*pt2[1]-pt1[1]*pt2[0]);
}

template<class array_t>
auto cos_angle_3D(const array_t& pt1,const array_t& pt2)
->decltype(pt1[2]+0)
{
    return dot_product_3D(pt1,pt2)/(distance_3D(pt1)*distance_3D(pt2));
}

template<class array_t>
array_t to_polar(const array_t& pt1)
{
    using value_t=decltype(pt1[2]+0);
    value_t dist2=distance_2D(pt1);
    value_t dist3=distance_3D(pt1);
    value_t fi=(pt1[1]>value_t{0})? acos(pt1[0]/dist2) : -acos(pt1[0]/dist2);
    return array_t{dist3,acos(pt1[2]/dist3),fi};
}

template<class array_t>
array_t to_decart(const array_t& pt1)//ro,@,fi
{
    return array_t{pt1[0]*sin(pt1[1])*cos(pt1[2]),
                   pt1[0]*sin(pt1[1])*sin(pt1[2]),
                   pt1[0]*cos(pt1[1])};
}


template <class T>
auto to_radian(const T& degrees)
{
    if constexpr(std::is_arithmetic<T>::value)
    {
        return Degree<T>*degrees;
    }
    else
    {
        return degrees*Degree<double>;
    }
}

template <class T>
auto to_degree(const T& rads)
{
    if constexpr(std::is_arithmetic<T>::value)
    {
        return rads/Degree<T>;
    }
    else
    {
        return rads/Degree<double>;
    }
}



#endif

