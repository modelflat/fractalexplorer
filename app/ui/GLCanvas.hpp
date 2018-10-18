#ifndef FRACTALEXPLORER_GLCANVAS_HPP
#define FRACTALEXPLORER_GLCANVAS_HPP

#include <QOpenGLWidget>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLTexture>

#include <CL/cl.hpp>

class OpenCLBackend {

public:

    cl::Context currentContext();

    bool interoperableWith(QOpenGLContext* context);

};

class OpenCLImageWidget : public QOpenGLWidget {

    std::shared_ptr<OpenCLBackend> backend_;

public:

    OpenCLImageWidget(std::shared_ptr<OpenCLBackend> backend);

    cl::Image& image();

protected:

    virtual void initializeGL() {
        auto* glContext = QOpenGLContext::currentContext();

        if (backend_->interoperableWith(glContext)) {
            GLuint image = backend_.acquireImage(image_);
            // redraw fast using GL interop
            redrawGLImage();
            //
            backend_.releaseImage(image_);
        } else {
            ??? image = backend_.acquireImageNoInterop(image_);
            // redraw copied buffer
            backend_.releaseImageNoInterop(image_);
        }

    }

    virtual void resizeGL(int w, int h) {
    }

    virtual void paintGL() {
    }

private:

    void redrawGLImage();

};


using real = double;

class GLCanvas : public QOpenGLWidget {

    Settings settings;

    cl::CommandQueue queue;

    cl::Context clContext;
    cl::ImageGL imageCL;
    QOpenGLTexture* texture;
    bool saveScreenshotOnce = false;

    unsigned int postClearProgram;

    unsigned int program;
    unsigned int vertexBufferObject;

    NewtonKernelWrapper newtonKernelWrapper;
    ClearKernelWrapper clearKernelWrapper;

    std::vector<int> buffer;

    size_t width_, height_;

    void initCLSide(QOpenGLContext* context);

    void initGLSide(QOpenGLFunctions* gl, std::vector<int>& buffer);

public:

    void initWithCL(cl::Context);

protected:
    virtual void initializeGL() override;

    virtual void resizeGL(int w, int h) override;

    virtual void paintGL() override;

};


#endif //FRACTALEXPLORER_GLCANVAS_HPP
