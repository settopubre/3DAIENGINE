#include "vector3d.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Vec3 vec3_create(double x, double y, double z) {
    Vec3 result;
    result.id = 0;
    result.x = x;
    result.y = y;
    result.z = z;
    return result;
}

double vec3_distance(const Vec3* a, const Vec3* b) {
    if (!a || !b) {
        fprintf(stderr, "ERROR: vec3_distance received NULL pointer\n");
        return -1.0;
    }
    double dx = a->x - b->x;
    double dy = a->y - b->y;
    double dz = a->z - b->z;
    return sqrt(dx*dx + dy*dy + dz*dz);
}

double vec3_determinant_3x3(const Vec3* a, const Vec3* b, const Vec3* c) {
    if (!a || !b || !c) {
        fprintf(stderr, "ERROR: vec3_determinant_3x3 received NULL pointer\n");
        return 0.0;
    }
    double minor1 = (b->y * c->z) - (b->z * c->y);
    double minor2 = (b->x * c->z) - (b->z * c->x);
    double minor3 = (b->x * c->y) - (b->y * c->x);
    double det = (a->x * minor1) - (a->y * minor2) + (a->z * minor3);
    const double EPSILON = 1e-10;
    if (fabs(det) < EPSILON) {
        return 0.0;
    }
    return det;
}

void vec3_cleanup(Vec3** vec_array) {
    if (vec_array && *vec_array) {
        free(*vec_array);
        *vec_array = NULL;
    }
}

void safe_vec3_demo(void) {
    printf("\n=== DEFER MACRO DEMONSTRATION ===\n");
    DEFER_FILE(log) = fopen("/tmp/3dai_demo.log", "w");
    if (!log) {
        printf("  [DEMO] Could not open log file (this is OK for demo)\n");
        return;
    }
    fprintf(log, "3DAI Engine - Vector3D Module Test Log\n");
    printf("  [DEFER] File opened - will auto-close on scope exit\n");
    DEFER_FREE(buffer) = malloc(1024);
    if (buffer) {
        memset(buffer, 0, 1024);
        printf("  [DEFER] 1KB allocated - will auto-free on scope exit\n");
    }
    DEFER_VEC3_ARRAY(points) = (Vec3*)malloc(5 * sizeof(Vec3));
    if (points) {
        for (int i = 0; i < 5; i++) {
            points[i] = vec3_create((double)i, (double)(i*2), (double)(i*3));
            points[i].id = i;
        }
        printf("  [DEFER] 5 Vec3 allocated - will auto-free on scope exit\n");
        if (5 >= 3) {
            double vol = vec3_determinant_3x3(&points[0], &points[1], &points[2]);
            printf("  [MATH] Determinant of first 3 points: %.6f\n", vol);
            if (fabs(vol) < 1e-10) {
                printf("  [MATH] -> Points are collinear/coplanar (volume zero)\n");
            } else {
                printf("  [MATH] -> Points form a proper 3D volume\n");
            }
        }
    }
    printf("  [DEFER] Function exiting - watch automatic cleanup...\n");
    printf("=== DEMO COMPLETE (all resources auto-freed) ===\n\n");
}
