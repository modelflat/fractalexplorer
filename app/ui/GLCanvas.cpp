#include "GLCanvas.hpp"

#include "../core/clc/CLC_Sources.hpp"
#include "../core/glsl/GLSL_Sources.hpp"

void GLCanvas::initializeGL() {
    auto* context = QOpenGLContext::currentContext();
    auto* gl = context->functions();

    buffer = std::vector<int> (width_ * height_);

    initCLSide(context);
    initGLSide(gl, buffer);

    // interop
    imageCL = cl::ImageGL(clContext, CL_MEM_WRITE_ONLY, texture->target(), 0, texture->textureId());

    // kernels
    newtonKernelWrapper.setBounds(-1, 1, -1, 1);
    newtonKernelWrapper.setC({-0.5, 0.5});
    newtonKernelWrapper.setH(1);
    newtonKernelWrapper.setT(1);
    newtonKernelWrapper.setImage(imageCL);

    clearKernelWrapper.setImage(imageCL);

//    computeKernel.setArg(0, imageCL);

//    boundingBoxKernel.setArg(0, imageCL);
//    boundingBoxBuffer = clContext.createIntBuffer(4, CLMemory.Mem.WRITE_ONLY);
//    boundingBoxKernel.setArg(1, boundingBoxBuffer);

//    drawable.setAutoSwapBufferMode(true); TODO find out where to configure this
}

void GLCanvas::resizeGL(int w, int h) {
    // resize
}

void GLCanvas::paintGL() {
    auto* gl = QOpenGLContext::currentContext()->functions();

    std::vector<cl::Memory> objs { imageCL };

    if (settings.recompute()) {
        queue.enqueueAcquireGLObjects(&objs);

        if (settings.clear()) {
//            queue.enqueueNDRangeKernel(clearKernel, 0, 0, width, height, 0, 0);
            clearKernelWrapper.runOn(queue);
        }

        newtonKernelWrapper.runOn(queue);

        queue.enqueueReleaseGLObjects(&objs); // TODO encapsulate in ImageCL wrapper / prob exists in cl.hpp

        if (settings.waitClear()) {
            queue.finish();
        }

        if (settings.computeD()) {
            real dResult = newtonKernelWrapper.computeD();
            settings.metricsLog().dim().write();
        }
    }

    if (settings->saveScreenshot() || saveScreenshotOnce) {
        queue.finish();
        queue.enqueueAcquireGLObjects(&objs);
//        queue.enqueueReadImage(imageCL, true);
        queue.finish();
//        String info = "h = " + h.getValue() + "; bounds = " +
//                      minX.getValue() + "," + minY.getValue() + "," + maxX.getValue() + "," + maxY.getValue();
//        SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss");
//        try {
//            saveScreenshot(String.format("./result/%05d.png", cnt), h.getValue());//+ format.format(new Date()) + "(" + info + ").png");
//            ++cnt;
//        } catch (IOException e) {
//            System.err.println("unable to save screenshot: " + e.getMessage());
//        }
        queue.enqueueReleaseGLObjects(&objs);
        saveScreenshotOnce = false;
    }

    if (settings->recompute()) {
        if (settings->evolve()) {
            newtonKernelWrapper->evolve();
            if (settings->autoscale()) {
                newtonKernelWrapper->autoscale();
            }
        }
    }

    gl->glClear(GL_COLOR_BUFFER_BIT);
    if (settings->postClear()) {
        gl->glUseProgram(postClearProgram);
    } else {
        gl->glUseProgram(program);
    }
    {
        gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
        {
            gl->glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
    }
}

cl::Device findGLCompatibleDevice(cl::Platform platform) {
    for (auto device : platform.getDevices(CL_DEVICE_TYPE_GPU) {
        if (device.isGLMemorySharingSupported()) {
            return device;
        }
    }
}

void GLCanvas::initCLSide(QOpenGLContext *context) {
    cl::Platform chosenPlatform = cl::Platform::getDefault();
    cl::Device chosenDevice = findGLCompatibleDevice(chosenPlatform);


//    return null;

    clContext = cl::Context(chosenDevice);

    queue = cl::CommandQueue(clContext, chosenDevice);

    cl::Program nkp = compileBuiltInAlgorithm(clContext, "newton_fractal", "-cl-no-signed-zeros");
    cl::Kernel nkk = cl::Kernel (nkp, "newton_fractal");

    newtonKernelWrapper.initWith(clContext, nkk);

    clearKernel = clContext.createProgram(Util.loadSourceFile("cl/clear_kernel.cl"))
            .build()
            .createCLKernel("clear");
    computeKernel = clContext.createProgram(Util.loadSourceFile("cl/compute_non_transparent.cl"))
            .build()
            .createCLKernel("compute");
    boundingBoxKernel = clContext.createProgram(Util.loadSourceFile("cl/compute_bounding_box.cl"))
            .build(define("WIDTH", width),
                   define("HEIGHT", height)).createCLKernel("compute_bounding_box");
}

void GLCanvas::initGLSide(QOpenGLFunctions *gl, std::vector<int> &buffer) {

    texture = util::createTexture(gl, buffer, width, height);
    postClearProgram = util::createProgram(gl, vertexShader, fragmentClearShader);
    program = util::createProgram(gl, vertexShader, fragmentShader);

    // get location of var "tex"
    int textureLocation = gl->glGetUniformLocation(program, "tex");
    gl->glUseProgram(program);
    {
        texture->bind();
        gl->glActiveTexture(GL_TEXTURE0);
        // tell opengl that a texture named "tex" points to GL_TEXTURE0
        gl->glUniform1i(textureLocation, 0);
    }

    // create VBO containing draw information
    vertexBufferObject = util::createVBO(gl, vertexData);

    // set buffer attribs
    gl->glBindBuffer(GL_ARRAY_BUFFER, vertexBufferObject);
    {
        gl->glEnableVertexAttribArray(0);
        // tell opengl that first 16 values govern vertices positions (see vertexShader)
        gl->glVertexAttribPointer(0, 2, GL_FLOAT, false, 0, nullptr);
        gl->glEnableVertexAttribArray(1);
        // tell opengl that remaining values govern fragment positions (see vertexShader)
        gl->glVertexAttribPointer(1, 2, GL_FLOAT, false, 0, 4 * 4 * 2);
    }
    gl->glBindBuffer(GL_ARRAY_BUFFER, 0);

    // set clear color to dark red
    gl->glClearColor(0.2f, 0.0f, 0.0f, 1.0f);
}
