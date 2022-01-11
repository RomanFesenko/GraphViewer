
#ifndef _shader_programm_
#define _shader_programm_

#include <assert.h>
#include <string>
#include <type_traits>

#include <GL/gl.h>

#include "uniform_value.h"


class CShaderBuildError
{
    public:
    enum status_t
    {
        no_error,link_error,compile_error
    };
    private:
    status_t m_status;
    std::string m_log;
    void m_SetNoError(){m_status=no_error;m_log.clear();}
    public:
    status_t Status()const{return m_status;}
    const std::string& What()const{return m_log;}
    explicit operator bool()const{return m_status!=no_error;}
    friend class CShader;
    friend class CShaderProgramm;
};


class CShaderBuildException:public std::exception
{
    CShaderBuildError m_error;
    public:
    explicit CShaderBuildException(const CShaderBuildError&err):m_error(err){assert(err);}
    virtual const char* what() const noexcept override
    {
        return m_error.What().data();
    }
    CShaderBuildError::status_t Status()const{return m_error.Status();}
};

class CShader
{
    GLuint m_handler=0;
    CShader(GLuint h):m_handler(h){}
    public:
    enum type_t
    {
        compute,
        vertex,
        tess_control,
        tess_evaluation,
        geometry,
        fragment
    };
    CShader(){}
    static CShader Compile(type_t t,const char*str,CShaderBuildError&);
    static CShader Compile(type_t t,const std::string&str,CShaderBuildError&err)
    {return Compile(t,str.c_str(),err);}
    static CShader Compile(type_t,const char*);
    static CShader Compile(type_t t,const std::string&str){return Compile(t,str.c_str());}

    CShader(const CShader&)=delete;
    CShader&operator=(const CShader&)=delete;

    CShader(CShader&&other)
    {
        m_handler=other.m_handler;
        other.m_handler=0;
    }
    CShader&operator=(CShader&&other);
    GLuint Handler()const{return m_handler;}
    bool Valid()const{return m_handler;}
    ~CShader();

};

class CShaderProgramm
{
    GLuint m_handler=0;
    CShaderProgramm(GLuint h):m_handler(h){}
    static GLuint m_glCreateProgram();

    void m_CheckError(CShaderBuildError&);
    void m_glAttachShader(GLuint);
    void m_glDetachShader(GLuint);
    void m_glLinkProgram();
    GLint m_glGetUniformLocation(const GLchar*)const;

    static const CShaderProgramm*m_current;
    template<class...shaders_t>
    void m_attach_all(const CShader&head,const shaders_t&...shaders)
    {
        assert(head.Valid());
        m_glAttachShader(head.Handler());
        if constexpr(sizeof...(shaders)) m_attach_all(shaders...);
    }

    template<class...shaders_t>
    void m_detach_all(const CShader&head,const shaders_t&...shaders)
    {
        assert(head.Valid());
        m_glDetachShader(head.Handler());
        if constexpr(sizeof...(shaders)) m_detach_all(shaders...);
    }
    public:
    CShaderProgramm(){}
    CShaderProgramm(const CShaderProgramm&)=delete;
    CShaderProgramm&operator=(const CShaderProgramm&)=delete;

    CShaderProgramm(CShaderProgramm&&other)
    {
        m_handler=other.m_handler;
        other.m_handler=0;
    }
    CShaderProgramm& operator=(CShaderProgramm&&other);

    template<class...shaders_t>
    static CShaderProgramm Link(CShaderBuildError&error,const shaders_t&...shaders)
    {
        GLuint handler=m_glCreateProgram();
        CShaderProgramm result(handler);
        result.m_attach_all(shaders...);
        result.m_glLinkProgram();
        result.m_detach_all(shaders...);
        result.m_CheckError(error);
        return result;
    }
    template<class...shaders_t>
    static CShaderProgramm Link(const shaders_t&...shaders)
    {
        CShaderBuildError error;
        auto result=Link(error,shaders...);
        if(error) throw CShaderBuildException(error);
        return result;
    }
    const CShaderProgramm*Current(){return m_current;}
    void   Use()const;
    GLuint Handler()const{return m_handler;}
    bool Valid()const{return m_handler;}
    template<class T>
    auto get_uniform(const char*str)const
    {
        return uniform_value<T>(m_glGetUniformLocation(str));
    }
    template<class T>
    auto get_uniform(const std::string&str)const
    {
        return get_uniform<T>(str.c_str());
    }
    ~CShaderProgramm();
};

#endif
