#ifndef VECTOR3D_H
#define VECTOR3D_H
#pragma once

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

typedef struct {
    int id;
    double x;
    double y;
    double z;
} Vec3;

static inline void _defer_close_file(FILE** fp) {
    if (fp && *fp) {
        fclose(*fp);
        *fp = NULL;
    }
}

static inline void _defer_close_dir(DIR** dirp) {
    if (dirp && *dirp) {
        closedir(*dirp);
        *dirp = NULL;
    }
}

static inline void _defer_free(void** ptr) {
    if (ptr && *ptr) {
        free(*ptr);
        *ptr = NULL;
    }
}

static inline void _defer_cleanup_vec3(Vec3** vec);

#define DEFER_CLEANUP(cleanup_func, type, var) \
    __attribute__((cleanup(cleanup_func))) type var

#define DEFER_FILE(var) \
    __attribute__((cleanup(_defer_close_file))) FILE* var

#define DEFER_DIR(var) \
    __attribute__((cleanup(_defer_close_dir))) DIR* var

#define DEFER_FREE(var) \
    __attribute__((cleanup(_defer_free))) void* var

#define DEFER_VEC3_ARRAY(var) \
    __attribute__((cleanup(_defer_cleanup_vec3))) Vec3* var

#ifdef __cplusplus
extern "C" {
#endif

Vec3 vec3_create(double x, double y, double z);
double vec3_distance(const Vec3* a, const Vec3* b);
double vec3_determinant_3x3(const Vec3* a, const Vec3* b, const Vec3* c);
void vec3_cleanup(Vec3** vec_array);

#ifdef __cplusplus
}
#endif

static inline void _defer_cleanup_vec3(Vec3** vec) {
    vec3_cleanup(vec);
}

#endif
