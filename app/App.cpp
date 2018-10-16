#include <iostream>

#include <QOpenGLWidget>
#include <QApplication>
#include <QPushButton>

#include "ComputableImage.hpp"

int main( int argc, char* argv[] )
{
    QApplication a(argc, argv);

    std::cout << "CL: " << cl::Platform::getDefault().getInfo<CL_PLATFORM_NAME>() << std::endl;

    QPushButton hello( "Hello world!", nullptr );
    hello.resize( 640, 480 );
    hello.show();

    OpenCLBackendPtr backend = std::make_shared<OpenCLBackend>();
    OpenCLComputableImage<Dim_2D> img(Range<2> {512, 512}, backend);

    try {
        backend->compileKernel({
            "newton_fractal", "default"
        });
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }

    img.clear();

    backend->currentQueue().finish();

    return QApplication::exec();
}