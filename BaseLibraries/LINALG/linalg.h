
#ifndef  _linalg_
#define  _linalg_

#include <assert.h>
#include <utility>

template<class visitor_t>
void rows_visit(visitor_t visitor,std::size_t rows,std::size_t cols)
{
    using size_t=std::size_t;
    for(size_t i=0;i<rows;++i)
    {
        for(size_t j=0;j<cols;++j)
        {
            visitor(i,j);
        }
    }
}

template<class visitor_t>
void columns_visit(visitor_t visitor,std::size_t rows,std::size_t cols)
{
    for(size_t i=0;i<cols;++i)
    {
        for(size_t j=0;j<rows;++j)
        {
            visitor(j,i);
        }
    }
}

template<class matrix_t1,class matrix_t2>
auto fn_matrix_to_matrix(const matrix_t1& fst,const matrix_t2& snd,std::size_t row_2 )
{
    using size_t=std::size_t;
    using value_t=decltype(fst[0][0]*snd[0][0]);
    return [&fst,&snd,row_2](size_t row,size_t coln)
    {
        value_t res=0;
        for(size_t i=0;i<row_2;++i)
        {
            res+=fst[row][i]*snd[i][coln];
        }
        return res;
    };
}

template<class matrix_t,class vector_t>
auto fn_matrix_to_vector(const matrix_t& mtx,const vector_t& vct,std::size_t row_2 )
{
    using size_t=std::size_t;
    using value_t=decltype(mtx[0][0]*vct[0]);
    return [&mtx,&vct,row_2](size_t row)
    {
        value_t res=0;
        for(size_t i=0;i<row_2;++i)
        {
            res+=mtx[row][i]*vct[i];
        }
        return res;
    };
}


template<class matrix_t>
void to_zero(matrix_t& mtx,std::size_t rows,std::size_t cols)
{
    using size_t=std::size_t;
    for(size_t i=0;i<rows;++i)
    {
        for(size_t j=0;j<cols;++j)
        {
            mtx[i][j]=0;
        }
    }
}

template<class matrix_t>
void to_identity(matrix_t& mx,std::size_t rows,std::size_t cols)
{
    assert(rows==cols);
    to_zero(mx,rows,cols);
    for(std::size_t i=0;i<rows;++i)
    {
        mx[i][i]=1;
    }
}

template<class matrix_t,class functor_t>
void fill_matrix(matrix_t& mx,functor_t func,std::size_t rows,std::size_t cols)
{
    using size_t=std::size_t;
    for(size_t i=0;i<rows;++i)
    {
        for(size_t j=0;j<cols;++j)
        {
            mx[i][j]=func(i,j);
        }
    }
}

template<class matrix_t1,class matrix_t2,class matrix_t3>
void matrix_to_matrix(const matrix_t1& mx1,
                      const matrix_t2& mx2,
                      matrix_t3& mx3,
                      std::size_t row1,std::size_t col1,std::size_t col2)
{
    fill_matrix(mx3,fn_matrix_to_matrix(mx1,mx2,col1),row1,col2);
}


template<class matrix_t1,class array_t1,class array_t2>
void matrix_to_vector(const matrix_t1& mx1,
                      const array_t1& arr1,
                      array_t2& arr2,
                      std::size_t rows,std::size_t cols)
{
    auto ftr=fn_matrix_to_vector(mx1,arr1,cols);
    for(std::size_t i=0;i<rows;++i){ arr2[i]=ftr(i);}
}

#endif

