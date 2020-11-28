
#ifndef  _matrix_
#define  _matrix_

// 

namespace ext{

template <class T>
class matrix
{
   using size_t=std::size_t;
   T* m_headelems=nullptr;
   T** m_rows_ptrs;
   size_t m_size,m_rows,m_columns;
   void m_free_mem();
   void m_alloc_mem(size_t _rows, size_t _columns);
   public:
   size_t size()const{return m_size;}
   size_t rows()const{return m_rows;}
   size_t columns()const{return m_columns;}

   matrix(){}
   explicit matrix(size_t rows, size_t columns);
   ~matrix();
   void resize(size_t rows, size_t columns);

   T* operator [](size_t row){return *(m_rows_ptrs+row);}
   const T*operator [](size_t row)const{return *(m_rows_ptrs+row);}
   matrix& operator=(const matrix& rhs)=delete;
   matrix(const matrix& rhs)=delete;
};

template <class T>
void matrix<T>::m_alloc_mem(size_t _rows, size_t _columns)
{
   m_rows=_rows;  m_columns=_columns; m_size=_rows*_columns;
   m_headelems=new T[m_size];
   m_rows_ptrs= new T*[_rows];
   size_t temp,i;
   for(i=0,temp=0;i<_rows;i++,temp+=_columns)
   {
       m_rows_ptrs[i]=m_headelems+temp;
   }
}

template <class T>
void matrix<T>::m_free_mem()
{
    if(m_headelems==nullptr) return;
    delete[] m_headelems;
    delete[] m_rows_ptrs;
    m_headelems=nullptr;
}

template <class T>
matrix<T>::matrix(size_t _rows, size_t _columns)
{
   m_alloc_mem(_rows,_columns);
}

template <class T>
void matrix<T>::resize(size_t _rows, size_t _columns)
{
    m_free_mem();
    m_alloc_mem(_rows,_columns);
}

template <class T>
matrix<T>::~matrix()
{
    m_free_mem();
}

}

#endif

