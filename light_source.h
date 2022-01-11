
#ifndef  _light_source_
#define  _light_source_

#include <Eigen/Core>


class CLightSource
{
    using point_t=Eigen::Vector3f;
    point_t m_position={0,0,5};
    point_t m_ambient={0.1,0.1,0.1};
    point_t m_diffuse={0.5,0.5,0.5};
    point_t m_specular={1.0,1.0,1.0};
    point_t m_attenuation={1.1,0.0,0.0};

    public:
    CLightSource() {}
    CLightSource&SetPosition(const point_t&);
    CLightSource&SetAmbient(const point_t&);
    CLightSource&SetDiffuse(const point_t&);
    CLightSource&SetSpecular(const point_t&);
    CLightSource&SetAttenuation(const point_t&);

    CLightSource&SetPosition(float,float,float);
    CLightSource&SetAmbient(float,float,float);
    CLightSource&SetDiffuse(float,float,float);
    CLightSource&SetSpecular(float,float,float);
    CLightSource&SetAttenuation(float,float,float);

    const point_t&Position()const{return m_position;}
    const point_t&Ambient()const{return m_ambient;}
    const point_t&Diffuse()const{return m_diffuse;}
    const point_t&Specular()const{return m_specular;}
    const point_t&Attenuation()const{return m_attenuation;}

};


#endif

