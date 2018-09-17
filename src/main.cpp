#include <iostream>
#include <QOpenGLWidget>
#include <QApplication>
#include <QPushButton>

int main( int argc, char* argv[] )
{
    QApplication a(argc, argv);

    QPushButton hello( "Hello world!", nullptr );
    hello.resize( 640, 480 );
    hello.show();

    return QApplication::exec();
}