
#ifndef _vao_managment_
#define _vao_managment_

#include <assert.h>
#include <array>
#include <type_traits>

#include <GL/gl.h>

#include "../opengl_iface.h"

template<class T>
constexpr GLenum type_to_enum()
{
    using decay_type=std::decay_t<T>;
    if constexpr(std::is_same_v<decay_type,GLbyte>) return GL_BYTE;
    else if constexpr(std::is_same_v<decay_type,GLubyte>) return GL_UNSIGNED_BYTE;
    else if constexpr(std::is_same_v<decay_type,GLshort>) return GL_SHORT;
    else if constexpr(std::is_same_v<decay_type,GLushort>) return GL_UNSIGNED_SHORT;
    else if constexpr(std::is_same_v<decay_type,GLint>) return GL_INT;
    else if constexpr(std::is_same_v<decay_type,GLuint>) return GL_UNSIGNED_INT;
    else if constexpr(std::is_same_v<decay_type,GLfloat>) return GL_FLOAT;
    else if constexpr(std::is_same_v<decay_type,GLdouble>) return GL_DOUBLE;

    else static_assert(sizeof(T)==0, "No match");
}



#define value_wrapper(class_name,value_type)\
class class_name\
{\
    value_type m_value;\
    public:\
    explicit class_name(value_type v):m_value(v){}\
    operator value_type()const{return m_value;}\
};


class CBufferFormat
{
    using size_t=std::size_t;
    bool   m_normalize=false;
    size_t m_start_ofset=0;
    size_t m_block_size=0;
    size_t m_step=0;
    CBufferFormat(size_t n,size_t s,size_t b,size_t step):
    m_normalize(n),m_start_ofset(s),m_block_size(b),m_step(step)
    {}
    public:
    CBufferFormat(){}
    static CBufferFormat Solid(size_t s)
    {
        return CBufferFormat{false,0,s,s};
    }

    CBufferFormat&SetStartOffset(size_t);
    CBufferFormat&SetBlockSize(size_t);
    CBufferFormat&SetStep(size_t);
    CBufferFormat&SetNormalize(size_t);

    bool operator==(const CBufferFormat&)const;
    bool operator!=(const CBufferFormat&other)const{return !(other==*this);}
    size_t StartOfset()const{return m_start_ofset;}
    size_t BlockSize()const{return  m_block_size;}
    size_t Gap()const{return m_step-m_block_size;}
    size_t Step()const{return m_step;}
    bool   Normalize()const{return m_normalize;}
};
#undef value_wrapper



class CBaseBufferImpl
{
    using size_t=std::size_t;
    public:
    enum usage_t:GLenum
    {
        stream_draw=GL_STREAM_DRAW,stream_read=GL_STREAM_READ,stream_copy=GL_STREAM_COPY,
        static_draw=GL_STATIC_DRAW,static_read=GL_STATIC_READ,static_copy=GL_STATIC_COPY,
        dynamic_draw=GL_DYNAMIC_DRAW,dynamic_read=GL_DYNAMIC_READ,dynamic_copy=GL_DYNAMIC_COPY
    };
    private:
    const GLenum m_buffer_type;
    GLuint m_handler;
    size_t m_sizeof_type;
    size_t m_num_elements;
    void   m_CopyData(const CBaseBufferImpl&other);
    protected:
    GLenum       m_type_enum;
    void         m_Write(size_t size_of_type,size_t num_type,const void*ptr,usage_t);
    size_t       m_Read(void*ptr,size_t bytes)const;
    void         m_Swap(CBaseBufferImpl&other);

    CBaseBufferImpl(GLenum t,size_t);
    CBaseBufferImpl(const CBaseBufferImpl&)=delete;
    CBaseBufferImpl&operator=(const CBaseBufferImpl&)=delete;
    CBaseBufferImpl(CBaseBufferImpl&&other):m_buffer_type(other.m_buffer_type)
    {
        m_CopyData(other);
        other.m_sizeof_type=0;
    }

    CBaseBufferImpl&operator=(CBaseBufferImpl&&other);
    public:

    GLenum TypeEnum()const{return m_type_enum;}
    GLuint Handler()const{return m_handler;}
    size_t SizeOfType()const{return m_sizeof_type;}
    size_t Size()const{return m_num_elements;}
    bool   Empty()const{return m_num_elements==0;}
    bool   Valid()const{return m_sizeof_type!=0;}
    ~CBaseBufferImpl();
};

template<GLenum buf_type,class...avail_t>
class buffer_impl_t:public CBaseBufferImpl
{
    template<class test_t,class head_t,class...tail_t>
    static constexpr bool m_acceptable_type()
    {
        if constexpr(std::is_same_v<test_t,head_t>) return true;
        else if constexpr(sizeof...(tail_t)!=0)
        {
            return m_acceptable_type<test_t,tail_t...>();
        }
        else
        {
            return false;
        }
    }
    public:
    template<class type>
    buffer_impl_t(type):CBaseBufferImpl(buf_type,sizeof(type))
    {
        static_assert(m_acceptable_type<type,avail_t...>());
        m_type_enum=type_to_enum<type>();
    }

    buffer_impl_t(buffer_impl_t&&other)=default;
    template<class T>
    void Write(const T* ptr,size_t size,usage_t usage)
    {
        static_assert(m_acceptable_type<std::decay_t<T>,avail_t...>());
        m_type_enum=type_to_enum<T>();
        m_Write(sizeof(T),size,ptr,usage);
    }
    template<class range_t,typename=decltype(std::data(std::declval<const range_t&>())),
                           typename=decltype(std::size(std::declval<const range_t&>()))>
    void Write(const range_t&r,usage_t usage)
    {
        using type=std::decay_t<decltype(*std::data(r))>;
        static_assert(m_acceptable_type<type,avail_t...>());
        m_type_enum=type_to_enum<type>();
        m_Write(sizeof(type),std::size(r),std::data(r),usage);
    }

    template<class T>
    size_t Read(T*ptr,size_t size)const
    {
        static_assert(m_acceptable_type<std::decay_t<T>,avail_t...>());
        assert(type_to_enum<T>()==m_type_enum);
        return m_Read(ptr,size)/SizeOfType();
    }
    template<class range_t,typename=decltype(std::data(std::declval<const range_t&>())),
                           typename=decltype(std::size(std::declval<const range_t&>()))>
    size_t Read(range_t&r,usage_t usage)const
    {
        using type=std::decay_t<decltype(*std::data(r))>;
        static_assert(m_acceptable_type<type,avail_t...>());
        assert(type_to_enum<type>()==m_type_enum);
        return m_Read(std::data(r),std::size(r))/SizeOfType();
    }
    void Swap(buffer_impl_t&other)
    {
        m_Swap(other);
    }
    friend class CVao;
};

using CBuffer=buffer_impl_t<GL_ARRAY_BUFFER,GLbyte,GLubyte,GLushort,GLint,GLuint,GLfloat,GLdouble>;
using CIndexBuffer=buffer_impl_t<GL_ELEMENT_ARRAY_BUFFER,GLubyte,GLushort,GLuint>;


class CVao
{
    using size_t=std::size_t;
    public:
    enum mode_t:GLenum
    {
        points=GL_POINTS,
        line_strip=GL_LINE_STRIP, line_loop=GL_LINE_LOOP, lines=GL_LINES,
        line_strip_adjacensy=GL_LINE_STRIP_ADJACENCY,lines_adjacency=GL_LINES_ADJACENCY,
        triangle_strip=GL_TRIANGLE_STRIP,triangle_fan=GL_TRIANGLE_FAN,triangles=GL_TRIANGLES,
        triangle_strip_adjacency=GL_TRIANGLE_STRIP_ADJACENCY,trinagle_adjacency=GL_TRIANGLES_ADJACENCY,
        patches=GL_PATCHES
    };
    private:
    bool m_moved=false;
    GLuint m_handler;
    public:
    CVao();
    CVao(const CVao&)=delete;
    CVao&operator=(const CVao&)=delete;
    CVao(CVao&&other)
    {
        m_handler=other.m_handler;
        other.m_moved=true;
    }
    CVao&operator=(CVao&&other);

    GLuint Handler()const{return m_handler;}
    const CVao& EnableLayout(GLuint,CBuffer&,const CBufferFormat&)const;
    void DisableLayout(GLuint)const;
    CBufferFormat GetFormatOfLayout(GLuint)const;

    void DrawArraw(mode_t,size_t ,size_t )const;

    void DrawElement(const CIndexBuffer&,mode_t ,size_t )const;
    void DrawElement(const CIndexBuffer&,mode_t )const;

    void Swap(CVao&other);
    ~CVao();
};



#endif
