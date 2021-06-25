#-------------------------------------------------
#
# Project created by QtCreator 2021-03-16T10:28:56
#
#-------------------------------------------------

QT       += core gui opengl
#CONFIG+=release
QMAKE_CXXFLAGS += -std=c++20

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
TARGET = GraphViewer
TEMPLATE = app
INCLUDEPATH =EIGEN_ROOT/eigen_34


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
Expression/data_pool.cpp\
Expression/dependency_graph.cpp\
json11.cpp\
renderer.cpp \
plot_2D_base.cpp \
plot_2D_opengl.cpp\
plot_2D_qt.cpp\
main_window.cpp \
plot_3D.cpp\
view_widget.cpp\
json_convert.cpp


HEADERS +=\
data_mesh.h\
figure.h\
opengl_iface.h\
tree_builder.h\
view_ruling.h\
transformation.h\
Expression/expression_parser.h\
Expression/data_pool.h\
Expression/parse_error.h\
Expression/reversed_sequence.h\
Expression/string_comparer.h\
Expression/dependency_graph.h\
json11.hpp\
renderer.h\
rect_mesh.h \
plot_2D_base.h\
plot_2D_opengl.h\
plot_2D_qt.h\
main_window.h \
plot_3D.h \
view_widget.h\
json_convert.h\
matrix_ref.h\
vecalg.h\
output_iterator.hpp





