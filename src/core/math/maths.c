#include "maths.h"
#include "platform/window.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool random_seed = false;

int32_t random(void) {
    if (!random_seed) {
        srand((uint32_t)get_abs_time());
        random_seed = true;
    }
    return rand();
}

int32_t random_in_range(int32_t min, int32_t max) {
    if (!random_seed) {
        srand((uint32_t)get_abs_time());
        random_seed = true;
    }
    return (rand() % (max - min + 1)) + min;
}

float frandom(void) { return (float)random() / (float)RAND_MAX; }

float frandom_in_range(float min, float max) {
    return min + ((float)random() / ((float)RAND_MAX / (max / min)));
}

/************************************
 * VECTOR 2
 ************************************/
vec2 vec2_create(float x, float y) {
    return (vec2){.comp1.x = x, .comp1.y = y};
}

vec2 vec2_zero(void) { return (vec2){.comp1.x = 0.0f, .comp1.y = 0.0f}; }
vec2 vec2_one(void) { return (vec2){.comp1.x = 1.0f, .comp1.y = 1.0f}; }

vec2 vec2_add(vec2 v1, vec2 v2) {
    return (vec2){.comp1.x = v1.comp1.x + v2.comp1.x,
                  .comp1.y = v1.comp1.y + v2.comp1.y};
}

vec2 vec2_sub(vec2 v1, vec2 v2) {
    return (vec2){.comp1.x = v1.comp1.x - v2.comp1.x,
                  .comp1.y = v1.comp1.y - v2.comp1.y};
}

vec2 vec2_multi(vec2 v1, vec2 v2) {
    return (vec2){.comp1.x = v1.comp1.x * v2.comp1.x,
                  .comp1.y = v1.comp1.y * v2.comp1.y};
}

vec2 vec2_divide(vec2 v1, vec2 v2) {
    return (vec2){.comp1.x = v1.comp1.x / v2.comp1.x,
                  .comp1.y = v1.comp1.y / v2.comp1.y};
}

float vec2_length_square(vec2 v) {
    return v.comp1.x * v.comp1.x + v.comp1.y * v.comp1.y;
}

float vec2_length(vec2 v) { return sqrtf(vec2_length_square(v)); }

float vec2_dot(vec2 v1, vec2 v2) {
    return v1.comp1.x * v2.comp1.x + v1.comp1.y * v2.comp1.y;
}

float vec2_distance(vec2 v1, vec2 v2) {
    vec2 dist = (vec2){.comp1.x = v1.comp1.x - v2.comp1.x,
                       .comp1.y = v1.comp1.y - v2.comp1.y};
    return vec2_length(dist);
}

/************************************
 * VECTOR 3
 ************************************/
vec3 vec3_create(float x, float y, float z) {
    return (vec3){.comp1.x = x, .comp1.y = y, .comp1.z = z};
}

vec3 vec3_zero(void) {
    return (vec3){.comp1.x = 0.0f, .comp1.y = 0.0f, .comp1.z = 0.0f};
}

vec3 vec3_one(void) {
    return (vec3){.comp1.x = 1.0f, .comp1.y = 1.0f, .comp1.z = 1.0f};
}

vec3 vec3_add(vec3 v1, vec3 v2) {
    return (vec3){.comp1.x = v1.comp1.x + v2.comp1.x,
                  .comp1.y = v1.comp1.y + v2.comp1.y,
                  .comp1.z = v1.comp1.z + v2.comp1.z};
}

vec3 vec3_sub(vec3 v1, vec3 v2) {
    return (vec3){.comp1.x = v1.comp1.x - v2.comp1.x,
                  .comp1.y = v1.comp1.y - v2.comp1.y,
                  .comp1.z = v1.comp1.z - v2.comp1.z};
}

vec3 vec3_multi(vec3 v1, vec3 v2) {
    return (vec3){.comp1.x = v1.comp1.x * v2.comp1.x,
                  .comp1.y = v1.comp1.y * v2.comp1.y,
                  .comp1.z = v1.comp1.z * v2.comp1.z};
}

vec3 vec3_divide(vec3 v1, vec3 v2) {
    return (vec3){.comp1.x = v1.comp1.x / v2.comp1.x,
                  .comp1.y = v1.comp1.y / v2.comp1.y,
                  .comp1.z = v1.comp1.z / v2.comp1.z};
}

vec3 vec3_multi_scalar(vec3 v, float s) {
    return (vec3){.comp1.x = v.comp1.x * s,
                  .comp1.y = v.comp1.y * s,
                  .comp1.z = v.comp1.z * s};
}

float vec3_length_square(vec3 v) {
    return v.comp1.x * v.comp1.x + v.comp1.y * v.comp1.y +
           v.comp1.z * v.comp1.z;
}

float vec3_length(vec3 v) { return sqrtf(vec3_length_square(v)); }

float vec3_dot(vec3 v1, vec3 v2) {
    return v1.comp1.x * v2.comp1.x + v1.comp1.y * v2.comp1.y +
           v1.comp1.z * v2.comp1.z;
}

vec3 vec3_cross(vec3 v1, vec3 v2) {
    return (vec3){.comp1.x = v1.comp1.y * v2.comp1.z - v1.comp1.z * v2.comp1.y,
                  .comp1.y = v1.comp1.z * v2.comp1.x - v1.comp1.x * v2.comp1.z,
                  .comp1.z = v1.comp1.x * v2.comp1.y - v1.comp1.y * v2.comp1.x};
}

void vec3_normalized(vec3 *v) {
    const float length = vec3_length(*v);
    v->comp1.x /= length;
    v->comp1.y /= length;
    v->comp1.z /= length;
}

vec3 vec3_get_normalized(vec3 v) {
    vec3_normalized(&v);
    return v;
}

float vec3_distance(vec3 v1, vec3 v2) {
    vec3 dist = (vec3){.comp1.x = v1.comp1.x - v2.comp1.x,
                       .comp1.y = v1.comp1.y - v2.comp1.y,
                       .comp1.z = v1.comp1.z - v2.comp1.z};
    return vec3_length(dist);
}

bool vec3_compared(vec3 v1, vec3 v2, float tolerance) {
    if (fabsf(v1.comp1.x - v2.comp1.x) > tolerance) {
        return false;
    }
    if (fabsf(v1.comp1.y - v2.comp1.y) > tolerance) {
        return false;
    }
    if (fabsf(v1.comp1.z - v2.comp1.z) > tolerance) {
        return false;
    }
    return true;
}

/************************************
 * VECTOR 4
 ************************************/
vec4 vec4_create(float x, float y, float z, float w) {
    vec4 out;
    out.comp1.x = x;
    out.comp1.y = y;
    out.comp1.z = z;
    out.comp1.w = w;
    return out;
}

vec4 vec4_zero(void) {
    return (vec4){.comp1.x = 0.0f,
                  .comp1.y = 0.0f,
                  .comp1.z = 0.0f,
                  .comp1.w = 0.0f};
}

vec4 vec4_one(void) {
    return (vec4){.comp1.x = 1.0f,
                  .comp1.y = 1.0f,
                  .comp1.z = 1.0f,
                  .comp1.w = 1.0f};
}

vec4 vec4_add(vec4 v1, vec4 v2) {
    return (vec4){.comp1.x = v1.comp1.x + v2.comp1.x,
                  .comp1.y = v1.comp1.y + v2.comp1.y,
                  .comp1.z = v1.comp1.z + v2.comp1.z,
                  .comp1.w = v1.comp1.w + v2.comp1.w};
}

vec4 vec4_sub(vec4 v1, vec4 v2) {
    return (vec4){.comp1.x = v1.comp1.x - v2.comp1.x,
                  .comp1.y = v1.comp1.y - v2.comp1.y,
                  .comp1.z = v1.comp1.z - v2.comp1.z,
                  .comp1.w = v1.comp1.w - v2.comp1.w};
}

vec4 vec4_multi(vec4 v1, vec4 v2) {
    return (vec4){.comp1.x = v1.comp1.x * v2.comp1.x,
                  .comp1.y = v1.comp1.y * v2.comp1.y,
                  .comp1.z = v1.comp1.z * v2.comp1.z,
                  .comp1.w = v1.comp1.w * v2.comp1.w};
}

vec4 vec4_divide(vec4 v1, vec4 v2) {
    return (vec4){.comp1.x = v1.comp1.x / v2.comp1.x,
                  .comp1.y = v1.comp1.y / v2.comp1.y,
                  .comp1.z = v1.comp1.z / v2.comp1.z,
                  .comp1.w = v1.comp1.w / v2.comp1.w};
}

float vec4_length_square(vec4 v) {
    return v.comp1.x * v.comp1.x + v.comp1.y * v.comp1.y +
           v.comp1.z * v.comp1.z + v.comp1.w * v.comp1.w;
}

float vec4_length(vec4 v) { return sqrtf(vec4_length_square(v)); }

void vec4_normalized(vec4 *v) {
    const float length = vec4_length(*v);
    v->comp1.x /= length;
    v->comp1.y /= length;
    v->comp1.z /= length;
    v->comp1.w /= length;
}

vec4 vec4_get_normalized(vec4 v) {
    vec4_normalized(&v);
    return v;
}

float vec4_dot_float(float a0, float a1, float a2, float a3, float b0, float b1,
                     float b2, float b3) {
    return (a0 * b0) + (a1 * b1) + (a2 * b2) + (a3 * b3);
}

/************************************
 * MATRIX 4
 ************************************/
mat4 mat4_identity(void) {
    mat4 result;
    memset(result.data, 0, sizeof(float) * 16);
    result.data[0] = 1.0f;
    result.data[5] = 1.0f;
    result.data[10] = 1.0f;
    result.data[15] = 1.0f;
    return result;
}

mat4 mat4_translate(vec3 pos) {
    mat4 result = mat4_identity();

    result.data[12] = pos.comp1.x;
    result.data[13] = pos.comp1.y;
    result.data[14] = pos.comp1.z;

    return result;
}

mat4 mat4_scale(vec3 scale) {
    mat4 result = mat4_identity();

    result.data[0] = scale.comp1.x;
    result.data[5] = scale.comp1.y;
    result.data[10] = scale.comp1.z;

    return result;
}

vec3 mat4_forward(mat4 matrix) {
    vec3 result;

    result.comp1.x = -matrix.data[2];
    result.comp1.y = -matrix.data[6];
    result.comp1.z = -matrix.data[10];
    vec3_normalized(&result);

    return result;
}

vec3 mat4_backward(mat4 matrix) {
    vec3 result;

    result.comp1.x = matrix.data[2];
    result.comp1.y = matrix.data[6];
    result.comp1.z = matrix.data[10];
    vec3_normalized(&result);

    return result;
}

vec3 mat4_up(mat4 matrix) {
    vec3 result;

    result.comp1.x = matrix.data[1];
    result.comp1.y = matrix.data[5];
    result.comp1.z = matrix.data[9];
    vec3_normalized(&result);

    return result;
}

vec3 mat4_down(mat4 matrix) {
    vec3 result;

    result.comp1.x = -matrix.data[1];
    result.comp1.y = -matrix.data[5];
    result.comp1.z = -matrix.data[9];
    vec3_normalized(&result);

    return result;
}

vec3 mat4_left(mat4 matrix) {
    vec3 result;

    result.comp1.x = -matrix.data[0];
    result.comp1.y = -matrix.data[4];
    result.comp1.z = -matrix.data[8];
    vec3_normalized(&result);

    return result;
}

vec3 mat4_right(mat4 matrix) {
    vec3 result;

    result.comp1.x = matrix.data[0];
    result.comp1.y = matrix.data[4];
    result.comp1.z = matrix.data[8];
    vec3_normalized(&result);

    return result;
}

mat4 mat4_euler_x(float angle_rad) {
    mat4 result = mat4_identity();
    float cos = cosf(angle_rad);
    float sin = sinf(angle_rad);

    result.data[5] = cos;
    result.data[6] = sin;
    result.data[9] = -sin;
    result.data[10] = cos;

    return result;
}

mat4 mat4_euler_y(float angle_rad) {
    mat4 result = mat4_identity();
    float cos = cosf(angle_rad);
    float sin = sinf(angle_rad);

    result.data[0] = cos;
    result.data[2] = -sin;
    result.data[8] = sin;
    result.data[10] = cos;

    return result;
}

mat4 mat4_euler_z(float angle_rad) {
    mat4 result = mat4_identity();
    float cos = cosf(angle_rad);
    float sin = sinf(angle_rad);

    result.data[0] = cos;
    result.data[1] = sin;
    result.data[4] = -sin;
    result.data[5] = cos;

    return result;
}

mat4 mat4_row_multi(mat4 m1, mat4 m2) {
    mat4 result;
    const float *m1_ptr = m1.data;
    const float *m2_ptr = m2.data;
    float *dst_ptr = result.data;

    for (int32_t i = 0; i < 4; ++i) {
        for (int32_t j = 0; j < 4; ++j) {
            *dst_ptr = m1_ptr[0] * m2_ptr[0 + j] + m1_ptr[1] * m2_ptr[4 + j] +
                       m1_ptr[2] * m2_ptr[8 + j] + m1_ptr[3] * m2_ptr[12 + j];
            dst_ptr++;
        }
        m1_ptr += 4;
    }

    return result;
}

mat4 mat4_column_multi(mat4 m1, mat4 m2) {
    mat4 result;
    for (int32_t col = 0; col < 4; ++col) {
        for (int32_t row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int32_t k = 0; k < 4; ++k) {
                sum += m1.data[k * 4 + row] * m2.data[col * 4 + k];
            }
            result.data[col * 4 + row] = sum;
        }
    }
    return result;
}

mat4 mat4_row_ortho(float left, float right, float bottom, float top,
                    float near, float far) {
    mat4 result;
    memset(result.data, 0, sizeof(float) * 16);

    float lr = 1.0f / (right - left);
    float bt = 1.0f / (top - bottom);
    float nf = 1.0f / (far - near);

    result.data[0] = 2.0f * lr;
    result.data[5] = 2.0f * bt;
    result.data[10] = -2.0f * nf;
    result.data[12] = -(right + left) * lr;
    result.data[13] = -(top + bottom) * bt;
    result.data[14] = -(far + near) * nf;
    result.data[15] = 1.0f;

    return result;
}

mat4 mat4_column_ortho(float left, float right, float bottom, float top,
                       float near, float far) {
    mat4 result;
    memset(result.data, 0, sizeof(float) * 16);

    float lr = 1.0f / (right - left);
    float bt = 1.0f / (top - bottom);
    float nf = 1.0f / (far - near);

    // Scale
    result.data[0] = 2.0f * lr;
    result.data[5] = 2.0f * bt;
    result.data[10] = -2.0f * nf;
    result.data[15] = 1.0f;

    // Translate
    result.data[12] = -(right + left) * lr;
    result.data[13] = -(top + bottom) * bt;
    result.data[14] = -(far + near) * nf;

    return result;
}

mat4 mat4_row_perspective(float fov_rad, float aspect_ratio, float near,
                          float far) {
    float half_tan = tanf(fov_rad * 0.5f);
    mat4 result;
    memset(result.data, 0, sizeof(float) * 16);

    result.data[0] = 1.0f / (aspect_ratio * half_tan);
    result.data[5] = -1.0f / half_tan;
    result.data[10] = far / (near - far);
    result.data[11] = -1.0f;
    result.data[14] = (far * near) / (near - far);

    return result;
}

mat4 mat4_column_perspective(float fov_rad, float aspect_ratio, float near,
                             float far) {
    float half_tan = tanf(fov_rad * 0.5f);
    mat4 result;
    memset(result.data, 0, sizeof(result.data));

    result.data[0] = 1.0f / (aspect_ratio * half_tan);
    result.data[5] = 1.0f / half_tan;
    result.data[10] = far / (near - far);
    result.data[11] = -1.0f;
    result.data[14] = (far * near) / (near - far);
    result.data[15] = 0.0f;

    return result;
}

mat4 mat4_row_lookat(vec3 pos, vec3 target, vec3 up) {
    mat4 result;
    vec3 z_axis = vec3_get_normalized(vec3_sub(target, pos));
    vec3 x_axis = vec3_get_normalized(vec3_cross(z_axis, up));
    vec3 y_axis = vec3_cross(x_axis, z_axis);

    result.data[0] = x_axis.comp1.x;
    result.data[1] = x_axis.comp1.y;
    result.data[2] = x_axis.comp1.z;
    result.data[3] = -vec3_dot(x_axis, pos);

    result.data[4] = y_axis.comp1.x;
    result.data[5] = y_axis.comp1.y;
    result.data[6] = y_axis.comp1.z;
    result.data[7] = -vec3_dot(y_axis, pos);

    result.data[8] = -z_axis.comp1.x;
    result.data[9] = -z_axis.comp1.y;
    result.data[10] = -z_axis.comp1.z;
    result.data[11] = vec3_dot(z_axis, pos);

    result.data[12] = 0;
    result.data[13] = 0;
    result.data[14] = 0;
    result.data[15] = 1.0f;

    return result;
}

mat4 mat4_column_lookat(vec3 pos, vec3 target, vec3 up) {
    mat4 result;
    vec3 z_axis = vec3_get_normalized(vec3_sub(target, pos));
    vec3 x_axis = vec3_get_normalized(vec3_cross(z_axis, up));
    vec3 y_axis = vec3_cross(x_axis, z_axis);

    result.data[0] = x_axis.comp1.x;
    result.data[1] = y_axis.comp1.x;
    result.data[2] = -z_axis.comp1.x;
    result.data[3] = 0;

    result.data[4] = x_axis.comp1.y;
    result.data[5] = y_axis.comp1.y;
    result.data[6] = -z_axis.comp1.y;
    result.data[7] = 0;

    result.data[8] = x_axis.comp1.z;
    result.data[9] = y_axis.comp1.z;
    result.data[10] = -z_axis.comp1.z;
    result.data[11] = 0;

    result.data[12] = -vec3_dot(x_axis, pos);
    result.data[13] = -vec3_dot(y_axis, pos);
    result.data[14] = vec3_dot(z_axis, pos);
    result.data[15] = 1.0f;

    return result;
}

mat4 mat4_transpose(mat4 matrix) {
    mat4 result;

    result.data[0] = matrix.data[0];
    result.data[4] = matrix.data[1];
    result.data[8] = matrix.data[2];
    result.data[12] = matrix.data[3];

    result.data[1] = matrix.data[4];
    result.data[5] = matrix.data[5];
    result.data[9] = matrix.data[6];
    result.data[13] = matrix.data[7];

    result.data[2] = matrix.data[8];
    result.data[6] = matrix.data[9];
    result.data[10] = matrix.data[10];
    result.data[14] = matrix.data[11];

    result.data[3] = matrix.data[12];
    result.data[7] = matrix.data[13];
    result.data[11] = matrix.data[14];
    result.data[15] = matrix.data[15];

    return result;
}

mat4 mat4_inverse_rigid(mat4 m) {
    mat4 inv = mat4_identity();

    // Transpose upper-left 3x3 (rotation)
    for (uint32_t r = 0; r < 3; ++r)
        for (uint32_t c = 0; c < 3; ++c)
            inv.data[c * 4 + r] = m.data[r * 4 + c];

    // Inverse translation = -R^T * T
    float tx = m.data[12];
    float ty = m.data[13];
    float tz = m.data[14];

    inv.data[12] = -(inv.data[0] * tx + inv.data[4] * ty + inv.data[8] * tz);
    inv.data[13] = -(inv.data[1] * tx + inv.data[5] * ty + inv.data[9] * tz);
    inv.data[14] = -(inv.data[2] * tx + inv.data[6] * ty + inv.data[10] * tz);

    // Bottom row stays [0 0 0 1]
    inv.data[3] = 0.0f;
    inv.data[7] = 0.0f;
    inv.data[11] = 0.0f;
    inv.data[15] = 1.0f;

    return inv;
}

mat4 mat4_inverse(mat4 matrix) {
    const float *m = matrix.data;
    float t0 = m[10] * m[15];
    float t1 = m[14] * m[11];
    float t2 = m[6] * m[15];
    float t3 = m[14] * m[7];
    float t4 = m[6] * m[11];
    float t5 = m[10] * m[7];
    float t6 = m[2] * m[15];
    float t7 = m[14] * m[3];
    float t8 = m[2] * m[11];
    float t9 = m[10] * m[3];
    float t10 = m[2] * m[7];
    float t11 = m[6] * m[3];
    float t12 = m[8] * m[13];
    float t13 = m[12] * m[9];
    float t14 = m[4] * m[13];
    float t15 = m[12] * m[5];
    float t16 = m[4] * m[9];
    float t17 = m[8] * m[5];
    float t18 = m[0] * m[13];
    float t19 = m[12] * m[1];
    float t20 = m[0] * m[9];
    float t21 = m[8] * m[1];
    float t22 = m[0] * m[5];
    float t23 = m[4] * m[1];

    mat4 result;
    float *o = result.data;

    o[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) -
           (t1 * m[5] + t2 * m[9] + t5 * m[13]);

    o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) -
           (t0 * m[1] + t7 * m[9] + t8 * m[13]);

    o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) -
           (t3 * m[1] + t6 * m[5] + t11 * m[13]);

    o[3] = (t5 * m[1] + t8 * m[5] + t11 * m[9]) -
           (t4 * m[1] + t9 * m[5] + t10 * m[9]);

    float d = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);
    o[0] = d * o[0];
    o[1] = d * o[1];
    o[2] = d * o[2];
    o[3] = d * o[3];

    o[4] = d * ((t1 * m[4] + t2 * m[8] + t5 * m[12]) -
                (t0 * m[4] + t3 * m[8] + t4 * m[12]));

    o[5] = d * ((t0 * m[0] + t7 * m[8] + t8 * m[12]) -
                (t1 * m[0] + t6 * m[8] + t9 * m[12]));

    o[6] = d * ((t3 * m[0] + t6 * m[4] + t11 * m[12]) -
                (t2 * m[0] + t7 * m[4] + t10 * m[12]));

    o[7] = d * ((t4 * m[0] + t9 * m[4] + t10 * m[8]) -
                (t5 * m[0] + t8 * m[4] + t11 * m[8]));

    o[8] = d * ((t12 * m[7] + t15 * m[11] + t16 * m[15]) -
                (t13 * m[7] + t14 * m[11] + t17 * m[15]));

    o[9] = d * ((t13 * m[3] + t18 * m[11] + t21 * m[15]) -
                (t12 * m[3] + t19 * m[11] + t20 * m[15]));

    o[10] = d * ((t14 * m[3] + t19 * m[7] + t22 * m[15]) -
                 (t15 * m[3] + t18 * m[7] + t23 * m[15]));

    o[11] = d * ((t17 * m[3] + t20 * m[7] + t23 * m[11]) -
                 (t16 * m[3] + t21 * m[7] + t22 * m[11]));

    o[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6]) -
                 (t16 * m[14] + t12 * m[6] + t15 * m[10]));

    o[13] = d * ((t20 * m[14] + t12 * m[2] + t19 * m[10]) -
                 (t18 * m[10] + t21 * m[14] + t13 * m[2]));

    o[14] = d * ((t18 * m[6] + t23 * m[14] + t15 * m[2]) -
                 (t22 * m[14] + t14 * m[2] + t19 * m[6]));

    o[15] = d * ((t22 * m[10] + t16 * m[2] + t21 * m[6]) -
                 (t20 * m[6] + t23 * m[10] + t17 * m[2]));

    return result;
}

mat4 mat4_euler_xyz(float x_rad, float y_rad, float z_rad) {
    mat4 rx = mat4_euler_x(x_rad);
    mat4 ry = mat4_euler_y(y_rad);
    mat4 rz = mat4_euler_z(z_rad);

    mat4 result = mat4_column_multi(rz, ry);
    result = mat4_column_multi(result, rx);

    return result;
}

vec4 mat4_mul_vec4(mat4 m, vec4 v) {
    vec4 r;
    for (uint32_t i = 0; i < 4; i++) {
        r.elements[i] =
            m.data[i] * v.elements[0] + m.data[i + 4] * v.elements[1] +
            m.data[i + 8] * v.elements[2] + m.data[i + 12] * v.elements[3];
    }
    return r;
}

/************************************
 * QUATERNION
 ************************************/
quat quat_identity(void) {
    return (quat){.comp1.x = 0.0f,
                  .comp1.y = 0.0f,
                  .comp1.z = 0.0f,
                  .comp1.w = 1.0f};
}

float quat_normalized(quat q) {
    return sqrtf(q.comp1.x * q.comp1.x + q.comp1.y * q.comp1.y +
                 q.comp1.z * q.comp1.z + q.comp1.w * q.comp1.w);
}

quat quat_get_normalized(quat q) {
    float normal = quat_normalized(q);
    return (quat){.comp1.x = q.comp1.x * normal,
                  .comp1.y = q.comp1.y * normal,
                  .comp1.z = q.comp1.z * normal,
                  .comp1.w = q.comp1.w * normal};
}

quat quat_conjugate(quat q) {
    return (quat){.comp1.x = -q.comp1.x,
                  .comp1.y = -q.comp1.y,
                  .comp1.z = -q.comp1.z,
                  .comp1.w = q.comp1.w};
}

quat quat_inverse(quat q) { return quat_get_normalized(quat_conjugate(q)); }

float quat_dot(quat q1, quat q2) {
    return q1.comp1.x * q2.comp1.x + q1.comp1.y * q2.comp1.y +
           q1.comp1.z * q2.comp1.z + q1.comp1.w * q2.comp1.w;
}

quat quat_multi(quat q1, quat q2) {
    quat result;

    result.comp1.x = q1.comp1.x * q2.comp1.w + q1.comp1.y * q2.comp1.z -
                     q1.comp1.z * q2.comp1.y + q1.comp1.z * q1.comp1.x;
    result.comp1.y = -q1.comp1.x * q2.comp1.z + q1.comp1.y * q2.comp1.w +
                     q1.comp1.z * q2.comp1.x + q1.comp1.w * q2.comp1.y;
    result.comp1.z = q1.comp1.x * q2.comp1.y - q1.comp1.y * q2.comp1.x +
                     q1.comp1.z * q2.comp1.w + q1.comp1.w * q2.comp1.z;
    result.comp1.w = -q1.comp1.x * q2.comp1.x - q1.comp1.y * q2.comp1.y -
                     q1.comp1.z * q2.comp1.z + q1.comp1.w * q2.comp1.w;

    return result;
}

quat quat_from_axis_angle(vec3 axis, float angle, bool normalize) {
    const float half_angle = 0.5f * angle;
    float s = sinf(half_angle);
    float c = cosf(half_angle);
    quat result = (quat){.comp1.x = s * axis.comp1.x,
                         .comp1.y = s * axis.comp1.y,
                         .comp1.z = s * axis.comp1.z,
                         .comp1.w = c};

    if (normalize) return quat_get_normalized(result);

    return result;
}

quat quat_slerp(quat q_0, quat q_1, float percentage) {
    quat out_quaternion;
    // Source: https://en.wikipedia.org/wiki/Slerp
    // Only unit quaternions are valid rotations.
    quat v0 = quat_get_normalized(q_0);
    quat v1 = quat_get_normalized(q_1);
    float dot = quat_dot(v0, v1);

    // If the dot product is negative, slerp won't take
    // the shorter path. Note that v1 and -v1 are equivalent when
    // the negation is applied to all four components. Fix by reversing one
    // quaternion
    if (dot < 0.0f) {
        v1.comp1.x = -v1.comp1.x;
        v1.comp1.y = -v1.comp1.y;
        v1.comp1.z = -v1.comp1.z;
        v1.comp1.w = -v1.comp1.w;
        dot = -dot;
    }
    const float DOT_THRESHOLD = 0.9995f;
    if (dot > DOT_THRESHOLD) {
        // If the inputs are too close for comfort, linearly interpolate &
        // normalized
        out_quaternion =
            (quat){.comp1.x =
                       v0.comp1.x + ((v1.comp1.x - v0.comp1.x) * percentage),
                   .comp1.y =
                       v0.comp1.y + ((v1.comp1.y - v0.comp1.y) * percentage),
                   .comp1.z =
                       v0.comp1.z + ((v1.comp1.z - v0.comp1.z) * percentage),
                   .comp1.w =
                       v0.comp1.w + ((v1.comp1.w - v0.comp1.w) * percentage)};
        return quat_get_normalized(out_quaternion);
    }

    // Since dot is in range [0, DOT_THRESHOLD], acos is safe
    float theta_0 = acosf(dot);
    float theta = theta_0 * percentage;
    float sin_theta = sinf(theta);
    float sin_theta_0 = sinf(theta_0);
    float s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
    float s1 = sin_theta / sin_theta_0;

    return (quat){.comp1.x = (v0.comp1.x * s0) + (v1.comp1.x * s1),
                  .comp1.y = (v0.comp1.y * s0) + (v1.comp1.y * s1),
                  .comp1.z = (v0.comp1.z * s0) + (v1.comp1.z * s1),
                  .comp1.w = (v0.comp1.w * s0) + (v1.comp1.w * s1)};
}

mat4 quat_to_rotation_matrix(quat q, vec3 center) {
    mat4 result;
    float *o = result.data;

    float xx = q.comp1.x * q.comp1.x;
    float yy = q.comp1.y * q.comp1.y;
    float zz = q.comp1.z * q.comp1.z;
    float xy = q.comp1.x * q.comp1.y;
    float xz = q.comp1.x * q.comp1.z;
    float yz = q.comp1.y * q.comp1.z;
    float wx = q.comp1.w * q.comp1.x;
    float wy = q.comp1.w * q.comp1.y;
    float wz = q.comp1.w * q.comp1.z;

    o[0] = 1.0f - 2.0f * (yy + zz);
    o[1] = 2.0f * (xx + wz);
    o[2] = 2.0f * (xz - wy);
    o[3] = center.comp1.x - center.comp1.x * o[0] - center.comp1.y * o[1] -
           center.comp1.z * o[2];

    o[4] = 2.0f * (xy - wz);
    o[5] = 1.0f - 2.0f * (xx + zz);
    o[6] = 2.0f * (yz + wx);
    o[7] = center.comp1.y - center.comp1.x * o[4] - center.comp1.y * o[5] -
           center.comp1.z * o[6];

    o[8] = 2.0f * (xz + wy);
    o[9] = 2.0f * (yz - wx);
    o[10] = 1.0f - 2.0f * (xx + yy);
    o[11] = center.comp1.z - center.comp1.x * o[8] - center.comp1.y * o[9] -
            center.comp1.z * o[10];

    o[12] = 0.0f;
    o[13] = 0.0f;
    o[14] = 0.0f;
    o[15] = 1.0f;

    return result;
}
