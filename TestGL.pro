#-------------------------------------------------
#
# Project created by QtCreator 2021-03-16T10:28:56
#
#-------------------------------------------------

QMAKE_CC = gcс 11
QMAKE_CXX = g++-11
QMAKE_LINK_C = gcc 11
QMAKE_LINK = g++-11

QT            +=core gui opengl widgets


QMAKE_CXXFLAGS+=-std=c++20 -fsanitize=address
QMAKE_LFLAGS+=-fsanitize=address

TARGET = GraphViewer
TEMPLATE = app

CONFIG+=qt


INCLUDEPATH =/home/roma/EIGEN_ROOT/eigen-3.4.0

LIBS +=-lGLEW

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
main.cpp\
view_ruling.cpp\
../BaseLibraries/Expression/function_pool.cpp\
../../CppProjects/json11/json11.cpp\
Shaders/shader_programm.cpp\
Shaders/vao_managment.cpp\
functional_mesh.cpp\
Shaders/uniform_value.cpp\
legacy_render.cpp\
light_source.cpp\
rigid_transform.cpp\
scene.cpp\
plot_2D_base.cpp \
plot_2D_opengl.cpp\
plot_2D_qt.cpp\
main_window.cpp \
view_widget.cpp\
json_convert.cpp


HEADERS +=\
opengl_iface.h\
view_ruling.h\
../BaseLibraries/GEOMETRY/transformation.h\
../BaseLibraries/GEOMETRY/vecalg.h\
../BaseLibraries/Expression/expression_parser.h\
../BaseLibraries/Expression/function_pool.h\
../BaseLibraries/Expression/reversed_sequence.h\
../BaseLibraries/Expression/string_util.h\
../BaseLibraries/Expression/dependency_graph.h\
../../CppProjects/json11/json11.hpp\
Shaders/shader_programm.h\
Shaders/vao_managment.h\
Shaders/shader_programm.h\
Shaders/uniform_value.h\
legacy_render.h\
functional_mesh.h\
rigid_transform.h\
legacy_render.h\
scene.h\
plot_2D_base.h\
plot_2D_opengl.h\
plot_2D_qt.h\
main_window.h \
view_widget.h\
json_convert.h\
rect_mesh.h








