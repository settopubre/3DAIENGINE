#include "vector3d.h"
#include "graph_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <sys/stat.h>
#include <omp.h>

Graph3D* graph3d_create(void) {
    Graph3D* graph = (Graph3D*)malloc(sizeof(Graph3D));
    if (!graph) return NULL;
    graph->node_count = 0;
    graph->nodes = NULL;
    graph->distance_matrix = NULL;
    graph->adjacency_matrix = NULL;
    graph->sigma = 1.0;
    graph->threshold = 0.1;
    graph->k_nearest = 5;
    return graph;
}

void graph3d_destroy(Graph3D* graph) {
    if (!graph) return;
    if (graph->nodes) {
        free(graph->nodes);
        graph->nodes = NULL;
    }
    if (graph->distance_matrix) {
        for (int i = 0; i < graph->node_count; i++) {
            if (graph->distance_matrix[i]) {
                free(graph->distance_matrix[i]);
            }
        }
        free(graph->distance_matrix);
        graph->distance_matrix = NULL;
    }
    if (graph->adjacency_matrix) {
        for (int i = 0; i < graph->node_count; i++) {
            if (graph->adjacency_matrix[i]) {
                free(graph->adjacency_matrix[i]);
            }
        }
        free(graph->adjacency_matrix);
        graph->adjacency_matrix = NULL;
    }
    free(graph);
}

static int parse_node_file(const char* filepath, Vec3* out_vec, int id) {
    DEFER_FILE(fp) = fopen(filepath, "r");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot open %s\n", filepath);
        return 0;
    }
    double x, y, z;
    if (fscanf(fp, "%lf %lf %lf", &x, &y, &z) != 3) {
        fprintf(stderr, "ERROR: Invalid format in %s (expected: x y z)\n", filepath);
        return 0;
    }
    *out_vec = vec3_create(x, y, z);
    out_vec->id = id;
    return 1;
}

static int is_node_file(const char* name) {
    if (strncmp(name, "node_", 5) != 0) return 0;
    const char* ext = strrchr(name, '.');
    if (!ext) return 0;
    if (strcmp(ext, ".dat") != 0) return 0;
    return 1;
}

static int count_node_files(const char* dir_path) {
    DEFER_DIR(dir) = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "ERROR: Cannot open directory %s\n", dir_path);
        return -1;
    }
    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_node_file(entry->d_name)) {
            count++;
        }
    }
    return count;
}

int graph3d_load_nodes_from_flat_dir(Graph3D* graph, const char* dir_path) {
    if (!graph || !dir_path) return 0;
    int file_count = count_node_files(dir_path);
    if (file_count <= 0) {
        fprintf(stderr, "ERROR: No node_*.dat files found in %s\n", dir_path);
        return 0;
    }
    printf("  Found %d node files\n", file_count);
    graph->node_count = file_count;
    graph->nodes = (Vec3*)malloc(file_count * sizeof(Vec3));
    if (!graph->nodes) {
        fprintf(stderr, "ERROR: Memory allocation failed\n");
        return 0;
    }
    DEFER_DIR(dir) = opendir(dir_path);
    if (!dir) {
        fprintf(stderr, "ERROR: Cannot open directory %s\n", dir_path);
        return 0;
    }
    int loaded = 0;
    struct dirent* entry;
    char filepath[1024];
    while ((entry = readdir(dir)) != NULL && loaded < file_count) {
        if (!is_node_file(entry->d_name)) continue;
        snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);
        if (parse_node_file(filepath, &graph->nodes[loaded], loaded)) {
            loaded++;
        }
    }
    graph->node_count = loaded;
    printf("  Successfully loaded %d nodes\n", loaded);
    return 1;
}

int graph3d_compute_distance_matrix(Graph3D* graph) {
    if (!graph || graph->node_count == 0) return 0;
    int n = graph->node_count;
    graph->distance_matrix = (double**)malloc(n * sizeof(double*));
    if (!graph->distance_matrix) return 0;
    for (int i = 0; i < n; i++) {
        graph->distance_matrix[i] = (double*)malloc(n * sizeof(double));
        if (!graph->distance_matrix[i]) return 0;
    }
    
    #pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < n; i++) {
        graph->distance_matrix[i][i] = 0.0;
        for (int j = i + 1; j < n; j++) {
            double dist = vec3_distance(&graph->nodes[i], &graph->nodes[j]);
            graph->distance_matrix[i][j] = dist;
            graph->distance_matrix[j][i] = dist;
        }
    }
    
    printf("  [OPENMP] Threads active: %d\n", omp_get_max_threads());
    printf("  Distance matrix computed: %dx%d\n", n, n);
    return 1;
}

int graph3d_compute_gaussian_adjacency(Graph3D* graph) {
    if (!graph || !graph->distance_matrix || graph->node_count == 0) return 0;
    int n = graph->node_count;
    graph->adjacency_matrix = (double**)malloc(n * sizeof(double*));
    if (!graph->adjacency_matrix) return 0;
    for (int i = 0; i < n; i++) {
        graph->adjacency_matrix[i] = (double*)calloc(n, sizeof(double));
        if (!graph->adjacency_matrix[i]) return 0;
    }
    double sigma_sq_2 = 2.0 * graph->sigma * graph->sigma;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double d = graph->distance_matrix[i][j];
            double w = exp(-(d * d) / sigma_sq_2);
            if (w >= graph->threshold) {
                graph->adjacency_matrix[i][j] = w;
                graph->adjacency_matrix[j][i] = w;
            }
        }
    }
    int edge_count = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (graph->adjacency_matrix[i][j] > 0.0) edge_count++;
        }
    }
    printf("  Gaussian adjacency: %d edges (sigma=%.3f, threshold=%.3f)\n", 
           edge_count, graph->sigma, graph->threshold);
    return 1;
}

int graph3d_export_distance_matrix_csv(const Graph3D* graph, const char* filename) {
    if (!graph || !graph->distance_matrix || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot create %s\n", filename);
        return 0;
    }
    int n = graph->node_count;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fprintf(fp, "%.6f", graph->distance_matrix[i][j]);
            if (j < n - 1) fprintf(fp, ",");
        }
        fprintf(fp, "\n");
    }
    printf("  Exported: %s\n", filename);
    return 1;
}

int graph3d_export_adjacency_matrix_csv(const Graph3D* graph, const char* filename) {
    if (!graph || !graph->adjacency_matrix || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot create %s\n", filename);
        return 0;
    }
    int n = graph->node_count;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            fprintf(fp, "%.6f", graph->adjacency_matrix[i][j]);
            if (j < n - 1) fprintf(fp, ",");
        }
        fprintf(fp, "\n");
    }
    printf("  Exported: %s\n", filename);
    return 1;
}

int graph3d_export_node_positions_csv(const Graph3D* graph, const char* filename) {
    if (!graph || !graph->nodes || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot create %s\n", filename);
        return 0;
    }
    fprintf(fp, "id,x,y,z\n");
    for (int i = 0; i < graph->node_count; i++) {
        fprintf(fp, "%d,%.6f,%.6f,%.6f\n", 
                graph->nodes[i].id, 
                graph->nodes[i].x, 
                graph->nodes[i].y, 
                graph->nodes[i].z);
    }
    printf("  Exported: %s\n", filename);
    return 1;
}

int graph3d_export_obj(const Graph3D* graph, const char* filename) {
    if (!graph || !graph->nodes || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot create %s\n", filename);
        return 0;
    }
    
    int edge_count = 0;
    for (int i = 0; i < graph->node_count; i++) {
        for (int j = i + 1; j < graph->node_count; j++) {
            if (graph->adjacency_matrix && graph->adjacency_matrix[i][j] > 0.0) edge_count++;
        }
    }
    
    fprintf(fp, "# 3DAI Engine - Robot Spatial Graph | Nodes: %d | Edges: %d\n", graph->node_count, edge_count);
    
    for (int i = 0; i < graph->node_count; i++) {
        fprintf(fp, "v %.6f %.6f %.6f\n", graph->nodes[i].x, graph->nodes[i].y, graph->nodes[i].z);
    }
    
    if (graph->adjacency_matrix) {
        for (int i = 0; i < graph->node_count; i++) {
            for (int j = i + 1; j < graph->node_count; j++) {
                if (graph->adjacency_matrix[i][j] > 0.0) {
                    fprintf(fp, "l %d %d\n", i + 1, j + 1);
                }
            }
        }
    }
    
    printf("  Exported: %s\n", filename);
    return 1;
}

int graph3d_export_edges_json(const Graph3D* graph, const char* filename) {
    if (!graph || !graph->adjacency_matrix || !graph->distance_matrix || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot create %s\n", filename);
        return 0;
    }
    
    fprintf(fp, "[\n");
    int first = 1;
    for (int i = 0; i < graph->node_count; i++) {
        for (int j = i + 1; j < graph->node_count; j++) {
            if (graph->adjacency_matrix[i][j] > 0.0) {
                if (!first) fprintf(fp, ",\n");
                fprintf(fp, "  {\"src\":%d,\"dst\":%d,\"weight\":%.6f,\"dist\":%.6f}",
                        i, j, graph->adjacency_matrix[i][j], graph->distance_matrix[i][j]);
                first = 0;
            }
        }
    }
    fprintf(fp, "\n]\n");
    
    printf("  Exported: %s\n", filename);
    return 1;
}

int graph3d_export_metadata_csv(const Graph3D* graph, const char* filename) {
    if (!graph || !graph->nodes || !filename) return 0;
    DEFER_FILE(fp) = fopen(filename, "w");
    if (!fp) {
        fprintf(stderr, "ERROR: Cannot create %s\n", filename);
        return 0;
    }
    
    fprintf(fp, "id,x,y,z,degree,avg_weight\n");
    for (int i = 0; i < graph->node_count; i++) {
        int degree = 0;
        double weight_sum = 0.0;
        if (graph->adjacency_matrix) {
            for (int j = 0; j < graph->node_count; j++) {
                if (graph->adjacency_matrix[i][j] > 0.0) {
                    degree++;
                    weight_sum += graph->adjacency_matrix[i][j];
                }
            }
        }
        double avg_weight = (degree > 0) ? weight_sum / degree : 0.0;
        fprintf(fp, "%d,%.6f,%.6f,%.6f,%d,%.6f\n",
                graph->nodes[i].id,
                graph->nodes[i].x,
                graph->nodes[i].y,
                graph->nodes[i].z,
                degree,
                avg_weight);
    }
    
    printf("  Exported: %s\n", filename);
    return 1;
}

void graph3d_print_summary(const Graph3D* graph) {
    if (!graph) return;
    printf("\n=== GRAPH SUMMARY ===\n");
    printf("Nodes: %d\n", graph->node_count);
    printf("Sigma: %.3f\n", graph->sigma);
    printf("Threshold: %.3f\n", graph->threshold);
    if (graph->nodes && graph->node_count > 0) {
        printf("\nFirst 3 nodes:\n");
        int show = graph->node_count < 3 ? graph->node_count : 3;
        for (int i = 0; i < show; i++) {
            printf("  Node %d: (%.3f, %.3f, %.3f)\n", 
                   graph->nodes[i].id, 
                   graph->nodes[i].x, 
                   graph->nodes[i].y, 
                   graph->nodes[i].z);
        }
    }
    if (graph->adjacency_matrix && graph->node_count > 0) {
        printf("\nAdjacency matrix (first 5x5):\n");
        int show = graph->node_count < 5 ? graph->node_count : 5;
        for (int i = 0; i < show; i++) {
            printf("  ");
            for (int j = 0; j < show; j++) {
                printf("%.3f ", graph->adjacency_matrix[i][j]);
            }
            printf("\n");
        }
    }
    printf("======================\n\n");
}
