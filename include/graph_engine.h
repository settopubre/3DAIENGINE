#ifndef GRAPH_ENGINE_H
#define GRAPH_ENGINE_H
#pragma once

#include "vector3d.h"
#include <stddef.h>

typedef struct {
    int node_count;
    Vec3* nodes;
    double** distance_matrix;
    double** adjacency_matrix;
    double sigma;
    double threshold;
    int k_nearest;
} Graph3D;

#ifdef __cplusplus
extern "C" {
#endif

Graph3D* graph3d_create(void);
void graph3d_destroy(Graph3D* graph);
int graph3d_load_nodes_from_flat_dir(Graph3D* graph, const char* dir_path);
int graph3d_compute_distance_matrix(Graph3D* graph);
int graph3d_compute_gaussian_adjacency(Graph3D* graph);
int graph3d_export_distance_matrix_csv(const Graph3D* graph, const char* filename);
int graph3d_export_adjacency_matrix_csv(const Graph3D* graph, const char* filename);
int graph3d_export_node_positions_csv(const Graph3D* graph, const char* filename);
int graph3d_export_obj(const Graph3D* graph, const char* filename);
int graph3d_export_edges_json(const Graph3D* graph, const char* filename);
int graph3d_export_metadata_csv(const Graph3D* graph, const char* filename);
void graph3d_print_summary(const Graph3D* graph);

#ifdef __cplusplus
}
#endif

#endif
