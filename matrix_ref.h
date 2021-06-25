
#ifndef  _matrix_ref_
#define  _matrix_ref_

#define LAG_ALWAYS_INLINE __attribute__((always_inline)) inline

// Matrix reference

namespace lag{

namespace _detail{

template <class matrix_t>
concept matrix_concept=requires(matrix_t&mx)
{
    mx[0][0];
};

template <class functor_t>
concept bi_functor_concept=requires(functor_t&ftr)
{
    ftr(0,0);
};

template <class matrix_like_t>
concept matrix_like_concept=matrix_concept<matrix_like_t>||
                            bi_functor_concept<matrix_like_t>;


}// _detail

template<_detail::matrix_like_concept matrix_t>
class matrix_ref_t
{
    matrix_t&m_mx;
    public:
    constexpr matrix_ref_t(matrix_t&mx):m_mx(mx){}
    LAG_ALWAYS_INLINE
    auto& operator()(std::size_t i,std::size_t j) const
    {
        if constexpr(_detail::matrix_concept<matrix_t>)
        {
            return m_mx[i][j];
        }
        else
        {
            return m_mx(i,j);
        }
    }
};

template<_detail::matrix_like_concept matrix_t>
class const_matrix_ref_t
{
    const matrix_t&m_mx;
    public:
    constexpr const_matrix_ref_t(const matrix_t&mx):m_mx(mx){}
    LAG_ALWAYS_INLINE
    auto operator()(std::size_t i,std::size_t j)const
    {
        if constexpr(_detail::matrix_concept<matrix_t>)
        {
            return m_mx[i][j];
        }
        else
        {
            return m_mx(i,j);
        }
    }
};

}//lag

#endif

