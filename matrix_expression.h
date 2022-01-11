
#ifndef  _matrix_expression_
#define  _matrix_expression_

//#include <iostream>
#include <assert.h>
#include <iterator>
#include <type_traits>
#include <functional>

#include "value_traits.h"

namespace lag{

using size_t=std::size_t;

struct scalar_t{};
struct row_t{};
struct col_t{};
struct matrix_t{};
struct undef_t{};

// Concepts
template<class T>
concept vector_concept=std::is_same_v<T,row_t>||
                       std::is_same_v<T,col_t>;

template<class T>
concept tensor_concept=vector_concept<T>||
                       std::is_same_v<T,matrix_t>||
                       std::is_same_v<T,scalar_t>;

template<class T>
concept intermediate_concept=requires
{
    typename T::intermediate_type_t;
};

template<class T,class I>
concept intermediate_define_concept =intermediate_concept<T>&&
std::is_same_v<typename T::intermediate_type_t,I>;

template<class T1,class T2>
concept possible_fold_concept=
intermediate_concept<T1>&&
intermediate_concept<T2>&&
std::is_same_v<typename T1::intermediate_type_t,
               typename T2::intermediate_type_t>;

template<intermediate_concept T1,intermediate_concept T2>
constexpr auto get_matrix_mul_type()
{
    if constexpr(intermediate_define_concept<T1,row_t>&&
                 intermediate_define_concept<T2,col_t>) // scalar dot
    {
        return scalar_t{};
    }
    else if constexpr(intermediate_define_concept<T1,col_t>&&
                      intermediate_define_concept<T2,row_t>) // outer product
    {
        return matrix_t{};
    }
    else if constexpr(intermediate_define_concept<T1,row_t>&&
                      intermediate_define_concept<T2,matrix_t>)
    {
        return row_t{};
    }
    else if constexpr(intermediate_define_concept<T1,matrix_t>&&
                      intermediate_define_concept<T2,col_t>)
    {
        return col_t{};
    }
     else if constexpr(intermediate_define_concept<T1,matrix_t>&&
                       intermediate_define_concept<T2,matrix_t>)
    {
        return matrix_t{};
    }
    else
    {
        return undef_t{};
    }
}

template<intermediate_concept T>
requires (!intermediate_define_concept<T,scalar_t>)
constexpr auto get_transposed_type()
{
    if constexpr(intermediate_define_concept<T,matrix_t>)
    {
        return matrix_t{};
    }
    else if constexpr(intermediate_define_concept<T,row_t>)
    {
        return col_t{};
    }
    else if constexpr(intermediate_define_concept<T,col_t>)
    {
        return row_t{};
    }
    else
    {
        return undef_t{};
    }
}

//vector iterator

template<class vector_t>
class vector_iterator
{
    public:
    using difference_type=int;
    private:
    vector_t m_vector;
    difference_type   m_position;
    public:
    using iterator=vector_iterator<vector_t>;
    using value_type=std::decay_t<decltype(m_vector(0))>;
    using reference=decltype(m_vector(0));
    using pointer=value_type*;
    using iterator_category=std::random_access_iterator_tag;
    explicit vector_iterator(vector_t vec,difference_type position=0):
    m_vector(vec),m_position(position)
    {}
    iterator begin()const {return *this;}
    iterator end()const {return iterator(m_vector,m_vector.size());}
    iterator& operator++()
    {
        ++m_position;
        return *this;
    }
    iterator& operator--()
    {
        --m_position;
        return *this;
    }
    iterator operator++(int)const
    {
        return iterator(m_vector,m_position+1);
    }
    iterator operator--(int)const
    {
        return iterator(m_vector,m_position-1);
    }
    reference operator[](difference_type n){ return m_vector(m_position+n);}
    iterator operator+(difference_type n)const
    {
        return iterator(m_vector,m_position+n);
    }
    iterator operator-(difference_type n)const
    {
        return iterator(m_vector,m_position-n);
    }
    difference_type operator-(const iterator&other)const
    {
        return m_position-other.m_position;
    }
    iterator&operator+=(difference_type n)
    {
        m_position+=n;
        return *this;
    }
    iterator&operator-=(difference_type n)
    {
        m_position-=n;
        return *this;
    }
    bool operator==(const iterator&other)const{return m_position==other.m_position;}
    bool operator!=(const iterator&other)const{return m_position!=other.m_position;}
    bool operator<(const iterator&other)const{return m_position<other.m_position;}
    bool operator<=(const iterator&other)const{return m_position<=other.m_position;}
    bool operator>(const iterator&other)const{return m_position>other.m_position;}
    bool operator>=(const iterator&other)const{return m_position>=other.m_position;}
    reference operator*(){return m_vector(m_position);}
};

template<class vector_t>
vector_iterator<vector_t> operator+(typename vector_iterator<vector_t>::difference_type n,
                                    const vector_iterator<vector_t>&iter)
{
    return iter+n;
}

template<class vector_t>
vector_iterator<vector_t> operator-(typename vector_iterator<vector_t>::difference_type n,
                                    const vector_iterator<vector_t>&iter)
{
    return iter-n;
}

// matrix iterator

template<class mx_t,class handler_t>
class matrix_iterator
{
    public:
    using difference_type=int;
    private:
    mx_t m_matrix;
    difference_type   m_row_pos=0;
    difference_type   m_col_pos=0;
    public:
    using iterator=matrix_iterator<mx_t,handler_t>;
    using value_type=std::decay_t<decltype(m_matrix(0,0))>;
    using reference=decltype(m_matrix(0,0));
    using pointer=value_type*;
    using iterator_category=handler_t::iterator_category;
    explicit matrix_iterator(mx_t mx,difference_type r=0,difference_type c=0):
    m_matrix(mx),m_row_pos(r),m_col_pos(c)
    {}
    iterator begin()const {return *this;}
    iterator end()const {return iterator(m_matrix);}
    iterator& operator++()
    {
        handler_t::increment(m_row_pos,m_col_pos,m_matrix.rows(),m_matrix.cols());
        return *this;
    }
    iterator& operator--()
    {
        handler_t::decrement(m_row_pos,m_col_pos,m_matrix.rows(),m_matrix.cols());
        return *this;
    }
    iterator operator++(int)const
    {
        difference_type r=m_row_pos;
        difference_type c=m_col_pos;
        handler_t::increment(r,c,m_matrix.rows(),m_matrix.cols());
        return iterator(m_matrix,r,c);
    }
    iterator operator--(int)const
    {
        difference_type r=m_row_pos;
        difference_type c=m_col_pos;
        handler_t::decrement(r,c,m_matrix.rows(),m_matrix.cols());
        return iterator(m_matrix,r,c);
    }
    reference operator[](difference_type n)
    {
        difference_type r=m_row_pos;
        difference_type c=m_col_pos;
        handler_t::advance(n,r,c,m_matrix.rows(),m_matrix.cols());
        return m_matrix(r,c);
    }
    iterator operator+(difference_type n)const
    {
        difference_type r=m_row_pos;
        difference_type c=m_col_pos;
        handler_t::advance(n,r,c,m_matrix.rows(),m_matrix.cols());
        return iterator(m_matrix,r,c);
    }
    iterator operator-(difference_type n)const
    {
        difference_type r=m_row_pos;
        difference_type c=m_col_pos;
        handler_t::advance(-n,r,c,m_matrix.rows(),m_matrix.cols());
        return iterator(m_matrix,r,c);
    }
    difference_type operator-(const iterator&other)const
    {
        return handler_t::difference(m_row_pos,m_col_pos,
                                    other.m_row_pos,other.m_col_pos,
                                    m_matrix.rows(),m_matrix.cols());
    }
    iterator&operator+=(difference_type n)
    {
        handler_t::advance(n,m_row_pos,m_col_pos,m_matrix.rows(),m_matrix.cols());
        return *this;
    }
    iterator&operator-=(difference_type n)
    {
        handler_t::advance(-n,m_row_pos,m_col_pos,m_matrix.rows(),m_matrix.cols());
        return *this;
    }
    bool operator==(iterator other)const
    {
        return m_row_pos==other.m_position&&m_col_pos==other.m_col_pos;
    }
    bool operator!=(iterator other)const
    {
        return m_row_pos!=other.m_position||m_col_pos!=other.m_col_pos;
    }
    bool operator<(const iterator&other)const
    {
        return handler_t::compare(m_row_pos,m_col_pos,
                                  other.m_row_pos,other.m_col_pos,
                                  m_matrix.rows(),m_matrix.cols())<0;
    }
    bool operator<=(const iterator&other)const
    {
        return handler_t::compare(m_row_pos,m_col_pos,
                                  other.m_row_pos,other.m_col_pos,
                                  m_matrix.rows(),m_matrix.cols())<=0;
    }
    bool operator>(const iterator&other)const{return other<*this;}
    bool operator>=(const iterator&other)const{return other<=*this;}
    reference operator*(){return m_matrix(m_row_pos,m_col_pos);}
    difference_type row()const{return m_row_pos;}
    difference_type col()const{return m_col_pos;}
};

template<class mx_t,class handler_t>
matrix_iterator<mx_t,handler_t> operator+(typename matrix_iterator<mx_t,handler_t>::difference_type n,
                                const matrix_iterator<mx_t,handler_t>&iter)
{
    return iter+n;
}

template<class mx_t,class handler_t>
matrix_iterator<mx_t,handler_t> operator-(typename matrix_iterator<mx_t,handler_t>::difference_type n,
                                const matrix_iterator<mx_t,handler_t>&iter)
{
    return iter-n;
}

struct row_iterator_handler_t
{
    using iterator_category=std::random_access_iterator_tag;
    using difference_type=int;
    static void increment(difference_type&row,difference_type&col,
                          difference_type total_row,difference_type total_col)
    {
        ++col;
        if(col==total_col)
        {
            col=0;
            ++row;
        }
    }
    static void decrement(difference_type&row,difference_type&col,
                          difference_type total_row,difference_type total_col)
    {
        if(col!=0)
        {
            --col;
        }
        else
        {
            col=total_col-1;
            --row;
        }
    }
    static void advance(difference_type n,difference_type&row,difference_type&col,
                        difference_type total_row,difference_type total_col)
    {
        row+=n/total_col;
        col+=n%total_col;
    }
    static difference_type difference(difference_type r1,difference_type c1,
                                      difference_type r2,difference_type c2,
                                      difference_type total_row,difference_type total_col)
    {
        return (r1-r2)*total_col+c1-c2;
    }
    static difference_type compare(difference_type r1,difference_type c1,
                                   difference_type r2,difference_type c2,
                                   difference_type total_row,difference_type total_col)
    {
        return (r1!=r2)? r1-r2: c1-c2;
    }
};

struct col_iterator_handler_t
{
    using iterator_category=std::random_access_iterator_tag;
    using difference_type=int;
    static void increment(difference_type&row,difference_type&col,
                          difference_type total_row,difference_type total_col)
    {
        ++row;
        if(row==total_row)
        {
            row=0;
            ++col;
        }
    }
    static void decrement(difference_type&row,difference_type&col,
                          difference_type total_row,difference_type total_col)
    {
        if(row!=0)
        {
            --row;
        }
        else
        {
            row=total_row-1;
            --col;
        }
    }
    static void advance(difference_type n,difference_type&row,difference_type&col,
                        difference_type total_row,difference_type total_col)
    {
        row+=n%total_row;
        col+=n/total_row;
    }
     static difference_type difference(difference_type r1,difference_type c1,
                                       difference_type r2,difference_type c2,
                                       difference_type total_row,difference_type total_col)
    {
        return (c1-c2)*total_row+r1-r2;
    }
    static difference_type compare(difference_type r1,difference_type c1,
                                   difference_type r2,difference_type c2,
                                   difference_type total_row,difference_type total_col)
    {
        return (c1!=c2)? c1-c2: r1-r2;
    }
};

////////////////////////////////////////////////////////////////////////////////
///                     base_intermediate_t                                 ////
////////////////////////////////////////////////////////////////////////////////

template<class T1,class T2>
concept possible_mul_concept=
intermediate_concept<T1>&&
intermediate_concept<T2>&&
(!std::is_same_v<decltype(get_matrix_mul_type<T1,T2>()),undef_t>);

template<class derived_t,tensor_concept tensor_t>
class base_intermediate_t
{
    using ipair_t=std::pair<size_t,size_t>;
    derived_t& m_this(){return static_cast<derived_t&>(*this);}
    const derived_t& m_this()const{return static_cast<const derived_t&>(*this);}
    inline static std::default_random_engine m_dre;
    public:
    //using intermediate_type_t=tensor_t;
    auto make_ref()const{return m_this();}

    auto T()const
    requires (!std::is_same_v<tensor_t,scalar_t>);
    auto T()
    requires (!std::is_same_v<tensor_t,scalar_t>);

    auto block(const ipair_t&rows,const ipair_t&cols)const
    requires std::is_same_v<tensor_t,matrix_t>;
    auto block(const ipair_t&rows,const ipair_t&cols)
    requires std::is_same_v<tensor_t,matrix_t>;

    auto block(size_t row,const ipair_t&cols)const
    requires std::is_same_v<tensor_t,matrix_t>;
    auto block(size_t row,const ipair_t&cols)
    requires std::is_same_v<tensor_t,matrix_t>;

    auto row(size_t i)const
    requires std::is_same_v<tensor_t,matrix_t>
    {return block(i,ipair_t{0,m_this().cols()});}
    auto row(size_t i)
    requires std::is_same_v<tensor_t,matrix_t>
    {return block(i,ipair_t{0,m_this().cols()});}

    auto block(const ipair_t&rows,size_t col)const
    requires std::is_same_v<tensor_t,matrix_t>;
    auto block(const ipair_t&rows,size_t col)
    requires std::is_same_v<tensor_t,matrix_t>;

    auto col(size_t i)const
    requires std::is_same_v<tensor_t,matrix_t>
    {return block(ipair_t{0,m_this().rows()},i);}
    auto col(size_t i)
    requires std::is_same_v<tensor_t,matrix_t>
    {return block(ipair_t{0,m_this().rows()},i);}

    auto block(const ipair_t&range)const
    requires vector_concept<tensor_t>;
    auto block(const ipair_t&range)
    requires vector_concept<tensor_t>;

    auto diag_row(int)const
    requires std::is_same_v<tensor_t,matrix_t>;
    auto diag_row(int)
    requires std::is_same_v<tensor_t,matrix_t>;

    auto diag_col(int)const
    requires std::is_same_v<tensor_t,matrix_t>;
    auto diag_col(int)
    requires std::is_same_v<tensor_t,matrix_t>;

    // Assign operations
    template<class value_t>
    void set_constant(value_t val)
    requires vector_concept<tensor_t>
    {
        for(size_t i=0;i<m_this().size();i++) m_this()(i)=val;
    }
    template<class value_t>
    void set_constant(value_t val)
    requires std::is_same_v<tensor_t,matrix_t>
    {
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++)
                m_this()(i,j)=val;
    }
    void set_identity()
    requires std::is_same_v<tensor_t,matrix_t>
    {
        assert(m_this().rows()==m_this().cols());
        set_zero();
        for(size_t i=0;i<m_this().rows();i++) m_this()(i,i)=1;
    }
    void set_zero() {set_constant(0);}

    template<class T>
    void set_random(T min,T max)
    requires std::is_same_v<tensor_t,matrix_t>
    {
        assert(min<max);
        using value_t=std::decay<decltype(m_this()(0,0))>::type;
        auto rand_gen=value_traits<value_t>::random_uniform_distribution(min,max);
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++)
                m_this()(i,j)=rand_gen();

    }
    template<class T>
    void set_random(T min,T max)
    requires vector_concept<tensor_t>
    {
        assert(min<max);
        using value_t=std::decay<decltype(m_this()(0))>::type;
        auto rand_gen=value_traits<value_t>::random_uniform_distribution(min,max);
        for(size_t i=0;i<m_this().size();i++) m_this()(i)=rand_gen();
    }

    //Scalar operations
    template<class value_t>
    void operator*=(value_t val)
    requires vector_concept<tensor_t>
    {
        for(size_t i=0;i<m_this().size();i++) m_this()(i)*=val;
    }
    template<class value_t>
    void operator*=(value_t val)
    requires std::is_same_v<tensor_t,matrix_t>
    {
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++)
                m_this()(i,j)*=val;
    }
    template<class value_t>
    void operator/=(value_t val)
    requires vector_concept<tensor_t>
    {
        for(size_t i=0;i<m_this().size();i++) m_this()(i)/=val;
    }
    template<class value_t>
    void operator/=(value_t val)
    requires std::is_same_v<tensor_t,matrix_t>
    {
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++)
                m_this()(i,j)/=val;
    }
    //Matrix/vector operations
    template<class rvalue_t>
    void assign(const rvalue_t& other) // for definition operator= in derived classes
    requires (possible_fold_concept<derived_t,rvalue_t>&&
              vector_concept<tensor_t>)
    {
        size_t s_other=other.size();
        assert(s_other==m_this().size());
        for(size_t i=0;i<s_other;i++) m_this()(i)=other(i);
    }
    template<class rvalue_t>
    void assign(const rvalue_t& other) // for definition operator= in derived classes
    requires (possible_fold_concept<derived_t,rvalue_t>&&
              std::is_same_v<tensor_t,matrix_t>)

    {
        size_t r_other=other.rows();
        size_t c_other=other.cols();
        assert(r_other==m_this().rows());
        assert(c_other==m_this().cols());
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++)
                m_this()(i,j)=other(i,j);
    }

    template<class rvalue_t>
    void operator+=(const rvalue_t&other)
    requires (possible_fold_concept<derived_t,rvalue_t>&&
              vector_concept<tensor_t>)
    {
        size_t s=other.size();
        assert(m_this().size()==s);
        for(size_t i=0;i<m_this().size();i++) m_this()(i)+=other(i);
    }
    template<class rvalue_t>
    void operator+=(const rvalue_t&other)
    requires (possible_fold_concept<derived_t,rvalue_t>&&
              std::is_same_v<tensor_t,matrix_t>)
    {
        size_t r_other=other.rows();
        size_t c_other=other.cols();
        assert(r_other==m_this().rows());
        assert(c_other==m_this().cols());
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++)
                m_this()(i,j)+=other(i,j);
    }
    template<class rvalue_t>
    void operator-=(const rvalue_t&other)
    requires (possible_fold_concept<derived_t,rvalue_t>&&
              vector_concept<tensor_t>)
    {
        size_t s=other.size();
        assert(m_this().size()==s);
        for(size_t i=0;i<m_this().size();i++) m_this()(i)-=other(i);
    }
    template<class rvalue_t>
    void operator-=(const rvalue_t&other)
    requires (possible_fold_concept<derived_t,rvalue_t>&&
              std::is_same_v<tensor_t,matrix_t>)
    {
        size_t r_other=other.rows();
        size_t c_other=other.cols();
        assert(r_other==m_this().rows());
        assert(c_other==m_this().cols());
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++)
                m_this()(i,j)-=other(i,j);
    }
    template<class rvalue_t>
    void swap(rvalue_t&&other)// no accept left value
    requires (possible_fold_concept<derived_t,std::decay_t<rvalue_t>>&&
              vector_concept<tensor_t>)
    {
        size_t s=other.size();
        assert(m_this().size()==s);
        for(size_t i=0;i<m_this().size();i++)
        {
            std::swap(m_this()(i),other(i));
        }
    }
    template<class rvalue_t>
    void swap(rvalue_t&&other)// no accept left value
    requires (possible_fold_concept<derived_t,std::decay_t<rvalue_t>>&&
              std::is_same_v<tensor_t,matrix_t>)
    {
        size_t r_other=other.rows();
        size_t c_other=other.cols();
        assert(r_other==m_this().rows());
        assert(c_other==m_this().cols());
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++)
            {
                std::swap(m_this()(i,j),other(i,j));
            }
    }
    static auto identity(size_t)
    requires std::is_same_v<tensor_t,matrix_t>;
    //mapping operations

    auto abs()const
    requires (!std::is_same_v<tensor_t,scalar_t>);
    template<class functor_t>
    auto map(functor_t func)const
    requires (!std::is_same_v<tensor_t,scalar_t>);

    // accumulate operations
    // sum
    auto sum()const
    requires std::is_same_v<tensor_t,matrix_t>
    {
        using value_t=std::decay<decltype(m_this()(0,0))>::type;
        value_t res=0;
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++) res+=m_this()(i,j);
        return res;
    }
    auto sum()const
    requires vector_concept<tensor_t>
    {
        using value_t=std::decay<decltype(m_this()(0))>::type;
        value_t res=0;
        for(size_t i=0;i<m_this().size();i++) res+=m_this()(i);
        return res;
    }
    // product
    auto product()const
    requires std::is_same_v<tensor_t,matrix_t>
    {
        using value_t=std::decay<decltype(m_this()(0,0))>::type;
        value_t res=1;
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++) res*=m_this()(i,j);
        return res;
    }
    auto product()const
    requires vector_concept<tensor_t>
    {
        using value_t=std::decay<decltype(m_this()(0))>::type;
        value_t res=1;
        for(size_t i=0;i<m_this().size();i++) res*=m_this()(i);
        return res;
    }

    auto min()const
    requires (std::is_same_v<tensor_t,matrix_t>)
    {
        using value_t=std::decay<decltype(m_this()(0,0))>::type;
        assert(m_this().rows()>0);
        assert(m_this().cols()>0);
        struct cursor_t{ value_t value; size_t row,col; };
        cursor_t result(m_this()(0,0),0,0);
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++)
            {
                value_t temp=m_this()(i,j);
                if(temp<result.value)
                {
                    result.value=temp;
                    result.row=i;result.col=j;
                }
            }
        return result;
    }
    auto min()const
    requires vector_concept<tensor_t>
    {
        using value_t=std::decay<decltype(m_this()(0))>::type;
        assert(m_this().size()>0);
        struct cursor_t{ value_t value; size_t pos; };
        cursor_t result(m_this()(0),0);
        for(size_t i=0;i<m_this().size();i++)
        {
            value_t temp=m_this()(i);
            if(temp<result.value)
            {
                result.value=temp;
                result.pos=i;
            }
        }
        return result;
    }

    auto max()const
    requires (std::is_same_v<tensor_t,matrix_t>)
    {
        using value_t=std::decay<decltype(m_this()(0,0))>::type;
        assert(m_this().rows()>0);
        assert(m_this().cols()>0);
        struct cursor_t{ value_t value; size_t row,col; };
        cursor_t result(m_this()(0,0),0,0);
        for(size_t i=0;i<m_this().rows();i++)
            for(size_t j=0;j<m_this().cols();j++)
            {
                value_t temp=m_this()(i,j);
                if(temp>result.value)
                {
                    result.value=temp;
                    result.row=i;result.col=j;
                }
            }
        return result;
    }
    auto max()const
    requires vector_concept<tensor_t>
    {
        using value_t=std::decay<decltype(m_this()(0))>::type;
        assert(m_this().size()>0);
        struct cursor_t{ value_t value; size_t pos; };
        cursor_t result(m_this()(0),0);
        for(size_t i=0;i<m_this().size();i++)
        {
            value_t temp=m_this()(i);
            if(temp>result.value)
            {
                result.value=temp;
                result.pos=i;
            }
        }
        return result;
    }
    template<class stream_t>
    void out(stream_t&stream)const
    requires vector_concept<tensor_t>
    {
        stream<<'{';
        for(size_t i=0;i<m_this().size();i++) stream<<m_this()(i)<<',';
        stream<<'}';
    }
    template<class stream_t>
    void out(stream_t&stream)const
    requires (std::is_same_v<tensor_t,matrix_t>)
    {
        for(size_t i=0;i<m_this().rows();i++)
        {
            for(size_t j=0;j<m_this().cols();j++) stream<<m_this()(i,j)<<' ';
            stream<<'\n';
        }
    }
    //iterators
    auto iterator()const
    requires vector_concept<tensor_t>
    {
        return vector_iterator(m_this().make_ref());
    }
    auto iterator()
    requires vector_concept<tensor_t>
    {
        return vector_iterator(m_this().make_ref());
    }
    auto row_iterator()const
    requires tensor_concept<tensor_t>
    {
        return matrix_iterator<decltype(m_this().make_ref()),row_iterator_handler_t>(m_this().make_ref());
    }
    auto row_iterator()
    requires tensor_concept<tensor_t>
    {
        return matrix_iterator<decltype(m_this().make_ref()),row_iterator_handler_t>(m_this().make_ref());
    }
    auto col_iterator()const
    requires tensor_concept<tensor_t>
    {
        return matrix_iterator<decltype(m_this().make_ref()),col_iterator_handler_t>(m_this().make_ref());
    }
    auto col_iterator()
    requires tensor_concept<tensor_t>
    {
        return matrix_iterator<decltype(m_this().make_ref()),col_iterator_handler_t>(m_this().make_ref());
    }
};

// Transposed ( may be left value )

template<intermediate_concept sub_exp_t>
class transposed_exp_t:public
base_intermediate_t<transposed_exp_t<sub_exp_t>,
                    decltype(get_transposed_type<sub_exp_t>())>
{
    using base_intermediate_t<transposed_exp_t<sub_exp_t>,
                    decltype(get_transposed_type<sub_exp_t>())>::T;
    using self_t=transposed_exp_t<sub_exp_t>;
    public:
    using value_t=sub_exp_t::value_t;
    using intermediate_type_t=decltype(get_transposed_type<sub_exp_t>());
    sub_exp_t m_sub_exp;
    constexpr transposed_exp_t(sub_exp_t _exp):m_sub_exp(_exp){}

    size_t rows()const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        return m_sub_exp.cols();
    }
    size_t cols()const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        return m_sub_exp.rows();
    }
    size_t size()const
    {
        return m_sub_exp.size();
    }
    decltype(auto) operator()(size_t i)const
    requires vector_concept<intermediate_type_t>
    {
        return m_sub_exp(i);
    }
    decltype(auto) operator()(size_t i,size_t j)const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        return m_sub_exp(j,i);
    }
    sub_exp_t T()const { return m_sub_exp;}// redefinition base_t::T
};

// Matrix block (may be left value)

template<intermediate_define_concept<matrix_t> full_matrix_t>
struct matrix_block_t:
public base_intermediate_t<matrix_block_t<full_matrix_t>,matrix_t>
{
    using self_t=matrix_block_t<full_matrix_t>;
    using ipair_t=std::pair<size_t,size_t>;
    using value_t=full_matrix_t::value_t;
    using intermediate_type_t=matrix_t;
    full_matrix_t m_source;
    size_t m_pivot_row,m_pivot_col;
    size_t m_rows,m_cols;
    constexpr matrix_block_t(full_matrix_t _mx,
                             ipair_t rows,
                             ipair_t cols):
    m_source(_mx),
    m_pivot_row(rows.first),m_pivot_col(cols.first),
    m_rows(rows.second-rows.first),
    m_cols(cols.second-cols.first)
    {
        assert(m_rows>0);
        assert(m_cols>0);
    }
    size_t rows()const{return m_rows;}
    size_t cols()const{return m_cols;}
    size_t size()const{return m_cols*m_rows;}
    decltype(auto) operator()(size_t i,size_t j)const
    {
        return m_source(m_pivot_row+i,m_pivot_col+j);
    }
};

// Matrix subvector (may be left value)

template<vector_concept vector_t,
         intermediate_define_concept<matrix_t> full_matrix_t>
struct matrix_subvector_t:
public base_intermediate_t<matrix_subvector_t<vector_t,full_matrix_t>,vector_t>
{
    using self_t=matrix_subvector_t<vector_t,full_matrix_t>;
    using value_t=full_matrix_t::value_t;
    using intermediate_type_t=vector_t;
    full_matrix_t m_source;
    size_t m_pivot_row,m_pivot_col,m_size;
    constexpr matrix_subvector_t(full_matrix_t _mx,size_t pivot_row,size_t pivot_col,size_t size):
    m_source(_mx),
    m_pivot_row(pivot_row),m_pivot_col(pivot_col),m_size(size)
    {
        assert(m_size>0);
    }
    size_t size()const{return m_size;}
    decltype(auto) operator()(size_t i)const
    {
        if constexpr(std::is_same_v<vector_t,col_t>)
        {
            return m_source(m_pivot_row+i,m_pivot_col);
        }
        else
        {
            return m_source(m_pivot_row,m_pivot_col+i);
        }
    }
};

// Matrix diagonal

template<intermediate_define_concept<matrix_t> full_matrix_t,
         vector_concept vector_t>
struct matrix_diagonal_t:
public base_intermediate_t<matrix_diagonal_t<full_matrix_t,vector_t>,vector_t>
{
    using self_t=matrix_diagonal_t<full_matrix_t,vector_t>;
    using value_t=full_matrix_t::value_t;
    using intermediate_type_t=vector_t;
    full_matrix_t m_source;
    int m_diagonal;
    std::size_t m_size;
    constexpr matrix_diagonal_t(full_matrix_t _mx,int dg):
    m_source(_mx),
    m_diagonal(dg),
    m_size(std::min(_mx.rows(),_mx.cols())-((dg<0)?-dg:dg))
    {
        assert(m_size>0);
    }
    size_t size()const{return m_size;}
    decltype(auto) operator()(size_t i)const
    {
        return (m_diagonal>=0)? m_source(i,i+m_diagonal):
                                m_source(i-m_diagonal,i);
    }
};

//Subvector (may be left value)

template<class vector_t>
requires (intermediate_define_concept<vector_t,row_t>||
          intermediate_define_concept<vector_t,col_t>)
struct subvector_t:
public base_intermediate_t<subvector_t<vector_t>,typename vector_t::intermediate_type_t>
{
    using self_t=subvector_t<vector_t>;
    using value_t=vector_t::value_t;
    using intermediate_type_t=vector_t::intermediate_type_t;
    vector_t m_source;
    size_t m_pivot_inx;
    size_t m_size;
    subvector_t(vector_t source,size_t pivot,size_t size):
    m_source(source),m_pivot_inx(pivot),m_size(size)
    {
        assert(m_size>0);
    }
    size_t size()const{return m_size;}
    decltype(auto) operator()(size_t i)const
    {return m_source(m_pivot_inx+i);}
};

//T()

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::T()const
requires (!std::is_same_v<tensor_t,scalar_t>)
{
    return transposed_exp_t(m_this().make_ref());
}

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::T()
requires (!std::is_same_v<tensor_t,scalar_t>)
{
    return transposed_exp_t(m_this().make_ref());
}

//block(...)

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::block(const ipair_t&rows,const ipair_t&cols)const
requires std::is_same_v<tensor_t,matrix_t>
{
    return matrix_block_t(m_this().make_ref(),rows,cols);
}

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::block(const ipair_t&rows,const ipair_t&cols)
requires std::is_same_v<tensor_t,matrix_t>
{
    return matrix_block_t(m_this().make_ref(),rows,cols);
}

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::block(size_t row,const ipair_t&cols)const
requires std::is_same_v<tensor_t,matrix_t>
{
    return matrix_subvector_t<row_t,decltype(m_this().make_ref())>
           (m_this().make_ref(),row,cols.first,cols.second-cols.first);
}

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::block(size_t row,const ipair_t&cols)
requires std::is_same_v<tensor_t,matrix_t>
{
    return matrix_subvector_t<row_t,decltype(m_this().make_ref())>
           (m_this().make_ref(),row,cols.first,cols.second-cols.first);
}

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::block(const ipair_t&rows,size_t col)const
requires std::is_same_v<tensor_t,matrix_t>
{
    return matrix_subvector_t<col_t,decltype(m_this().make_ref())>
            (m_this().make_ref(),rows.first,col,rows.second-rows.first);
}

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::block(const ipair_t&rows,size_t col)
requires std::is_same_v<tensor_t,matrix_t>
{
    return matrix_subvector_t<col_t,decltype(m_this().make_ref())>
            (m_this().make_ref(),rows.first,col,rows.second-rows.first);
}

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::block(const ipair_t&range)const
requires vector_concept<tensor_t>
{
    return subvector_t(m_this().make_ref(),range.first,range.second-range.first);
}

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::block(const ipair_t&range)
requires vector_concept<tensor_t>
{
    return subvector_t(m_this().make_ref(),range.first,range.second-range.first);
}

// Diagonals

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::diag_row(int i)const
requires std::is_same_v<tensor_t,matrix_t>
{
    return matrix_diagonal_t<decltype(m_this().make_ref()),row_t>(m_this().make_ref(),i);
}

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::diag_row(int i)
requires std::is_same_v<tensor_t,matrix_t>
{
    return matrix_diagonal_t<decltype(m_this().make_ref()),row_t>(m_this().make_ref(),i);
}

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::diag_col(int i)const
requires std::is_same_v<tensor_t,matrix_t>
{
    return matrix_diagonal_t<decltype(m_this().make_ref()),col_t>(m_this().make_ref(),i);
}

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::diag_col(int i)
requires std::is_same_v<tensor_t,matrix_t>
{
    return matrix_diagonal_t<decltype(m_this().make_ref()),col_t>(m_this().make_ref(),i);
}


// Functors (right value only)

template<class functor_t,vector_concept vector_t>
struct unary_functor_exp_t:
public base_intermediate_t<unary_functor_exp_t<functor_t,vector_t>,vector_t>
{
    using intermediate_type_t=vector_t;
    size_t m_size;
    functor_t m_functor;
    using value_t=decltype(m_functor(0));
    explicit unary_functor_exp_t(size_t s,functor_t functor):
    m_size(s),m_functor(functor){}
    size_t size()const{return m_size;}
    value_t operator()(size_t i)const{return m_functor(i);}
};

template<vector_concept vector_t,class functor_t>
unary_functor_exp_t<functor_t,vector_t>
functor(size_t size,functor_t f)
{
    return functor_exp_t<vector_t,functor_t>(size,f);
}

template<class functor_t>
struct binary_functor_exp_t:
public base_intermediate_t<binary_functor_exp_t<functor_t>,matrix_t>
{
    using intermediate_type_t=matrix_t;
    size_t m_rows,m_cols;
    functor_t m_functor;
    using value_t=decltype(m_functor(0,0));
    explicit binary_functor_exp_t(size_t r,size_t c,functor_t functor):
    m_rows(r),m_cols(c),m_functor(functor){}
    size_t size()const{return m_rows*m_cols;}
    size_t rows()const{return m_rows;}
    size_t cols()const{return m_cols;}
    value_t operator()(size_t i,size_t j)const{return m_functor(i,j);}
};

template<class functor_t>
binary_functor_exp_t<functor_t>
functor(size_t rows,size_t cols,functor_t f)
{
    return binary_functor_exp_t(rows,cols,f);
}

template<class derived_t,tensor_concept tensor_t>
auto
base_intermediate_t<derived_t,tensor_t>::identity(size_t i)
requires std::is_same_v<tensor_t,matrix_t>
{
    return functor(i,i,[](size_t i,size_t j){return (i==j)? 1:0;});
}

//  Unary operations (rvalue only)

template<class sub_exp_t,class map_t>
struct unary_op_t:
public base_intermediate_t<unary_op_t<sub_exp_t,map_t>,
                           typename sub_exp_t::intermediate_type_t>
{
    using sub_value_t=sub_exp_t::value_t;
    using intermediate_type_t=sub_exp_t::intermediate_type_t;
    sub_exp_t m_exp;
    map_t m_map;
    using value_t=decltype(m_map(std::declval<sub_value_t>()));
    constexpr unary_op_t(sub_exp_t exp_1,map_t _map):
    m_exp(exp_1),m_map(_map)
    {
    }
    size_t rows()const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        return m_exp.rows();
    }
    size_t cols()const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        return m_exp.cols();
    }
    size_t size()const
    requires std::is_same_v<intermediate_type_t,matrix_t>||
             std::is_same_v<intermediate_type_t,col_t>||
             std::is_same_v<intermediate_type_t,row_t>
    {
        return m_exp.size();
    }
    value_t operator()(size_t i)const
    requires std::is_same_v<intermediate_type_t,col_t>||
             std::is_same_v<intermediate_type_t,row_t>
    {
        return  m_map(m_exp(i));
    }
    value_t operator()(size_t i,size_t j)const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        return m_map(m_exp(i,j));
    }
};

// Mapping functions

template<class derived_t,tensor_concept tensor_t>
auto base_intermediate_t<derived_t,tensor_t>::abs()const
requires (!std::is_same_v<tensor_t,scalar_t>)
{
    if constexpr(vector_concept<tensor_t>)
    {
        return unary_op_t(m_this().make_ref(),
               value_traits<std::decay_t<decltype(m_this()(0))>>::abs);
    }
    else
    {
        return unary_op_t(m_this().make_ref(),
               value_traits<std::decay_t<decltype(m_this()(0,0))>>::abs);
    }
}

template<class derived_t,tensor_concept tensor_t>
template<class functor_t>
auto base_intermediate_t<derived_t,tensor_t>::map(functor_t func)const
requires (!std::is_same_v<tensor_t,scalar_t>)
{
    return unary_op_t(m_this().make_ref(),func);
}

// Scaling (right value only)

template<intermediate_concept sub_exp_t,bool is_mul>
struct scalable_exp_t:
public base_intermediate_t<scalable_exp_t<sub_exp_t,is_mul>,
                           typename sub_exp_t::intermediate_type_t>
{
    using value_t=sub_exp_t::value_t;
    using intermediate_type_t=sub_exp_t::intermediate_type_t;
    sub_exp_t m_exp;
    value_t m_factor;
    constexpr scalable_exp_t(sub_exp_t exp_,value_t val):
    m_exp(exp_),m_factor(val){}
    size_t rows()const
    requires intermediate_define_concept<sub_exp_t,matrix_t>
    {
        return m_exp.rows();
    }
    size_t cols()const
    requires intermediate_define_concept<sub_exp_t,matrix_t>
    {
        return m_exp.cols();
    }
    size_t size()const
    requires (!std::is_same_v<intermediate_type_t,scalar_t>)
    {
        return m_exp.size();
    }
    value_t operator()(size_t i)const
    requires vector_concept<intermediate_type_t>
    {
        if constexpr(is_mul) return m_exp(i)*m_factor;
        else                 return m_exp(i)/m_factor;
    }
    value_t operator()(size_t i,size_t j)const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        if constexpr(is_mul) return m_exp(i,j)*m_factor;
        else                 return m_exp(i,j)/m_factor;
    }
    operator value_t()const
    requires std::is_same_v<intermediate_type_t,scalar_t>
    {
         if constexpr(is_mul) return static_cast<value_t>(m_exp)*m_factor;
         else                 return static_cast<value_t>(m_exp)/m_factor;
    }
};

template<intermediate_concept exp_t>
constexpr auto operator*(const exp_t&exp_,typename exp_t::value_t v)
{
    return scalable_exp_t<decltype(exp_.make_ref()),true>(exp_.make_ref(),v);
}

template<intermediate_concept exp_t>
constexpr auto operator*(typename exp_t::value_t v,const exp_t&exp_)
{
    return scalable_exp_t<decltype(exp_.make_ref()),true>(exp_.make_ref(),v);
}

// vector*(1/b) != vector/b in common case (int for sample)

template<intermediate_concept exp_t>
constexpr auto operator/(const exp_t&exp_,typename exp_t::value_t v)
{
    return scalable_exp_t<decltype(exp_.make_ref()),false>(exp_.make_ref(),v);
}

// Adding and substraction (right value only)

template<class sub_exp_1_t,class sub_exp_2_t,class op_t>
struct binary_op_t:
public base_intermediate_t<binary_op_t<sub_exp_1_t,sub_exp_2_t,op_t>,
                           typename sub_exp_1_t::intermediate_type_t>
{
    using value_t=sub_exp_1_t::value_t;
    using intermediate_type_t=sub_exp_1_t::intermediate_type_t;
    sub_exp_1_t m_exp_1;
    sub_exp_2_t m_exp_2;
    op_t m_op;
    constexpr binary_op_t(sub_exp_1_t exp_1,sub_exp_2_t exp_2,op_t op):
    m_exp_1(exp_1),m_exp_2(exp_2),m_op(op)
    {
        static_assert(std::is_same_v<typename sub_exp_1_t::intermediate_type_t,
                                     typename sub_exp_2_t::intermediate_type_t>);
        if constexpr(std::is_same_v<intermediate_type_t,row_t>||
                     std::is_same_v<intermediate_type_t,col_t>)
        {
            assert(m_exp_1.size()==m_exp_2.size());
        }
        if constexpr(std::is_same_v<intermediate_type_t,matrix_t>)
        {
            assert(m_exp_1.rows()==m_exp_2.rows());
            assert(m_exp_1.cols()==m_exp_2.cols());
        }
    }
    size_t rows()const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        return m_exp_1.rows();
    }
    size_t cols()const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        return m_exp_1.cols();
    }
    size_t size()const
    requires (!std::is_same_v<intermediate_type_t,scalar_t>)
    {
        return m_exp_1.size();
    }
    value_t operator()(size_t i)const
    requires vector_concept<intermediate_type_t>
    {
        return  m_op(m_exp_1(i),m_exp_2(i));
    }
    value_t operator()(size_t i,size_t j)const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        return m_op(m_exp_1(i,j),m_exp_2(i,j));
    }
    operator value_t()const
    requires std::is_same_v<intermediate_type_t,scalar_t>
    {
        return m_op(static_cast<value_t>(m_exp_1),
                    static_cast<value_t>(m_exp_2));
    }
};

template<class T1,class T2>
requires possible_fold_concept<T1,T2>
constexpr auto operator+(const T1&_1,const T2&_2)

{
    return binary_op_t(_1.make_ref(),_2.make_ref(),std::plus<>{});
}

template<class T1,class T2>
requires possible_fold_concept<T1,T2>
constexpr auto operator-(const T1&_1,const T2&_2)

{
    return binary_op_t(_1.make_ref(),_2.make_ref(),std::minus<>{});
}

template<class T1,class T2,class functor_t>
requires possible_fold_concept<T1,T2>
constexpr auto map(const T1&_1,const T2&_2,functor_t func)
{
    return binary_op_t(_1.make_ref(),_2.make_ref(),func);
}

// Matrix multiplying (right value only)

template<class sub_exp_1_t,class sub_exp_2_t>
struct matrix_mul_t:
public base_intermediate_t<matrix_mul_t<sub_exp_1_t,sub_exp_2_t>,
                           decltype(get_matrix_mul_type<sub_exp_1_t,
                                                        sub_exp_2_t>())>
{
    using size_t=std::size_t;
    using value_t=sub_exp_1_t::value_t;
    using intermediate_type_t=decltype(get_matrix_mul_type<sub_exp_1_t,
                                                           sub_exp_2_t>());
    sub_exp_1_t m_exp_1;
    sub_exp_2_t m_exp_2;
    size_t m_optional_size;
    constexpr matrix_mul_t(sub_exp_1_t exp_1,sub_exp_2_t exp_2):
    m_exp_1(exp_1),m_exp_2(exp_2)
    {
        if constexpr(intermediate_define_concept<sub_exp_1_t,row_t>&&
                     intermediate_define_concept<sub_exp_2_t,col_t>) // scalar dot
        {
            assert(m_exp_1.size()==m_exp_2.size());
            m_optional_size=1; // no using
        }
        if constexpr(intermediate_define_concept<sub_exp_1_t,row_t>&&
                     intermediate_define_concept<sub_exp_2_t,matrix_t>)
        {
            assert(m_exp_1.size()==m_exp_2.rows());
            m_optional_size=m_exp_2.cols();
        }
        if constexpr(intermediate_define_concept<sub_exp_1_t,matrix_t>&&
                     intermediate_define_concept<sub_exp_2_t,col_t>)
        {
            assert(m_exp_1.cols()==m_exp_2.size());
            m_optional_size=m_exp_1.rows();
        }
        if constexpr(intermediate_define_concept<sub_exp_1_t,matrix_t>&&
                     intermediate_define_concept<sub_exp_2_t,matrix_t>)
        {
            assert(m_exp_1.cols()==m_exp_2.rows());
            m_optional_size=m_exp_1.rows()*m_exp_2.cols();
        }
    }
    size_t rows()const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        if constexpr(intermediate_define_concept<sub_exp_1_t,col_t>)
             {return m_exp_1.size();}
        else {return m_exp_1.rows();}
    }
    size_t cols()const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        if constexpr(intermediate_define_concept<sub_exp_1_t,col_t>)
             {return m_exp_2.size();}
        else {return m_exp_2.cols();}
    }
    size_t size()const
    requires (!std::is_same_v<intermediate_type_t,scalar_t>)
    {
        return m_optional_size;
    }
    value_t operator()(size_t i)const
    requires vector_concept<intermediate_type_t>
    {
        value_t res(0);
        if constexpr(std::is_same_v<intermediate_type_t,row_t>)
        {
            size_t _rows=m_exp_2.rows();
            for(size_t j=0;j<_rows;++j) res+=m_exp_1(j)*m_exp_2(i,j);
        }
        if constexpr(std::is_same_v<intermediate_type_t,col_t>)
        {
            size_t _cols=m_exp_1.cols();
            for(size_t j=0;j<_cols;++j) res+=m_exp_1(i,j)*m_exp_2(j);
        }
        return res;
    }
    value_t operator()(size_t i,size_t j)const
    requires std::is_same_v<intermediate_type_t,matrix_t>
    {
        if constexpr(intermediate_define_concept<sub_exp_1_t,col_t>)
            {return m_exp_1(i)*m_exp_2(j);}
        else
        {
            value_t res(0);
            size_t _cols=m_exp_1.cols();
            for(size_t k=0;k<_cols;++k) res+=m_exp_1(i,k)*m_exp_2(k,j);
            return res;
        }
    }
    operator value_t()const
    requires std::is_same_v<intermediate_type_t,scalar_t>
    {
        value_t res(0);
        size_t _size=m_exp_1.size();
        for(size_t j=0;j<_size;++j) res+=m_exp_1(j)*m_exp_2(j);
        return res;
    }
};

template<intermediate_concept exp_1_t,
         intermediate_concept exp_2_t>
constexpr auto operator*(const exp_1_t&_1,const exp_2_t&_2)
requires possible_mul_concept<exp_1_t,exp_2_t>
{
    return matrix_mul_t(_1.make_ref(),_2.make_ref());
}


// Wrappers for external vector-like and matrix-like types

template<class source_t,vector_concept variance_t>
class vector_ref_t:
public base_intermediate_t<vector_ref_t<source_t,variance_t>,
                           variance_t>
{
    using self_t=vector_ref_t<source_t,variance_t>;
    source_t&m_source;
    size_t   m_size;
    public:
    using intermediate_type_t=variance_t;
    constexpr vector_ref_t(source_t&s,size_t i):
    m_source(s),m_size(i)
    {}
    self_t&operator=(const self_t&rvalue)
    {
        if(&m_source==&rvalue.m_source) return *this;
        assert(m_size==rvalue.size());
        this->assign(rvalue);
        return *this;
    }
    template<class rvalue_t>
    self_t&operator=(const rvalue_t&rvalue)
    requires possible_fold_concept<rvalue_t,self_t>
    {
        assert(m_size==rvalue.size());
        this->assign(rvalue);
        return *this;
    }
    size_t size()const{return m_size;}

    decltype(auto) operator()(size_t i) const
    requires requires (source_t s){s[0];}
    {return m_source[i];}

    decltype(auto) operator()(size_t i) const
    requires ( requires (source_t s){s(0);}&&
              !requires (source_t s){s[0];} )
    {return m_source(i);}

    using value_t=std::decay_t<decltype(std::declval<self_t>()(0))>;
};

template<class source_t>
using vector_value_t=vector_ref_t<source_t,col_t>::value_t;

template<class source_t>
auto as_row(source_t&source,std::size_t size)
{
    return vector_ref_t<source_t,row_t>(source,size);
}

template<class source_t>
auto as_row(source_t&source)
{
    return vector_ref_t<source_t,row_t>(source,std::size(source));
}

template<class source_t>
auto as_col(source_t&source,std::size_t size)
{
    return vector_ref_t<source_t,col_t>(source,size);
}

template<class source_t>
auto as_col(source_t&source)
{
    return vector_ref_t<source_t,col_t>(source,std::size(source));
}

template<class source_t>
class matrix_ref_t:
public base_intermediate_t<matrix_ref_t<source_t>,matrix_t>
{
    using self_t=matrix_ref_t<source_t>;
    source_t&m_source;
    size_t   m_rows,m_cols;
    public:
    using intermediate_type_t=matrix_t;
    constexpr matrix_ref_t(source_t&s,size_t rows,size_t cols):
    m_source(s),m_rows(rows),m_cols(cols)
    {}
    self_t&operator=(const self_t&rvalue)
    {
        if(&m_source==&rvalue.m_source) return *this;
        assert(m_rows==rvalue.rows()&&m_cols==rvalue.cols());
        this->assign(rvalue);
        return *this;
    }
    template<class rvalue_t>
    self_t&operator=(const rvalue_t&rvalue)
    requires possible_fold_concept<rvalue_t,self_t>
    {
        assert(m_rows==rvalue.rows()&&m_cols==rvalue.cols());
        this->assign(rvalue);
        return *this;
    }
    size_t rows()const{return m_rows;}
    size_t cols()const{return m_cols;}

    decltype(auto) operator()(size_t i,size_t j) const
    requires requires (source_t s){s[0][0];}
    {return m_source[i][j];}

    decltype(auto) operator()(size_t i,size_t j) const
    requires ( requires (source_t s){s(0,0);}&&
              !requires (source_t s){s[0][0];} )
    {return m_source(i,j);}

    using value_t=std::decay_t<decltype(std::declval<self_t>()(0,0))>;
};

template<class source_t>
using matrix_value_t=matrix_ref_t<source_t>::value_t;

template<class source_t>
auto as_matrix(source_t&source,std::size_t rows,std::size_t cols)
{
    return matrix_ref_t<source_t>(source,rows,cols);
}

template<class source_t>
auto as_matrix(source_t&source)
{
    return matrix_ref_t<source_t>(source,source.rows(),source.cols());
}

template<class value_t,int rows,int cols>
auto as_matrix(value_t(&source)[rows][cols])
{
    return matrix_ref_t(source,rows,cols);
}

// Base matrix/vector expression

template<class derived_t,class val_t,vector_concept variance_t>
class base_vector_expression_t:
public base_intermediate_t<base_vector_expression_t<derived_t,val_t,variance_t>,
                           variance_t>
{
    using self_t=base_vector_expression_t<derived_t,val_t,variance_t>;
    using base_intermediate_t<self_t,variance_t>::make_ref;// will be redefine
    derived_t& m_this(){return static_cast<derived_t&>(*this);}
    const derived_t& m_this()const{return static_cast<const derived_t&>(*this);}
    public:
    using value_t=val_t;
    using intermediate_type_t=variance_t;
    base_vector_expression_t(){}
    size_t size()const{return m_this().size();}
    value_t operator()(size_t i)const{return m_this()(i);}
    value_t&operator()(size_t i){return m_this()(i);}
    auto make_ref()const
    {
        if constexpr(std::is_same_v<variance_t,row_t>)
             return as_row(m_this(),size());
        else return as_col(m_this(),size());
    }
    auto make_ref()
    {
        if constexpr(std::is_same_v<variance_t,row_t>)
             return as_row(m_this(),size());
        else return as_col(m_this(),size());
    }
};

template<class derived_t,class val_t>
class base_matrix_expression_t:
public base_intermediate_t<base_matrix_expression_t<derived_t,val_t>,
                           matrix_t>
{
    using self_t=base_matrix_expression_t<derived_t,val_t>;
    using base_intermediate_t<self_t,matrix_t>::make_ref;// will be redefine
    derived_t& m_this(){return static_cast<derived_t&>(*this);}
    const derived_t& m_this()const{return static_cast<const derived_t&>(*this);}
    public:
    using value_t=val_t;
    size_t rows()const{return m_this().rows();}
    size_t cols()const{return m_this().cols();}
    value_t operator()(size_t i,size_t j)const{return m_this()(i,j);}
    value_t&operator()(size_t i,size_t j){return m_this()(i,j);}
    using intermediate_type_t=matrix_t;
    base_matrix_expression_t(){}
    auto make_ref()const
    {
        return as_matrix(m_this(),rows(),cols());//redefinition
    }
    auto make_ref()
    {
        return as_matrix(m_this(),rows(),cols());//redefinition
    }
};

// Special matrix

template<class value_t>
auto identity(int size)
{
    static auto lam=[](int i,int j)->value_t
    {return (i==j)? value_traits<value_t>::one():value_traits<value_t>::zero();};
    return as_matrix(lam,size,size);
}

template<class vector_t>
auto diagonal_matrix(vector_t&&diag)
{
    using value_t=std::decay_t<decltype(diag(0))>;
    static auto lam=[ref=diag.make_ref()](int i,int j)->value_t
    {return (i==j)? ref(i):value_traits<value_t>::zero();};
    return as_matrix(lam,diag.size(),diag.size());
}

template<class value_t>
auto hilbert(int size)
{
    static auto lam=[](int i,int j)->value_t
    {return value_traits<value_t>::one()/value_traits<value_t>::integer(i+j+1);};
    return as_matrix(lam,size,size);
}

}//lag


#endif

