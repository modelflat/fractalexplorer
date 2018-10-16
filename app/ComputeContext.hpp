#ifndef FRACTALEXPLORER_COMPUTEINTERFACE_HPP
#define FRACTALEXPLORER_COMPUTEINTERFACE_HPP

#include <memory>
#include <string>
#import "../../../../../../Program Files (x86)/AMD APP SDK/3.0/include/CL/cl.hpp"



class MyGLCanvasWrapper {

    bool profile = false;
    long long MAX_TIME = 1000000000 / 30; // 30 fps

    float vertexData[] {
        // texture vertex coords: x, y
        1.0f, 1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, 1.0f,
        // texture UV coords (for texture mapping
        1.f, 1.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f,
    };
    cl::Context clContext;
    cl::CommandQueue queue;

    NewtonKernelWrapper newtonKernelWrapper = new NewtonKernelWrapper();

    cl::Kernel clearKernel;
    cl::Image2DGL imageCL;
    int program;
    QTexture texture;
    int vertexBufferObject;
    int width;
    int height;
    int postClearProgram;
    // GL canvas;
    bool doEvolveBounds = true;

//    EvolvableParameter minX = new EvolvableParameter(false, -1, 0.0, -10.0, 0.0);
//    EvolvableParameter maxX = new EvolvableParameter(false, 1, -0.0, 0.0, 10.0);
//    EvolvableParameter minY = new EvolvableParameter(false, -1, 0.0, -10.0, 0.0);
//    EvolvableParameter maxY = new EvolvableParameter(false, 1, -0.0, -0.0, 10.0);
//    int t = 1;
//    boolean direction = true;
//    EvolvableParameter h = new EvolvableParameter(false,
//                                                      new ApproachingEvolutionStrategy(
//                                                              ApproachingEvolutionStrategy.InternalStrategy.STOP_AT_POINT_OF_INTEREST, .4),
//                                                      1.0, .001, 0, 10);
    EvolvableParameter cReal = new EvolvableParameter(false, .5, -.05, -1.0, 1.0);
    EvolvableParameter cImag = new EvolvableParameter(false, -.5, .05, -1.0, 1.0);
    bool doCLClear = true;
    bool doPostCLear = false;
    bool doWaitForCL = true;
    bool doEvolve = true;
    bool doRecomputeFractal = true;
    bool autoscale = false;
    CLKernel boundingBoxKernel;
    CLBuffer<IntBuffer> boundingBoxBuffer;
    CLKernel computeKernel;
    bool doComputeD = false;
    CLBuffer<IntBuffer> count;
    PrintStream logFile;
    bool saveScreenshot = true;

public:

    MyGLCanvasWrapper(FPSAnimator animator, int width, int height, int canvasWidth, int canvasHeight) {
        this->width = width;
        this->height = height;

//    canvas = new GLCanvas(
//        new GLCapabilities(GLProfile.getMaxProgrammableCore( true ))
//    );

//    canvas.addGLEventListener(this);

//  canvas.setSize(canvasWidth, canvasHeight);//width, height);
//  animator.add(canvas);
    }

    void init(GLAutoDrawable drawable) {
        // perform CL initialization
        GLContext context = drawable.getContext();

        System.out.println(
                context.getGL().glGetString(context.getGL().GL_VENDOR)
        );

        try {
            initCLSide(context);
        } catch (NoSuchResourceException e) {
            e.printStackTrace();
        }

        // perform GL initialization
        GL4 gl = drawable.getGL().getGL4();
        IntBuffer buffer = GLBuffers.newDirectIntBuffer(width * height);
        try {
            initGLSide(gl, buffer);
        } catch (NoSuchResourceException e) {
            e.printStackTrace(); // TODO decide what to do
        }

        // interop
        imageCL = clContext.createFromGLTexture2d(
                buffer,
                texture.getTarget(), texture.getTextureObject(),
                0, CLMemory.Mem.WRITE_ONLY);

        // kernels
        newtonKernelWrapper.setBounds(minX.getValue(), maxX.getValue(), minY.getValue(), maxY.getValue());
        newtonKernelWrapper.setC(cReal.getValue(), cImag.getValue());
        newtonKernelWrapper.setH(h.getValue());
        newtonKernelWrapper.setT(t);
        newtonKernelWrapper.setBackwards(direction);
        newtonKernelWrapper.setImage(imageCL);

        clearKernel.setArg(0, imageCL);

        computeKernel.setArg(0, imageCL);

        boundingBoxKernel.setArg(0, imageCL);
        boundingBoxBuffer = clContext.createIntBuffer(4, CLMemory.Mem.WRITE_ONLY);
        boundingBoxKernel.setArg(1, boundingBoxBuffer);

        drawable.setAutoSwapBufferMode(true);
    }

    void dispose(GLAutoDrawable drawable) {
        clContext.release();
        if (logFile != null) {
            logFile.close();
        }
    }

    int cnt = 0;;

public void reshape(GLAutoDrawable drawable, int x, int y, int width, int height) {
        drawable.getGL().getGL4().glViewport(x, y, width, height);
    }

    @Override
    void display(GLAutoDrawable drawable) {
        GL4 gl = drawable.getGL().getGL4();

        long tt = 0, dd = 0, cd = 0, ev = 0, as = 0, pw = 0;
        if (doRecomputeFractal) {
            queue.putAcquireGLObject(imageCL);

            tt = nanoTime();
            if (doCLClear) {
                queue.put2DRangeKernel(clearKernel,
                                       0, 0,
                                       width, height, 0, 0)
                    //.finish()
                        ;
            }

            tt = (nanoTime() - tt);

            dd = nanoTime();
            newtonKernelWrapper.runOn(queue);
            queue.putReleaseGLObject(imageCL);
            dd = (nanoTime() - dd);

            pw = nanoTime();
            if (doWaitForCL) {
                queue.finish();
            }
            pw = nanoTime() - pw;

            cd = nanoTime();
            if (doComputeD) {
                if (logFile == null) {
                    SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss");
                    try {
                        logFile = new PrintStream("h-d@" + format.format(new Date()) + ".log");
                    } catch (FileNotFoundException e) {
                        e.printStackTrace();
                    }
                }
                double dResult = computeD();
                System.out.println(h.getValue() + "\t" + dResult);
                logFile.println(h.getValue() + "\t" + dResult);
            }
            cd = (nanoTime() - cd);
        }

        if (saveScreenshot) {
            queue.finish();
            queue.putAcquireGLObject(imageCL);
            // refresh image
            queue.putReadImage(imageCL, true).finish();
            String info = "h = " + h.getValue() + "; bounds = " +
                          minX.getValue() + "," + minY.getValue() + "," + maxX.getValue() + "," + maxY.getValue();
            SimpleDateFormat format = new SimpleDateFormat("yyyy-MM-dd_HH-mm-ss");
            try {
                saveScreenshot(String.format("./result/%05d.png", cnt), h.getValue());//+ format.format(new Date()) + "(" + info + ").png");
                ++cnt;
            } catch (IOException e) {
                System.err.println("unable to save screenshot: " + e.getMessage());
            }
            queue.putReleaseGLObject(imageCL);
            saveScreenshot = false;
        }

        if (doRecomputeFractal) { // separated to include saveScreenshot logic (above)
            if (doEvolve) {
                ev = nanoTime();
                evolve();
                ev = (nanoTime() - ev);

                as = nanoTime();
                if (autoscale) {
                    autoscale();
                }
                as = (nanoTime() - as);
            }
        }

        if (profile) {
            System.out.printf("wait: %d us\tclear: %d us\tdraw: %d us\td: %d us\tevolve: %d us\tautoscale: %d us\t" +
                              "remaining: %d us\n",
                              pw / 1000, tt / 1000, dd / 1000, cd / 1000, ev / 1000, as / 1000,
                              (MAX_TIME - (tt + dd + cd + ev + as + pw)) / 1000
            );
        }

        // long drw = System.nanoTime();
        gl.glClear(GL_COLOR_BUFFER_BIT);
        if (doPostCLear) {
            gl.glUseProgram(postClearProgram);
        } else {
            gl.glUseProgram(program);
        }
        {
            gl.glBindBuffer(GL4.GL_ARRAY_BUFFER, vertexBufferObject);
            {
                gl.glDrawArrays(GL4.GL_TRIANGLE_FAN, 0, 4);
            }
        }
        // System.out.println((System.nanoTime() - drw) / 1000 + "us");
    }

// ============================================================================
// Private methods
// ============================================================================

private void initGLSide(GL4 gl, IntBuffer buffer) throws NoSuchResourceException {
        // create texture
        texture = GLUtil.createTexture(gl, buffer, width, height);

        postClearProgram = GLUtil.createProgram(gl, vertexShader, fragmentClearShader);
        // create program w/ 2 shaders
        program = GLUtil.createProgram(gl, vertexShader, fragmentShader);
        // get location of var "tex"
        int textureLocation = gl.glGetUniformLocation(program, "tex");
        gl.glUseProgram(program);
        {
            texture.enable(gl);
            texture.bind(gl);
            gl.glActiveTexture(GL4.GL_TEXTURE0);
            // tell opengl that a texture named "tex" points to GL_TEXTURE0
            gl.glUniform1i(textureLocation, 0);
        }

        // create VBO containing draw information
        vertexBufferObject = GLUtil.createVBO(gl, vertexData);
        // set buffer attribs
        gl.glBindBuffer(GL4.GL_ARRAY_BUFFER, vertexBufferObject);
        {
            gl.glEnableVertexAttribArray(0);
            // tell opengl that first 16 values govern vertices positions (see vertexShader)
            gl.glVertexAttribPointer(0, 2, GL4.GL_FLOAT, false, 0,
                    0);
            gl.glEnableVertexAttribArray(1);
            // tell opengl that remaining values govern fragment positions (see vertexShader)
            gl.glVertexAttribPointer(1, 2, GL4.GL_FLOAT, false, 0,
                    4 * 4 * 2);
        }
        gl.glBindBuffer(GL4.GL_ARRAY_BUFFER, 0);

        // set clear color to dark red
        gl.glClearColor(0.2f, 0.0f, 0.0f, 1.0f);
}

private void initCLSide(GLContext context) throws NoSuchResourceException {

        CLPlatform chosenPlatform = CLPlatform.getDefault();
        CLDevice chosenDevice = GLUtil.findGLCompatibleDevice(chosenPlatform);
//        System.out.println(chosenPlatform);
//        System.out.println(chosenDevice);
//        System.out.println(context);
//        System.out.println("\n\n\n" + context.getGLExtensionsString() + "\n\n\n" + context.getGLSLVersionString());

        if (chosenDevice == null) {
            throw new RuntimeException(String.format("no device supporting GL sharing on platform %s!",
                                                     chosenPlatform.toString()));
        }
        clContext = CLGLContext.create(context, chosenDevice);

        queue = chosenDevice.createCommandQueue();

        newtonKernelWrapper.initWith(
        clContext,
        clContext.createProgram(Util.loadSourceFile("cl/newton_fractal.cl"))
        .build(
        "-I ./src/main/resources/cl/include -cl-no-signed-zeros")
        .createCLKernel("newton_fractal")
);
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

private void evolve() {
    if (h.evolve()) {
        newtonKernelWrapper.setH(h.getValue());
    }

    if (cReal.evolve() | cImag.evolve()) {
        newtonKernelWrapper.setC(cReal.getValue(), cImag.getValue());
    }

    if (minY.evolve() | maxY.evolve() | maxX.evolve() | minX.evolve()) {
        newtonKernelWrapper.setBounds(minX.getValue(), maxX.getValue(), minY.getValue(), maxY.getValue());
    }
}

private void autoscale() {
    if (Math.abs(h.getValue()) < 1e-12) {
        System.out.println("autoscale stopped");
        autoscale = false;
    }
    // + 1) compute bounding box
    // - 2) save previous bounds to history (optional)
    // + 3) change bounds corresponding to computed box
    boundingBoxBuffer.getBuffer().put(0).put(width - 1).put(0).put(height - 1).rewind();
    queue.putWriteBuffer(boundingBoxBuffer, true)
            .put1DRangeKernel(boundingBoxKernel,
                              0, 4, 0)
            .finish()
            .putReadBuffer(boundingBoxBuffer, true);
    boxToBounds(boundingBoxBuffer.getBuffer());
}

private void boxToBounds(IntBuffer box) {
    double sX = maxX.getValue() - minX.getValue();
    double sY = maxY.getValue() - minY.getValue();

    int padding = 2;

    int dxMin = box.get(0) - padding;
    int dxMax = box.get(1) + padding;
    int dyMin = box.get(3) + padding;
    int dyMax = box.get(2) - padding;

    double newMinX = dxMin <= 0 ?
                     minX.getValue() :
                     minX.getValue() + ((double) dxMin / width) * sX;
    double newMaxX = dxMax >= width - 1 ?
                     maxX.getValue() :
                     maxX.getValue() - (1 - (double) dxMax / width) * sX;
    double newMinY = dyMin >= height - 1 ?
                     minY.getValue() :
                     minY.getValue() + (1 - (double) dyMin / height) * sY;
    double newMaxY = dyMax <= 0 ?
                     maxY.getValue() :
                     maxY.getValue() - ((double) dyMax / height) * sY;

    newtonKernelWrapper.setBounds(newMinX, newMaxX, newMinY, newMaxY);

    minX.setValue(newMinX);
    maxX.setValue(newMaxX);
    minY.setValue(newMinY);
    maxY.setValue(newMaxY);
}

private double computeD() {
    int startBoxSize = 5;
    int endBoxSize = width / 16;
    double[][] boxes = new double[2][endBoxSize - startBoxSize];
    // for all box sizes from 2 to maxXSize
    for (int k = startBoxSize, bI = 0; k < endBoxSize; k++, bI++) {
        int sizeY = (height / k) + (height % k == 0 ? 0 : 1);
        int sizeX = (width / k) + (width % k == 0 ? 0 : 1);
        int totalBoxes = sizeX * sizeY;
        int activeBoxes = computeActiveBoxes(k);
//            System.out.println(String.format("%d / %d", activeBoxes, totalBoxes));
        boxes[0][bI] = log(1.0 / k);
        boxes[1][bI] = log(activeBoxes);
//            System.out.println(String.format("[%d] (%d) %f %f", bI, k, Math.log(1.0 / k), Math.log(activeBoxes)));
    }
    return BoxCountingCalculator.normalEquations2d(boxes[0], boxes[1])[0];
}

private int computeActiveBoxes(int boxSize) {
    computeKernel.setArg(1, boxSize);
    if (count == null) {
        count = clContext.createIntBuffer(1, CLMemory.Mem.READ_WRITE);
    }
    count.getBuffer().put(0, 0);
    computeKernel.setArg(2, count);
    queue.putWriteBuffer(count, true)
            .put2DRangeKernel(
                    computeKernel, 0, 0, width, height, 0, 0);
    queue.finish().putReadBuffer(count, true);
    return count.getBuffer().get(0);
}

private void saveScreenshot(String filename, double h) throws IOException {
        IntBuffer imageBuffer = imageCL.getBuffer();
        WritableImage image = new WritableImage(width, height);
        PixelWriter writer = image.getPixelWriter();
        // TODO RGBA -> ARGB, color change occurs
        writer.setPixels(0, 0, width, height, PixelFormat.getIntArgbInstance(), imageBuffer, width);
        // TODO optimize, this is horrible
        BufferedImage im = SwingFXUtils.fromFXImage(image, null);
        Graphics2D g2d = im.createGraphics();
        Font f = new Font(Font.SANS_SERIF, Font.BOLD, 24);
        String line = String.format("h = %g", h);
        g2d.setFont(f);
        Rectangle2D metrics = f.getStringBounds(line, g2d.getFontRenderContext());
        g2d.setBackground(new Color(255,255,255));
//        g2d.fillRect(3,0, (int)metrics.getWidth() + 2, (int)metrics.getHeight() + 2);
//        g2d.setColor(new Color(0, 120, 244));
//        g2d.drawString(line, 4, 24);
        ImageIO.write(SwingFXUtils.fromFXImage(SwingFXUtils.toFXImage(im, null), null), "png",
        new File(filename));
}

// ============================================================================
// Getters/setters
// ============================================================================

public EvolvableParameter getMinX() {
    return minX;
}

public EvolvableParameter getMaxX() {
    return maxX;
}

public EvolvableParameter getMinY() {
    return minY;
}

public EvolvableParameter getMaxY() {
    return maxY;
}

public EvolvableParameter getH() {
    return h;
}

public EvolvableParameter getcReal() {
    return cReal;
}

public EvolvableParameter getcImag() {
    return cImag;
}

public boolean doCLClear() {
    return doCLClear;
}

public void setDoCLClear(boolean doCLClear) {
    this.doCLClear = doCLClear;
}

public boolean doPostCLear() {
    return doPostCLear;
}

public void setDoPostCLear(boolean doPostCLear) {
    this.doPostCLear = doPostCLear;
}

public boolean doWaitForCL() {
    return doWaitForCL;
}

public void setDoWaitForCL(boolean doWaitForCL) {
    this.doWaitForCL = doWaitForCL;
}

public boolean doEvolve() {
    return doEvolve;
}

public void setDoEvolve(boolean doEvolve) {
    this.doEvolve = doEvolve;
}

public boolean doEvolveBounds() {
    return doEvolveBounds;
}

public void setDoEvolveBounds(boolean doEvolveBounds) {
    this.doEvolveBounds = doEvolveBounds;
}

public boolean doRecomputeFractal() {
    return doRecomputeFractal;
}

public void setDoRecomputeFractal(boolean doRecomputeFractal) {
    this.doRecomputeFractal = doRecomputeFractal;
}

public GLCanvas getCanvas() {
    return canvas;
}

public boolean doComputeD() {
    return doComputeD;
}

public void setDoComputeD(boolean doComputeD) {
    this.doComputeD = doComputeD;
}

public boolean doSaveScreenshot() {
    return saveScreenshot;
}

public void setSaveScreenshot(boolean saveScreenshot) {
    this.saveScreenshot = saveScreenshot;
}

public int toggleT() {
    t = -t;
    newtonKernelWrapper.setT(t);
    return t;
}

public boolean toggleDirection() {
    direction = !direction;
    newtonKernelWrapper.setBackwards(direction);
    return direction;
}

public NewtonKernelWrapper getNewtonKernelWrapper() {
    return newtonKernelWrapper;
}
}



#endif //FRACTALEXPLORER_COMPUTEINTERFACE_HPP
