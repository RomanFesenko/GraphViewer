
#include "light_source.h"

CLightSource&CLightSource::SetPosition(const point_t&pos)
{
    m_position=pos;
    return *this;
}

CLightSource&CLightSource::SetAmbient(const point_t&am)
{
    m_ambient=am;
    return *this;
}

CLightSource&CLightSource::SetDiffuse(const point_t&diff)
{
    m_diffuse=diff;
    return *this;
}

CLightSource&CLightSource::SetSpecular(const point_t&spec)
{
    m_specular=spec;
    return *this;
}

CLightSource&CLightSource::SetAttenuation(const point_t&att)
{
    m_attenuation=att;
    return *this;
}

CLightSource& CLightSource::SetPosition(float x,float y,float z)
{
    return SetPosition({x,y,z});
}

CLightSource& CLightSource::SetAmbient(float x,float y,float z)
{
    return SetAmbient({x,y,z});
}

CLightSource& CLightSource::SetDiffuse(float x,float y,float z)
{
    return SetDiffuse({x,y,z});
}

CLightSource& CLightSource::SetSpecular(float x,float y,float z)
{
    return SetSpecular({x,y,z});
}

CLightSource& CLightSource::SetAttenuation(float x,float y,float z)
{
    return SetAttenuation({x,y,z});
}

