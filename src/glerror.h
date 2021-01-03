#ifndef GLERROR_H
#define GLERROR_H

#ifdef __cplusplus
extern "C" {
#endif

//#include "dll.h"
#define AZUREEXPORT

AZUREEXPORT const char * __glError(const char *, const int);
 
/*
Usage
[... some opengl calls]
glError();
*/
#define _glError() __glError(__FILE__, __LINE__)

#ifdef __cplusplus
}
#endif
 
#endif
