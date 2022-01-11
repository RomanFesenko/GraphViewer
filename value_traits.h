
#ifndef  _value_traits_
#define  _value_traits_


//#define BOOST_DISABLE


#include <math.h>
#include <numeric>
#include <complex>
#include <random>

/*#ifndef BOOST_DISABLE
#include <boost/multiprecision/cpp_bin_float.hpp>
#endif*/

template<class T>
struct value_traits
{
    using value_t=T;
    using abs_value_t=T;
    inline static std::default_random_engine m_dre;
    static constexpr value_t zero()           {return T(0);}
    static constexpr value_t one()            {return T(1);}
    static constexpr value_t integer(int i)   {return T(i);}
    static constexpr value_t sqrt(value_t val){return std::sqrt(val);}
    static constexpr value_t atan(value_t val){return std::atan(val);}
    static constexpr value_t sin(value_t val){return std::sin(val);}
    static constexpr value_t cos(value_t val){return std::cos(val);}
    static constexpr abs_value_t epsilon()    {return std::numeric_limits<abs_value_t>::epsilon();}
    static abs_value_t abs(value_t v) {return v>=0? v:-v;}
    static constexpr bool is_real() {return true;}
    static constexpr bool is_complex() {return false;}
    static bool almost_zero(value_t v)
    {
        if constexpr(std::is_arithmetic_v<T>)
        {
                static const T tolerance=std::numeric_limits<T>::epsilon()*16;
                return abs(v)<=tolerance;
        }
        else    return v==zero();
    }
    static bool almost_equal(value_t v1,value_t v2)
    {
        if constexpr(std::is_arithmetic_v<T>)
        {
            static const value_t tolerance=std::numeric_limits<T>::epsilon()*16;
            abs_value_t delta=abs(v1-v2);
            return delta<=tolerance||
                   delta<=std::max(abs(v1),abs(v2))*tolerance;
        }
        else    return v1==v2;
    }
    static auto random_uniform_distribution(value_t min,value_t max)
    {
        if constexpr(std::is_floating_point_v<value_t>)
        {
            return [urd=std::uniform_real_distribution<value_t>{min,max}]()mutable
            {
                return urd(m_dre);
            };
        }
        else
        {
            return [urd=std::uniform_int_distribution<value_t>{min,max}]()mutable
            {
                return urd(m_dre);
            };
        }
    }
};

template<class T>
struct value_traits<std::complex<T>>
{
    using value_t=std::complex<T>;
    using abs_value_t=T;
    inline static std::default_random_engine m_dre;
    static constexpr value_t zero()           {return std::complex<T>(0,0);}
    static constexpr value_t one()            {return std::complex<T>(1,0);}
    static constexpr value_t integer(int i)   {return T(i,0);}
    static constexpr value_t sqrt(value_t val){return std::sqrt(val);}
    static constexpr value_t atan(value_t val){return std::atan(val);}
    static constexpr value_t sin(value_t val){return std::sin(val);}
    static constexpr value_t cos(value_t val){return std::cos(val);}
    static constexpr abs_value_t epsilon()    {return std::numeric_limits<abs_value_t>::epsilon();}
    static abs_value_t abs(const value_t&v){return std::abs(v);}
    static constexpr  bool is_real() {return false;}
    static constexpr  bool is_complex() {return true;}
    static bool almost_zero(const value_t&v)
    {
        if constexpr(std::is_arithmetic_v<T>)
        {
                static const abs_value_t tolerance=std::numeric_limits<T>::epsilon()*10;
                return abs(v)<=tolerance;
        }
        else    return v.real()==0&&v.imag()==0;
    }
    static bool almost_equal(const value_t&v1,const value_t&v2)
    {
        if constexpr(std::is_arithmetic_v<T>)
        {
            static const abs_value_t tolerance=std::numeric_limits<T>::epsilon()*16;
            abs_value_t delta=abs(v1-v2);
            return delta<=tolerance||
                   delta<=std::max(abs(v1),abs(v2))*tolerance;
        }
        else    return v1.real()==v2.real()&&v1.imag()==v2.imag();
    }
    static auto random_uniform_distribution(value_t min,value_t max)
    {
        if constexpr(std::is_floating_point_v<T>)
        {
            return [urd_real=std::uniform_real_distribution<T>{min.real(),max.real()},
                    urd_imag=std::uniform_real_distribution<T>{min.imag(),max.imag()}]()mutable
            {
                return value_t(urd_real(m_dre),urd_imag(m_dre));
            };
        }
        else
        {
            return [urd_real=std::uniform_int_distribution<T>{min.real(),max.real()},
                    urd_imag=std::uniform_int_distribution<T>{min.imag(),max.imag()}]()mutable
            {
                return value_t(urd_real(m_dre),urd_imag(m_dre));
            };
        }
    }
};


/*
#ifndef BOOST_DISABLE
template<>
struct value_traits<boost::multiprecision::cpp_bin_float_50>
{
    using value_t=boost::multiprecision::cpp_bin_float_50;
    using abs_value_t=value_t;
    inline static std::default_random_engine m_dre;
    static  value_t zero()           {return value_t(0);}
    static  value_t one()            {return value_t(1);}
    static  value_t integer(int i)   {return value_t(i);}
    static  value_t sqrt(value_t val){return boost::multiprecision::sqrt(val);}
    static  value_t atan(value_t val){return boost::multiprecision::atan(val);}
    static  value_t sin(value_t val){return boost::multiprecision::sin(val);}
    static  value_t cos(value_t val){return boost::multiprecision::cos(val);}
    static  abs_value_t epsilon()    {return std::numeric_limits<value_t>::epsilon();}
    static abs_value_t abs(const value_t&v){return boost::multiprecision::abs(v);}
    static constexpr  bool is_real() {return true;}
    static constexpr  bool is_complex() {return false;}
    static bool almost_zero(const value_t&v)
    {
        static const abs_value_t tolerance=std::numeric_limits<value_t>::epsilon()*10;
        return abs(v)<=tolerance;
    }
    static bool almost_equal(const value_t&v1,const value_t&v2)
    {
        static const abs_value_t tolerance=std::numeric_limits<value_t>::epsilon()*16;
        abs_value_t delta=abs(v1-v2);
        return delta<=tolerance||
               delta<=std::max(abs(v1),abs(v2))*tolerance;
    }
    static auto random_uniform_distribution(value_t min,value_t max)
    {
        return [urd=std::uniform_real_distribution<double>{static_cast<double>(min),
                                                           static_cast<double>(max)}]()mutable
        {
            return value_t(urd(m_dre));
        };
    }
};
#endif
*/

#endif

