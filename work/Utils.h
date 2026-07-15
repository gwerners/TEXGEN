#ifndef UTILS_H
#define UTILS_H

#include <raylib.h>
#include <sys/stat.h>
#include <string>

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <time.h>
#include "gentexture.hpp"
#include "texgen_utils.h"

#ifdef _WIN32
#pragma comment(lib, "winmm.lib")
#include <windows.h>
#else
#include <string.h>
#include <sys/time.h>
#include "rlgl.h"  // OpenGL abstraction layer to multiple versions

#define USE_UNIFORM
#ifdef USE_UNIFORM
#include <random>
#endif
long timeGetTime();
int timeBeginPeriod(unsigned int period);
int timeEndPeriod(unsigned int period);

#endif
bool exists(const std::string& filename);
std::string readFile(bool debug, const std::string& filename);



Texture2D LoadTextureFromGenTexture(GenTexture tex);

#endif  // UTILS_H
