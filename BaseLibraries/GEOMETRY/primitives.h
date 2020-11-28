
#ifndef  _primitives_
#define  _primitives_

#include <math.h>
#include <tuple>
#include <array>
#include <algorithm>
#include <type_traits>
#include <assert.h>

#include "vecalg.h"


// необычный рекурентный шаблон
// классы - наследники получают перегруженные
// арифметические операции с скалярами и векторами
template<class T,int _Dim,class _Der>
class FixedVector
{
    protected:
    T m_data[_Dim];
    using ptr_der_class_t=_Der*;
    _Der& m_this(){return *(static_cast<ptr_der_class_t>(this));}
    const _Der& m_this()const{return *(static_cast<const _Der*>(this));}

    public:
    template<int _int>
    class ort_t
    {
        T m_length;
        public:
        explicit ort_t(T length):m_length(length){}
        T length()const{return m_length;}
    };
    template<class _ArrVal>
    void to_array (_ArrVal* dest)const
    {
        for(int i=0;i<_Dim;++i) dest[i]=m_data[i];
    }
    const T* data()const{return m_data;}
    T* data(){return m_data;}
    T operator[](int i)const{return m_data[i];}
    T& operator[](int i){return m_data[i];}
    //_Der& FixedVector
    template<class T2>
    _Der& operator*=(T2 val)
    {
        for(int i=0;i<_Dim;++i) m_data[i]*=val;
        return m_this();
    }
    template<class T2>
    _Der operator*(T2 val) const
    {
        _Der res(m_this());
        for(int i=0;i<_Dim;++i) res[i]*=val;
        return res;
    }
    template<class T2>
    _Der& operator/=(T2 val)
    {
         for(int i=0;i<_Dim;++i) m_data[i]/=val;
         return m_this();
    }
    template<class T2>
    _Der operator/(T2 val) const
    {
        _Der res(m_this());
        for(int i=0;i<_Dim;++i) res[i]/=val;
        return res;
    }

    _Der& operator+=(const _Der& pt)
    {
        for(int i=0;i<_Dim;++i) m_data[i]+=pt[i];
        return m_this();
    }
    template<int _int>
    _Der& operator+=(ort_t<_int> ort)
    {
        m_data[_int]+=ort.length();
        return m_this();
    }

    _Der operator+(const _Der& pt)const
    {
        _Der res(m_this());
        for(int i=0;i<_Dim;++i) res.m_data[i]+=pt.m_data[i];
        return res;
    }
    template<int _int>
    _Der operator+(ort_t<_int> ort )const
    {
        _Der res(m_this());
        res[_int]+=ort.length();
        return res;
    }

    _Der& operator-=(const _Der& pt)
    {
        for(int i=0;i<_Dim;++i) m_data[i]-=pt[i];
        return m_this();
    }
    template<int _int>
    _Der& operator-=(ort_t<_int> ort)
    {
        m_data[_int]-=ort.length();
        return m_this();
    }


    _Der operator-(const _Der& pt)const
    {
        _Der res;
        for(int i=0;i<_Dim;++i) res.m_data[i]=m_data[i]-pt.m_data[i];
        return res;
    }
    template<int _int>
    _Der operator-(ort_t<_int> ort )const
    {
        _Der res(m_this());
        res[_int]-=ort.length();
        return res;
    }

    _Der operator-()const
    {
        _Der res;
        for(int i=0;i<_Dim;++i) res.m_data[i]=-m_data[i];
        return res;
    }
};

template<class T2,class T,int _Dim,class _Der>
_Der operator*(T2 rt,const FixedVector<T,_Dim,_Der>& snd)
{
    return snd*rt;
}

template<class real_t>
class Point2D: public FixedVector<real_t,2,Point2D<real_t> >
{
    public:
    Point2D(){}
    Point2D(real_t _x,real_t _y){this->m_data[0]=_x;this->m_data[1]=_y;}
    explicit Point2D(const real_t* _ptr)
    {
        this->m_data[0]=_ptr[0];
        this->m_data[1]=_ptr[1];
    }
};

template<class real_t>
class Point3D: public FixedVector<real_t,3,Point3D<real_t> >
{
    public:
    Point3D(){}
    Point3D(real_t _x,real_t _y,real_t _z)
    {this->m_data[0]=_x;this->m_data[1]=_y;this->m_data[2]=_z;}
    explicit Point3D(const real_t* _ptr)
    {
        this->m_data[0]=_ptr[0];
        this->m_data[1]=_ptr[1];
        this->m_data[2]=_ptr[2];
    }
};

// Требуются перегруженные операции +,-,/,* для типа point_t
// Построение окружности по 3 точкам на ней

template<class point_t>
auto circle_by_points_2D(const point_t& pt1,const point_t& pt2,const point_t& pt3)
->std::pair<point_t,decltype(pt1[0]+0)>
{
    using value_type=decltype(pt1[0]+0);
    using circle_t=std::pair<point_t,value_type>;
    circle_t res;
    point_t d12=pt2-pt1;
    value_type len12=distance_2D(d12);
    auto cs=cos_sin_angle_2D(pt1-pt3,pt2-pt3);
    value_type coeff=cs.first/(2.0*cs.second);//1/(2*tan(angle))
    res.second=len12/(2.0*fabs(cs.second)); // ðàäèóc
    res.first=(pt1+pt2)/2.0+coeff*turn_versus_clockwise(d12);
    return res;
}

//////////////////////////////////////////////////////////////
// Лучи и связанные с ними алгритмы                         //
//////////////////////////////////////////////////////////////

template<class point_t>
struct line_2D_t // 
{
    point_t base_point;
    using value_type=decltype(base_point[0]+0);
    value_type angle;// (-pi...pi)
    line_2D_t(const point_t& pt,value_type ang):base_point(pt),angle(ang){}
    line_2D_t(const point_t& pt1,const point_t& pt2)//от pt1 к pt2
    :base_point(pt1),angle(angle_2D(pt2-pt1)){}
    point_t ort()const{return point_t(cos(angle),sin(angle));}
};

const char Left=0;
const char Right=1;


// определение расстояние от точки до прямой
template<class point_t>
auto distance_2D(const line_2D_t<point_t>& line,const point_t& pt)
->std::pair<decltype(pt[0]+0),char>
{
    decltype(pt[0]+0) res=cross_product_2D(pt-line.base_point,line.ort());
    return (res>=0.0)? std::make_pair(res,Right):std::make_pair(-res,Left);
}

// генерация случайного массива num точек расположеных
// в интервале (0...max_pt[0], 0...max_pt[1]);
#include <random>

template<class _Int>
struct get_iniform_distribution
{
    std::uniform_int_distribution<_Int>
    operator()(_Int min_val, _Int max_val)
    {
        return std::uniform_int_distribution<_Int>(min_val,max_val);
    }
};

#define DEFINE_REAL_UNIFORM_DISTRIBUTION(FL_TYPE)\
template<>\
struct get_iniform_distribution<FL_TYPE>\
{\
    std::uniform_real_distribution<FL_TYPE>\
    operator()(FL_TYPE min_val, FL_TYPE max_val)\
    {\
        return std::uniform_real_distribution<FL_TYPE>(min_val,max_val);\
    }\
};\

DEFINE_REAL_UNIFORM_DISTRIBUTION(float)
DEFINE_REAL_UNIFORM_DISTRIBUTION(double)
DEFINE_REAL_UNIFORM_DISTRIBUTION(long double)


template<class point_t>
void generate_points_array(std::vector<point_t>& res,int num,point_t max_pt)
{
    static int for_random=2;
    using value_type=decltype(max_pt[0]+0);
    std::default_random_engine dre(for_random);
    auto urdx=get_iniform_distribution<value_type>()(0,max_pt[0]);
    auto urdy=get_iniform_distribution<value_type>()(0,max_pt[1]);
    res.clear();
    for(int i=0;i<num;++i)
    {
        res.push_back(point_t(urdx(dre),urdy(dre)));
        for_random+=2;
    }
}

// поиск минимального прямоугольника содержащего все точки
// возвращается нижняя левая и верхняя правая
template<class point_t,class iterator_t>
std::array<point_t,2> close_rectangle(iterator_t begin,iterator_t end)
{
    auto xcoord_comp=[](const point_t&pt1,const point_t&pt2){return pt1[0]<pt2[0];};
    auto ycoord_comp=[](const point_t&pt1,const point_t&pt2){return pt1[1]<pt2[1];};
    auto xdiap=std::minmax_element(begin,end,xcoord_comp);
    auto ydiap=std::minmax_element(begin,end,ycoord_comp);
    return {point_t((*(xdiap.first))[0],(*(ydiap.first))[1]),
            point_t((*(xdiap.second))[0],(*(ydiap.second))[1])};
}

// Определения точек пересечения геом примитивов

// Пересечение 2 прямых

template<class point_t>
auto intersect_lines(const line_2D_t<point_t>& ln1,const line_2D_t<point_t>& ln2)
->std::tuple<typename line_2D_t<point_t>::value_type,typename line_2D_t<point_t>::value_type,point_t>
{
    using value_type=line_2D_t<point_t>::value_type;
    point_t delta=ln1.base_point-ln2.base_point;
    point_t ort1=ln1.ort(); point_t ort2=ln2.ort();
    value_type denom=ort1[1]*ort2[0]-ort1[0]*ort2[1];
    value_type t1=(delta[0]*ort2[1]-delta[1]*ort2[0])/denom;
    return std::make_tuple
    (
        t1,
        (delta[0]*ort1[1]-delta[1]*ort1[0])/denom,
        ln1.base_point+ort1*t1
    );
}

// Пересечение окружности и прямой
template<class point_t>
struct intersect_line_circle_result
{
    using value_type=decltype(std::declval<point_t>()[0]+0);
    bool is_intersect=false;
    std::pair<value_type,point_t> point_1;
    std::pair<value_type,point_t> point_2; // óãîë è ñîáñòâåííî òî÷êà
};

template<class point_t>
intersect_line_circle_result<point_t> intersect_line_and_circle
(const std::pair<point_t,decltype(std::declval<point_t>()[0]+0)>& circle,const line_2D_t<point_t>& ln)
{
    using value_type=intersect_line_circle_result<point_t>::value_type;
    auto [dist,pos]=distance_2D(ln,circle.first);
    if(dist>circle.second) return intersect_line_circle_result<point_t>();// íå ïåðåñåêàåò
    value_type angleA=acos(dist/circle.second);
    value_type angleD=(pos==Right)? (ln.angle+Mpi<value_type>/2.0):(ln.angle-Mpi<value_type>/2.0);
    normalize_angle(angleD);
    intersect_line_circle_result<point_t> res;
    res.is_intersect=true;
    // первая точка пересечения
    res.point_1.first=angleD+angleA;normalize_angle(res.point_1.first);
    res.point_1.second=circle.first+circle.second*
    point_t(cos(res.point_1.first),sin(res.point_1.first));

    // вторая точка пересечения
    res.point_2.first=angleD-angleA;normalize_angle(res.point_2.first);
    res.point_2.second=circle.first+circle.second*
    point_t(cos(res.point_2.first),sin(res.point_2.first));
    return res;
}


// Пересечение 2 окружностей
template<class point_t>
struct intersect_two_circles_result
{
    using value_type=decltype(std::declval<point_t>()[0]+0);
    bool is_intersect=false;
    // 2 угла для обеих окр. и собственно точка
    std::tuple<value_type,value_type,point_t> point_1,point_2;
};

template<class point_t>
intersect_two_circles_result<point_t> intersect_two_circles
(const std::pair<point_t,decltype(std::declval<point_t>()[0]+0)>& circle1,
 const std::pair<point_t,decltype(std::declval<point_t>()[0]+0)>& circle2)
{
    intersect_two_circles_result<point_t> res;
    using value_type=decltype(std::declval<point_t>()[0]+0);
    auto [center1,R1]=circle1;
    auto [center2,R2]=circle2;
    auto [Rmin,Rmax]=std::minmax(R1,R2);
    value_type D12=distance_2D(center1-center2);
    // проверка на пересечение
    if((D12-R1-R2>0.0)|| (Rmax>Rmin+D12)) return res;
    res.is_intersect=true;
    value_type angleB=angle_2D(center2-center1);
    value_type delta1=acos((D12*D12+R1*R1-R2*R2)/(2*R1*D12));
    value_type delta2=acos((D12*D12+R2*R2-R1*R1)/(2*R2*D12));
    {
        auto& [a1,a2,point]=res.point_1;
        a1=angleB+delta1;normalize_angle(a1);
        a2=Mpi<value_type>+angleB+delta2;normalize_angle(a2);
        point=center1+R1*point_t(cos(a1),sin(a1));
    }
    {
        auto& [a1,a2,point]=res.point_2;
        a1=angleB-delta1;normalize_angle(a1);
        a2=Mpi<value_type>+angleB-delta2;normalize_angle(a2);
        point=center2+R2*point_t(cos(a1),sin(a1));
    }
    return res;
}

//Кривые

// Формирование базиса Безье для аппроксимации кривой
// заданой точками curve,curve.size()>=3
template<class point_t1,class point_t2>
void GetBezierPoints(const std::vector<point_t1>& curve,std::vector<point_t2>& bezier)
{
    bezier.resize(3*curve.size()-2);
    bezier[0]=curve[0];
    bezier[1]=curve[0]+(curve[1]-curve[0])/6.0;
    bezier[2]=curve[1]-(curve[2]-curve[0])/6.0;
    bezier[3]=curve[1];
    int j=4;int i=1;
    for(;i<curve.size()-2;++i)
    {
        bezier[j++]=curve[i]+(curve[i+1]-curve[i-1])/6.0;
        bezier[j++]=curve[i+1]-(curve[i+2]-curve[i])/6.0;
        bezier[j++]=curve[i+1];
    }
    bezier[j++]=curve[i]+(curve[i+1]-curve[i-1])/6.0;
    bezier[j++]=curve[i+1]-(curve[i+1]-curve[i])/6.0;
    bezier[j++]=curve[i+1];
}


template<class T>
T identity(T arg){return arg;}

template<class point_t,class real_t,class Yfunc,class Xfunc=decltype(identity<real_t>) >
void GetParametricCurvePoint
(real_t tmin,real_t tmax,real_t dt,std::vector<point_t>& arr,Yfunc ycall,
                             Xfunc xcall=identity<real_t>)
{
    arr.clear();
    for(;tmin<tmax;tmin+=dt)
    {
        arr.push_back(point_t(xcall(tmin),ycall(tmin)));
    }
}


// TEST


// определение длины биссектрисы на C по длине сторон
/*
double bissectrise(double A,double B,double C)
{
    using Point= Point2D<double>;
    Point pA(0,0); Point pB(0,A);
    auto intres=intersect_two_circles<Point>({pA,B},{pB,C});
    assert(intres.is_intersect);
    auto [v1,v2,pC]=intres.point_1;
    double ang=angle_2D(pB-pA,pC-pA)/2.0+angle_2D(pB-pA);

    normalize_angle(ang);
    line_2D_t bs1(pA,ang);
    auto [f1,f2,pD]=intersect_lines(bs1,line_2D_t(pB,pC));
    return distance_2D(pD-pA);
}

double bissectrise_test(double A,double B,double C)
{
    return sqrt(A*B*(A+B+C)*(A+B-C))/(A+B);
}


// определение длины медианы на C по длине сторон

double median(double A,double B,double C)
{
    using Point= Point2D<double>;
    Point pA(0,0); Point pB(0,A);
    auto intres=intersect_two_circles<Point>({pA,B},{pB,C});
    assert(intres.is_intersect);
    auto [v1,v2,pC]=intres.point_1;
    return distance_2D(pA-(pB+pC)/2.0);
}

double median_test(double A,double B,double C)
{
    return 0.5*sqrt(2*(A*A+B*B)-C*C);
}
*/
#endif









