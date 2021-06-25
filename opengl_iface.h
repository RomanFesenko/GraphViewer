
#ifndef  _opengl_iface_
#define  _opengl_iface_

#include <GL/gl.h>

#include <initializer_list>
#include <array>
#include <string>
#include <iostream>


#include "transformation.h"
#include <Eigen/Core>


const bool gl_debug=true;

using point_t=Eigen::Vector2f;
using point3D_t=Eigen::Vector3f;
using matrix3D_t=Eigen::Matrix<float,4,4,Eigen::ColMajor>;

inline void to_homogenius_buffer(const point3D_t& point,float* _float)
{
    _float[0]=point[0];
    _float[1]=point[1];
    _float[2]=point[2];
    _float[3]=1.0f;
}

inline void SetMatrix(const matrix3D_t& matrix,GLenum en_matr)
{
    glMatrixMode(en_matr);
    glLoadMatrixf(matrix.data());
}

struct light_params_t
{
    point3D_t m_position;
    std::array<point3D_t,3> m_intensity;
    point3D_t m_attenuation;
};

class CLightSource
{
    light_params_t m_params;
    public:
    enum light_t
    {
        id_ambient=0,
        id_diffuse,
        id_specular
    };
    CLightSource() {}
    void SetParams(const light_params_t&params)
    {
        SetPosition(params.m_position);
        SetIntensity(id_ambient,params.m_intensity[0]);
        SetIntensity(id_diffuse,params.m_intensity[1]);
        SetIntensity(id_specular,params.m_intensity[2]);
        SetAttenuation(params.m_attenuation);
    }
    void SetPosition(const point3D_t&pos)
    {
        m_params.m_position=pos;
        float _buf[4];
        to_homogenius_buffer(pos,_buf);
        glLightfv(GL_LIGHT0,GL_POSITION,_buf);
    }
    point3D_t GetPosition()const{return m_params.m_position;}

    void SetIntensity(light_t id_light,const point3D_t&intens)
    {
        m_params.m_intensity[id_light]=intens;
        float _buf[4];
        to_homogenius_buffer(intens,_buf);
        if(id_light==id_ambient)
        {
            glLightfv(GL_LIGHT0,GL_AMBIENT,_buf);
        }
        else if(id_light==id_diffuse)
        {
            glLightfv(GL_LIGHT0,GL_DIFFUSE,_buf);
        }
        else
        {
            glLightfv(GL_LIGHT0,GL_SPECULAR,_buf);
        }
    }
    void SetAttenuation(const point3D_t&coeffs)
    {
        m_params.m_attenuation=coeffs;
        float _buf[4];
        to_homogenius_buffer(coeffs,_buf);
        glLightfv(GL_LIGHT0,GL_CONSTANT_ATTENUATION,_buf);
    }
};

struct draw_guard
{
    static bool is_draw;
    explicit draw_guard(GLenum type)
    {
        assert(!is_draw);
        is_draw=true;
        glBegin(type);
    }
    ~draw_guard(){glEnd();is_draw=false;}
};
inline bool draw_guard::is_draw=false;

class matrix_guard
{
    public:
    explicit matrix_guard(const matrix3D_t&view,
                          const matrix3D_t&perspective)
    {
        assert(!draw_guard::is_draw);
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        assert(glGetError()==GL_NO_ERROR);
        SetMatrix(view,GL_MODELVIEW);

        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        assert(glGetError()==GL_NO_ERROR);
        SetMatrix(perspective,GL_PROJECTION);
    }
    ~matrix_guard()
    {
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        assert(glGetError()==GL_NO_ERROR);
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
        assert(glGetError()==GL_NO_ERROR);
    }
};

inline void _glCheckError_(const char *file, int line)
{
    if constexpr(gl_debug){
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR)
    {
        std::string error;
        switch (errorCode)
        {
            case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
            case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
            case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
            case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    }
}
#define glCheckError() _glCheckError_(__FILE__, __LINE__)


#endif

