#ifndef RECORDER_REGION_H
#define RECORDER_REGION_H
#include <windows.h>
typedef void (*RegionResult)(int x,int y,int width,int height,int accepted);
int region_show(HINSTANCE instance,RegionResult result);
#endif
