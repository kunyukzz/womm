#ifndef MATH_TYPE_H
#define MATH_TYPE_H

typedef union VEC2_ALIGN {
    struct {
        float x, y, _padz1, _padw1;
    } comp1;
    struct {
        float r, g, _padz2, _padw2;
    } comp2;
    struct {
        float u, v, _padz3, _padw3;
    } comp3;
    float elements[4];
} vec2;

typedef union VEC3_ALIGN {
    struct {
        float x, y, z, _pad1;
    } comp1;
    struct {
        float r, g, b, _pad2;
    } comp2;
    struct {
        float u, v, t, _pad3;
    } comp3;
    float elements[4];
} vec3;

typedef union VEC4_ALIGN {
    struct {
        float x, y, z, w;
    } comp1;
    struct {
        float r, g, b, a;
    } comp2;
    struct {
        float u, v, t, s;
    } comp3;
    float elements[4];
} vec4;

typedef union MAT4_ALIGN {
    float data[16];
} mat4;

typedef vec4 quat;

typedef struct vertex_3d {
    vec3 position;
    vec3 normal;
    vec2 texcoord;
} vertex_3d;

typedef struct vertex_2d {
    vec2 position;
    vec2 texcoord;
} vertex_2d;

#endif // MATH_TYPE_H
