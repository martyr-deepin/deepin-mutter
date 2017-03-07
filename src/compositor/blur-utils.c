#include "blur-utils.h"
#include <stdlib.h>
#include <string.h>

static void build_gaussian_blur_kernel(int* pradius, float* offset, float* weight)
{
    int radius = *pradius;
    radius += (radius + 1) % 2;
    int sz = (radius+2)*2-1;
    int N = sz-1;
    float sigma = 1.0f;

    float sum = powf(2, N);
    weight[radius+1] = 1.0;
    for (int i = 1; i < radius+2; i++) {
        weight[radius-i+1] = weight[radius-i+2] * (N-i+1) / i;
    }
    sum -= (weight[radius+1] + weight[radius]) * 2.0;

    for (int i = 0; i < radius; i++) {
        offset[i] = (float)i*sigma;
        weight[i] /= sum;
    }

    *pradius = radius;

    radius = (radius+1)/2;
    for (int i = 1; i < radius; i++) {
        float w = weight[i*2] + weight[i*2-1];
        float off = (offset[i*2] * weight[i*2] + offset[i*2-1] * weight[i*2-1]) / w;
        offset[i] = off;
        weight[i] = w;
    }
    *pradius = radius;
}

char *build_shader(int direction, int radius, float* offsets, float *weight)
{
    char* ret = (char*)malloc(10000);
    if (!ret) {
        return NULL;
    }

    char *p = ret;

    if (direction == VERTICAL) {
        int sz = sprintf(p, "vec2 tc = cogl_tex_coord.st;\n"
                "cogl_texel = texture2D(cogl_sampler, tc) * %f;\n",
                weight[0]);
        p += sz;

        for (int i = 1; i < radius; i++) {
            sz = sprintf(p, 
                    "cogl_texel += texture2D(cogl_sampler, tc - vec2(0.0, %f/resolution.y)) * %f; \n"
                    "cogl_texel += texture2D(cogl_sampler, tc + vec2(0.0, %f/resolution.y)) * %f; \n",
                    offsets[i], weight[i],
                    offsets[i], weight[i]);
            p += sz;
        }
    } else {

        int sz = sprintf(p,
                "vec2 tc = vec2(cogl_tex_coord.s, 1.0 - cogl_tex_coord.t); \n"
                "cogl_texel = texture2D(cogl_sampler, tc) * %f;\n",
                weight[0]);
        p += sz;

        for (int i = 1; i < radius; i++) {
            sz = sprintf(p, 
                    "cogl_texel += texture2D(cogl_sampler, tc - vec2(%f/resolution.x, 0.0)) * %f; \n"
                    "cogl_texel += texture2D(cogl_sampler, tc + vec2(%f/resolution.x, 0.0)) * %f; \n",
                    offsets[i], weight[i],
                    offsets[i], weight[i]);
            p += sz;
        }
    }
    
    return ret;
}

