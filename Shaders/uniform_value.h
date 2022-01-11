
#ifndef _uniform_value_
#define _uniform_value_

#include <type_traits>
#include <assert.h>

#include <GL/gl.h>

#include "../opengl_iface.h"


template<class T>
constexpr bool availabele_uniform_scalar()
{
    return std::is_same_v<T,GLuint>||
           std::is_same_v<T,GLint>||
           std::is_same_v<T,GLfloat>||
           std::is_same_v<T,GLdouble>;
}

template<class T,int dim>
struct vec
{
    static_assert(availabele_uniform_scalar<T>()&&dim>1&&dim<5);
};

template<int dim>
struct mat
{
    static_assert(dim>1&&dim<5);
};


using ivec2=vec<GLint,2>;
using ivec3=vec<GLint,3>;
using ivec4=vec<GLint,4>;

using uvec2=vec<GLuint,2>;
using uvec3=vec<GLuint,3>;
using uvec4=vec<GLuint,4>;

using vec2=vec<GLfloat,2>;
using vec3=vec<GLfloat,3>;
using vec4=vec<GLfloat,4>;

using dvec2=vec<GLdouble,2>;
using dvec3=vec<GLdouble,3>;
using dvec4=vec<GLdouble,4>;

using mat2=mat<2>;
using mat3=mat<3>;
using mat4=mat<4>;


class uniform_value_base
{
    protected:
    GLint m_handler;
    uniform_value_base(GLint h):m_handler(h){}

#define decl_uniform(short_type,full_type)\
void m_glUniform1##short_type (full_type);\
void m_glUniform2##short_type (full_type,full_type);\
void m_glUniform3##short_type (full_type,full_type,full_type);\
void m_glUniform4##short_type (full_type,full_type,full_type,full_type);\
void m_glUniform2##short_type##v (GLsizei,const full_type*);\
void m_glUniform3##short_type##v (GLsizei,const full_type*);\
void m_glUniform4##short_type##v (GLsizei,const full_type*);\

    decl_uniform(ui,GLuint)
    decl_uniform(i,GLint)
    decl_uniform(f,GLfloat)
    decl_uniform(d,GLdouble)

#undef decl_uniform

    void m_glUniformMatrix2fv(const GLfloat *);
    void m_glUniformMatrix3fv(const GLfloat *);
    void m_glUniformMatrix4fv(const GLfloat *);

    public:
    explicit operator bool()const{return m_handler!=-1;}
};

template<class T>
class uniform_value:public uniform_value_base
{
    static_assert(availabele_uniform_scalar<T>());

    uniform_value(GLint h):uniform_value_base(h){}
    public:
    uniform_value& operator=(const T&val)
    {
        assert(m_handler!=-1);
        if constexpr(std::is_same_v<T,GLuint>)
        {
            m_glUniform1ui(val);
        }
        else if constexpr(std::is_same_v<T,GLint>)
        {
            m_glUniform1i(val);
        }
        else if constexpr(std::is_same_v<T,GLfloat>)
        {
            m_glUniform1f(val);
        }
        else if constexpr(std::is_same_v<T,GLdouble>)
        {
            m_glUniform1d(val);
        }
        glCheckError();
        return *this;
    }
    friend class CShaderProgramm;
};

template<class T,int dim>
class uniform_value<vec<T,dim>>:public uniform_value_base
{
    static_assert(availabele_uniform_scalar<T>());
    uniform_value(GLint h):uniform_value_base(h){}
    public:
    template<class range_t>
    uniform_value&operator=(const range_t&rg)
    {
        assert(m_handler!=-1);
        if constexpr(std::is_same_v<T,GLuint>)
        {
            if constexpr(dim==2)
            {
                m_glUniform2ui(rg[0],rg[1]);
            }
            else if constexpr(dim==3)
            {
                m_glUniform3ui(rg[0],rg[1],rg[2]);
            }
            else if constexpr(dim==4)
            {
                m_glUniform4ui(rg[0],rg[1],rg[2],rg[3]);
            }
        }
        else if constexpr(std::is_same_v<T,GLint>)
        {
            if constexpr(dim==2)
            {
                m_glUniform2i(m_handler,rg[0],rg[1]);
            }
            else if constexpr(dim==3)
            {
                m_glUniform3i(m_handler,rg[0],rg[1],rg[2]);
            }
            else if constexpr(dim==4)
            {
                m_glUniform4i(m_handler,rg[0],rg[1],rg[2],rg[3]);
            }
        }
        else if constexpr(std::is_same_v<T,GLfloat>)
        {
            if constexpr(dim==2)
            {
                m_glUniform2f(m_handler,rg[0],rg[1]);
            }
            else if constexpr(dim==3)
            {
                glUniform3f(m_handler,rg[0],rg[1],rg[2]);
            }
            else if constexpr(dim==4)
            {
                glUniform4f(m_handler,rg[0],rg[1],rg[2],rg[3]);
            }
        }
        else if constexpr(std::is_same_v<T,GLdouble>)
        {
            if constexpr(dim==2)
            {
                m_glUniform2d(m_handler,rg[0],rg[1]);
            }
            else if constexpr(dim==3)
            {
                m_glUniform3d(m_handler,rg[0],rg[1],rg[2]);
            }
            else if constexpr(dim==4)
            {
                m_glUniform4d(m_handler,rg[0],rg[1],rg[2],rg[3]);
            }
        }
        glCheckError();
        return *this;
    }
    friend class CShaderProgramm;
};


template<int dim>
class uniform_value<mat<dim>>:public uniform_value_base
{
    uniform_value(GLint h):uniform_value_base(h){}
    public:
    template<class range_t>
    uniform_value&operator=(const range_t&rg)
    {
        assert(m_handler!=-1);
        if constexpr(dim==2)
        {
            m_glUniformMatrix2fv(rg.data());
        }
        else if constexpr(dim==3)
        {
            m_glUniformMatrix3fv(rg.data());
        }
        else if constexpr(dim==4)
        {
            m_glUniformMatrix4fv(rg.data());
        }
        glCheckError();
        return *this;
    }
    friend class CShaderProgramm;
};

template<class T,int dim>
class uniform_value<T[dim]>:public uniform_value_base
{
    static_assert(availabele_uniform_scalar<T>()&&dim>0&&dim<5);
    uniform_value(GLint h):uniform_value_base(h){}
    public:
    template<class range_t,typename=decltype(std::data(std::declval<const range_t&>())),
                           typename=decltype(std::size(std::declval<const range_t&>()))>
    uniform_value& operator=(const range_t&source)
    {
        assert(m_handler!=-1);
        auto count=std::size(source);
        if constexpr(std::is_same_v<T,GLuint>)
        {
            m_glUniform1uiv(count,std::data(source));
        }
        else if constexpr(std::is_same_v<T,GLint>)
        {
            m_glUniform1iv(count,std::data(source));
        }
        else if constexpr(std::is_same_v<T,GLfloat>)
        {
            m_glUniform1fv(count,std::data(source));
        }
        else if constexpr(std::is_same_v<T,GLdouble>)
        {
            m_glUniform1dv(count,std::data(source));
        }
        glCheckError();
        return *this;
    }
    friend class CShaderProgramm;
};

template<class T,int dim,int array_dim>
class uniform_value<vec<T,dim>[array_dim]>:public uniform_value_base
{
    static_assert(availabele_uniform_scalar<T>()&&dim>1&&dim<5);
    uniform_value(GLint h):uniform_value_base(h){}
    public:
    template<class range_t,typename=decltype(std::data(std::declval<const range_t&>())),
                           typename=decltype(std::size(std::declval<const range_t&>()))>
    uniform_value&operator=(const range_t&source)
    {
        assert(m_handler!=-1);
        auto count=std::size(source)/dim;
        const T*data=std::data(source);
        if constexpr(std::is_same_v<T,GLuint>)
        {
            if constexpr(dim==2)
            {
                m_glUniform2uiv(count,data);
            }
            else if constexpr(dim==3)
            {
                m_glUniform3uiv(count,data);
            }
            else if constexpr(dim==4)
            {
                m_glUniform4uiv(count,data);
            }
        }
        else if constexpr(std::is_same_v<T,GLint>)
        {
            if constexpr(dim==2)
            {
                m_glUniform2iv(count,data);
            }
            else if constexpr(dim==3)
            {
                m_glUniform3iv(count,data);
            }
            else if constexpr(dim==4)
            {
                m_glUniform4iv(count,data);
            }
        }
        else if constexpr(std::is_same_v<T,GLfloat>)
        {
            if constexpr(dim==2)
            {
                m_glUniform2fv(count,data);
            }
            else if constexpr(dim==3)
            {
                m_glUniform3fv(count,data);
            }
            else if constexpr(dim==4)
            {
                m_glUniform4fv(count,data);
            }
        }
        else if constexpr(std::is_same_v<T,GLdouble>)
        {
            if constexpr(dim==2)
            {
                m_glUniform2dv(count,data);
            }
            else if constexpr(dim==3)
            {
                m_glUniform3dv(count,data);
            }
            else if constexpr(dim==4)
            {
                m_glUniform4dv(count,data);
            }
        }
        glCheckError();
        return *this;
    }
    friend class CShaderProgramm;
};




#endif
