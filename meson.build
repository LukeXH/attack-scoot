project('simple-cam','cpp')

lib = library('libatkskt','src/image.cpp')
simple_cam = executable('simple-cam','src/cam_main.cpp', link_with: lib, dependencies: dependency('libcamera', required : true))
