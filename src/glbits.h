#ifndef GLBITS_H
#define GLBITS_H

#if defined(__APPLE__)
#	include <OpenGL/gl3.h>
#elif defined(_WIN32)
#	include <Windows.h>
#	include <GL/gl3w.h>
#elif defined(__linux__)
#	include <GL/gl3w.h>
#	include <GL/gl.h>
#else
#	error 'unsupported platform'
#endif

#endif
