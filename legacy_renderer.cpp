#include <iostream>
#include <assert.h>

#include "opengl_iface.h"
#include "legacy_renderer.h"


void LegacyRender(const CFunctionalMesh&mesh)
{
    using point_t=CFunctionalMesh::point_t;
    auto traits=mesh.RenderingTraits();

    glDisable(GL_LIGHTING);// for correct use glColor*
    if(traits.IsBox())
    {
        // draw the closing box
        auto box=mesh.BoundedBox();
        point_t min=box.first;
        point_t max=box.second;
        point_t min_proj=min;min_proj[2]=max[2];

        glColor3f(1.0,1.0,1.0);
        {
            CDrawGuard dg(GL_LINE_LOOP);

            point_t current=min;
            glVertex3fv(current.data());

            current[0]=max[0];
            glVertex3fv(current.data());

            current[1]=max[1];
            glVertex3fv(current.data());

            current[0]=min[0];
            glVertex3fv(current.data());
        }
        {
            CDrawGuard dg(GL_LINE_LOOP);

            point_t current=min_proj;
            glVertex3fv(current.data());

            current[0]=max[0];
            glVertex3fv(current.data());

            current[1]=max[1];
            glVertex3fv(current.data());

            current[0]=min[0];
            glVertex3fv(current.data());
        }

        {
            float saved_min0=min[0];

            CDrawGuard dg(GL_LINES);

            glVertex3fv(min.data());glVertex3fv(min_proj.data());

            min[0]=min_proj[0]=max[0];
            glVertex3fv(min.data());glVertex3fv(min_proj.data());

            min[1]=min_proj[1]=max[1];
            glVertex3fv(min.data());glVertex3fv(min_proj.data());

            min[0]=min_proj[0]=saved_min0;
            glVertex3fv(min.data());glVertex3fv(min_proj.data());

        }
        glCheckError();
    }

    // Levels rendering ?

    // Surface
    auto&mater=mesh.GetMaterial();
    if(traits.IsSpecularSurface()) glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,mater.shininess);
    // render the surface
    int param=traits.IsSurface();
    param|=traits.IsColored()<<1;
    param|=traits.IsSpecularSurface()<<2;

    float temp[4]={0,0,0,1};
    switch(param)
    {
        case 1:// pure surface, uniform grey
        {
            glColor3f(0.5,0.5,0.5);
            CDrawGuard dg(GL_QUADS);
            auto visitor=[](auto&v){glVertex3fv(v.data());};
            mesh.GetGrid().quad_visit(visitor,mesh.Points());
        }
        break;

        case 3:// color surface
        {
            CDrawGuard dg(GL_QUADS);
            auto visitor=[&](auto&v,auto&c){glColor3fv(c.data());glVertex3fv(v.data());};
            mesh.GetGrid().quad_visit(visitor,mesh.Points(),mesh.Colors());
        }
        break;

        case 5:// specular surface
        glEnable(GL_LIGHTING);
        temp[0]=temp[1]=temp[2]=mater.ambient;
        glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,temp);
        temp[0]=temp[1]=temp[2]=mater.diffuse;
        glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,temp);
        temp[0]=temp[1]=temp[2]=mater.specular;
        glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,temp);
        {
            CDrawGuard dg(GL_QUADS);
            auto visitor=[](auto&v,auto&n){glNormal3fv(n.data());glVertex3fv(v.data());};
            mesh.GetGrid().quad_visit(visitor,mesh.Points(),mesh.Normals());
        }

        break;
        case 7:// colored and specular surface
        glEnable(GL_LIGHTING);
        {
            CDrawGuard dg(GL_QUADS);
            auto visitor=[&](auto&v,auto&c,auto&n)
            {
                temp[0]=c[0];temp[1]=c[1];temp[2]=c[2];
                glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,temp);
                glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,temp);
                glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,temp);
                glColor3fv(c.data());
                glNormal3fv(n.data());
                glVertex3fv(v.data());
            };
            mesh.GetGrid().quad_visit(visitor,mesh.Points(),mesh.Colors(),mesh.Normals());
        }
        break;

        default: assert((param&1)==0);
    }
    glDisable(GL_LIGHTING);
    glCheckError();


    // Mesh
    param=traits.IsMesh();
    param|=traits.IsSurface()<<1;
    param|=traits.IsColored()<<2;

    switch(param)
    {
        case 1:// pure mesh, uniform white
        glColor3f(1,1,1);
        {
            CDrawGuard dg(GL_LINES);
            auto visitor=[](auto&v){glVertex3fv(v.data());};
            mesh.GetGrid().edge_visit(visitor,mesh.Points());
        }
        break;

        case 3:// mesh over unpainted surface
        case 7:// mesh over painted surface
        glColor3f(0,0,0);
        glLineWidth(2);
        {
            CDrawGuard dg(GL_LINES);
            auto visitor=[](auto&v){glVertex3fv(v.data());};
            mesh.GetGrid().edge_visit(visitor,mesh.Points());
        }
        glLineWidth(1);
        break;

        case 5:// colored mesh
        {
            CDrawGuard dg(GL_LINES);
            auto visitor=[](auto&v,auto&c){glColor3fv(c.data());glVertex3fv(v.data());};
            mesh.GetGrid().edge_visit(visitor,mesh.Points(),mesh.Colors());
        }
        break;

        default:assert((param&1)==0);
    }
    glCheckError();

    glDisable(GL_COLOR_MATERIAL);
}







































