#ifndef MATHS_H
#define MATHS_H

#include "core/define.h"
#include "math_type.h"

#define PI 3.14159265358979323846f
#define SQRT_2 1.41421356237309504880f
#define SQRT_3 1.73205080756887729252f
#define SEC2MS_MULTI 1000.0f
#define MS2SEC_MULTI 0.001f
#define INFINITE 1e30f
#define EPSILON 1.192092896e-07f

#define PI2 (2.0f * PI)
#define HALF_PI (0.5f * PI)
#define QUARTER_PI (0.25f * PI)
#define DEG2RAD (PI / 180.0f)
#define RAD2DEG (180.0f / PI)

INL bool mat4_has_nan(const mat4 *m) {
    for (uint64_t i = 0; i < 16; i++) {
        if (m->data[i] != m->data[i]) {
            return true;
        }
    }
    return false;
}

int32_t random(void);
int32_t random_in_range(int32_t min, int32_t max);
float frandom(void);
float frandom_in_range(float min, float max);

INL bool power_of_2(uint64_t value) {
    return (value != 0) && ((value & (value - 1)) == 0);
}

INL float deg_to_rad(float degree) { return degree * DEG2RAD; }
INL float rad_to_deg(float rad) { return rad * RAD2DEG; }

/************************************
 * VECTOR 2
 ************************************/
vec2 vec2_create(float x, float y);
vec2 vec2_zero(void);
vec2 vec2_one(void);
vec2 vec2_add(vec2 v1, vec2 v2);
vec2 vec2_sub(vec2 v1, vec2 v2);
vec2 vec2_multi(vec2 v1, vec2 v2);
vec2 vec2_divide(vec2 v1, vec2 v2);

float vec2_length_square(vec2 v);
float vec2_length(vec2 v);
float vec2_dot(vec2 v1, vec2 v2);
float vec2_distance(vec2 v1, vec2 v2);

/************************************
 * VECTOR 3
 ************************************/
vec3 vec3_create(float x, float y, float z);
vec3 vec3_zero(void);
vec3 vec3_one(void);
vec3 vec3_add(vec3 v1, vec3 v2);
vec3 vec3_sub(vec3 v1, vec3 v2);
vec3 vec3_multi(vec3 v1, vec3 v2);
vec3 vec3_divide(vec3 v1, vec3 v2);
vec3 vec3_multi_scalar(vec3 v, float s);

float vec3_length_square(vec3 v);
float vec3_length(vec3 v);
float vec3_dot(vec3 v1, vec3 v2);
vec3 vec3_cross(vec3 v1, vec3 v2);
void vec3_normalized(vec3 *v);
vec3 vec3_get_normalized(vec3 v);
float vec3_distance(vec3 v1, vec3 v2);
bool vec3_compared(vec3 v1, vec3 v2, float tolerance);

/************************************
 * VECTOR 4
 ************************************/
vec4 vec4_create(float x, float y, float z, float w);
vec4 vec4_zero(void);
vec4 vec4_one(void);
vec4 vec4_add(vec4 v1, vec4 v2);
vec4 vec4_sub(vec4 v1, vec4 v2);
vec4 vec4_multi(vec4 v1, vec4 v2);
vec4 vec4_divide(vec4 v1, vec4 v2);

float vec4_length_square(vec4 v);
float vec4_length(vec4 v);
void vec4_normalized(vec4 *v);
vec4 vec4_get_normalized(vec4 v);
float vec4_dot_float(float a0, float a1, float a2, float a3, float b0, float b1,
                     float b2, float b3);

/************************************
 * MATRIX 4
 ************************************/
mat4 mat4_identity(void);
mat4 mat4_translate(vec3 pos);
mat4 mat4_scale(vec3 scale);

vec3 mat4_forward(mat4 matrix);
vec3 mat4_backward(mat4 matrix);
vec3 mat4_up(mat4 matrix);
vec3 mat4_down(mat4 matrix);
vec3 mat4_left(mat4 matrix);
vec3 mat4_right(mat4 matrix);

vec4 mat4_mul_vec4(mat4 m, vec4 v);

mat4 mat4_euler_x(float angle_rad);
mat4 mat4_euler_y(float angle_rad);
mat4 mat4_euler_z(float angle_rad);
mat4 mat4_euler_xyz(float x_rad, float y_rad, float z_rad);

mat4 mat4_row_multi(mat4 m1, mat4 m2);
mat4 mat4_column_multi(mat4 m1, mat4 m2);

mat4 mat4_row_ortho(float left, float right, float bottom, float top,
                    float near, float far);
mat4 mat4_column_ortho(float left, float right, float bottom, float top,
                       float near, float far);
mat4 mat4_row_perspective(float fov_rad, float aspect_ratio, float near,
                          float far);
mat4 mat4_column_perspective(float fov_rad, float aspect_ratio, float near,
                             float far);

mat4 mat4_row_lookat(vec3 pos, vec3 target, vec3 up);
mat4 mat4_column_lookat(vec3 pos, vec3 target, vec3 up);

mat4 mat4_transpose(mat4 matrix);
mat4 mat4_inverse_rigid(mat4 matrix);
mat4 mat4_inverse(mat4 matrix);

/************************************
 * QUATERNION
 ************************************/
quat quat_identity(void);
quat quat_get_normalized(quat q);
quat quat_conjugate(quat q);
quat quat_inverse(quat q);

float quat_normalized(quat q);
float quat_dot(quat q1, quat q2);

quat quat_multi(quat q1, quat q2);
quat quat_from_axis_angle(vec3 axis, float angle, bool normalize);
quat quat_slerp(quat q_0, quat q_1, float percentage);

mat4 quat_to_rotation_matrix(quat q, vec3 center);

#endif // MATHS_H
