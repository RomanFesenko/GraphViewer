#include <iostream>
#include <fstream>

#include <GL/glut.h>

#include "ruling.h"
#include "parse_config.h"

const std::string plot_config="graph_config";
const std::string parse_report="parse_report";

point_t glScreen{600.0f,600.0f};
float glMoveVelocity=0.05f;
float glViewVelocity=2.0f*Degree<float>;

struct render_resources_t
{
    CFunctionPool m_function_pool;
    CRenderer m_renderer;
};
render_resources_t glResources;

float Now(){return (glutGet(GLUT_ELAPSED_TIME))/1000.0f;}

void InitConfig()
{
    std::cout<<"Init config\n";
    std::ifstream file(plot_config);
    std::ofstream out(parse_report);

    if(!file)
    {
        std::string out_str=plot_config+" not found\n";;
        if(out)
        {
            out<<out_str;
        }
        else
        {
            std::cout<<out_str;
        }
        return;
    }

    std::string config;
    for(char c;file.get(c);) config.push_back(c);
    parse_config_result_t pcr=ParseConfig(config,glResources.m_function_pool);
    if(!pcr.is_valid())
    {
        glResources.m_renderer.ClearContent();
        if(out)
        {
            out<<ErrorReport(pcr);
        }
        else
        {
            std::cout<<ErrorReport(pcr);
        }
        return;
    }
    glResources.m_renderer.SetContent({pcr.m_plots,glMoveVelocity,glViewVelocity},Now());
}


void CharPressed ( unsigned char ch, int x, int y )
{
    auto& renderer=glResources.m_renderer;
    switch(ch)
    {
        case 'w':
        case 'W':
        renderer.IncMove(CRenderer::forward_id);
        break;

        case 's':
        case 'S':
        renderer.IncMove(CRenderer::back_id);
        break;

        case 'd':
        case 'D':
        renderer.IncMove(CRenderer::right_id);
        break;

        case 'a':
        case 'A':
        renderer.IncMove(CRenderer::left_id);
        break;

        case 'e':
        case 'E':
        InitConfig();
        break;

        case ' ':
        renderer.NextPlot(Now());
        break;

        case 'q':
        case 'Q':
        exit(0);
        break;

        default:break;
    }
}


void mouseMove ( int x, int y )
{
    static bool is_warp=false;
    if(is_warp)
    {
        is_warp=false;
        return;
    }
    glResources.m_renderer.IncView(x-glScreen[0]/2.0,y-glScreen[1]/2.0);
    is_warp=true;
    glutWarpPointer(glScreen[0]/2,glScreen[1]/2);
}


void init()
{
    InitConfig();
    glCheckError();
}

void UpdatePlot(int)
{
    glutTimerFunc(30,UpdatePlot,0);
    glutPostRedisplay();
}

void display()
{
  glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
  glResources.m_renderer.Render(Now());
  glutSwapBuffers();
  //glFlush();
  glCheckError();
}

int main(int argc, char **argv)
{
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB|GLUT_DEPTH);
  glutInitWindowSize(glScreen[0], glScreen[1]);
  glutInitWindowPosition(100, 740);
  glutCreateWindow("Surface viewer.");

  glClearColor( 0.0,  0.0, 0.0, 1.0);

  init();
  glEnable(GL_DEPTH_TEST);


  glutDisplayFunc(display);
  glutKeyboardFunc(CharPressed);
  glutMotionFunc(mouseMove );
  glutPassiveMotionFunc(mouseMove);
  glutTimerFunc(30,UpdatePlot,0);

  glutMainLoop();
}
