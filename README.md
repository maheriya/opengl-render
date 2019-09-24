# opengl-render
Accelerated OpenGL rendering. OpenCV cv::cuda::GpuMat, detection boxes, etc.


## Opengl Accelerated Rendering
These are my experiments with OpenGL and and OpenCV's cv::cuda::GpuMat. 

A collection of programs that use OpenGL for rendering. Most are based on OpenGL tutorials around the Web which I used for learning OpenGL. 

The main program (main.cpp, and detection_window.cpp) had my implementation of rendering of object detection bounding boxes over OpenCV cv::cuda::GpuMat image directly in GPU. The example copies normal cv::Mat to cv::cuda::GpuMat; however, that is only for demonstration purposes. Normally, one would use Gstreamer pipeline to read in live video from camera sensor/s and operate on these frames directly in GPU. In my example, the image is read, copied to GPU, and then stays there throught the rending / processing.

## Build
Change directory to 'build' and run following:

```
cmake ../src
make -j8
```

## Run
```
./gl-render <path-to-image>
```

