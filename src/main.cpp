#include "graph_engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <chrono>

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#endif

static int dir_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

static int file_is_empty_or_missing(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return 1;
    return (st.st_size == 0);
}

static int needs_data_generation(const char* dir_path) {
    printf("[DEBUG] Scanning %s...\n", dir_path);
    if (!dir_exists(dir_path)) {
        #ifdef _WIN32
        mkdir(dir_path);
        #else
        mkdir(dir_path, 0755);
        #endif
        return 1;
    }
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/node_001.dat", dir_path);
    if (file_is_empty_or_missing(filepath)) {
        printf("[DEBUG] node_001.dat is empty or missing\n");
        return 1;
    }
    printf("[DEBUG] Found valid node files\n");
    return 0;
}

static int is_node_file(const char* name) {
    if (strncmp(name, "node_", 5) != 0) return 0;
    const char* ext = strrchr(name, '.');
    if (!ext) return 0;
    if (strcmp(ext, ".dat") != 0) return 0;
    return 1;
}

static int count_node_files(const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (!dir) return 0;
    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (is_node_file(entry->d_name)) {
            count++;
        }
    }
    closedir(dir);
    return count;
}

static void clear_sim_data(const char* dir_path) {
    DIR* dir = opendir(dir_path);
    if (!dir) return;
    struct dirent* entry;
    char filepath[1024];
    while ((entry = readdir(dir)) != NULL) {
        if (is_node_file(entry->d_name)) {
            snprintf(filepath, sizeof(filepath), "%s/%s", dir_path, entry->d_name);
            unlink(filepath);
        }
    }
    closedir(dir);
}

static void generate_sample_nodes(const char* dir_path, int count) {
    srand(42);
    char filepath[1024];
    for (int i = 0; i < count; i++) {
        snprintf(filepath, sizeof(filepath), "%s/node_%03d.dat", dir_path, i + 1);
        printf("[DEBUG] Generating %s", filepath);
        FILE* fp = fopen(filepath, "w");
        if (!fp) {
            fprintf(stderr, "FATAL: Cannot create %s\n", filepath);
            exit(1);
        }
        double x = (rand() % 1000) / 100.0 - 5.0;
        double y = (rand() % 1000) / 100.0 - 5.0;
        double z = (rand() % 1000) / 100.0 - 5.0;
        fprintf(fp, "%.6f %.6f %.6f\n", x, y, z);
        fclose(fp);
        printf(" -> x=%.6f y=%.6f z=%.6f\n", x, y, z);
    }
}

static void print_help(const char* prog) {
    printf("3DAI Engine - Terminal 3D Simulator\n");
    printf("Usage: %s [-n NODES] [-s SIGMA] [-t THRESH] [-h]\n", prog);
    printf("  -n INT   Number of nodes to generate (default: 10)\n");
    printf("  -s FLOAT Sigma for Gaussian weighting (default: 2.0)\n");
    printf("  -t FLOAT Adjacency threshold (default: 0.01)\n");
    printf("  -h       Show this help\n");
}

int main(int argc, char* argv[]) {
    int node_count = 10;
    double sigma = 2.0;
    double threshold = 0.01;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            node_count = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-s") == 0 && i + 1 < argc) {
            sigma = atof(argv[++i]);
        } else if (strcmp(argv[i], "-t") == 0 && i + 1 < argc) {
            threshold = atof(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0) {
            print_help(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_help(argv[0]);
            return 1;
        }
    }
    
    printf("\n========================================\n");
    printf("        3DAI ENGINE - TERMINAL SIM      \n");
    printf("========================================\n");
    printf("[CONFIG] nodes=%d sigma=%.3f threshold=%.3f\n\n", node_count, sigma, threshold);
    
    auto t_start = std::chrono::high_resolution_clock::now();
    
    if (!dir_exists("sim_data")) {
        #ifdef _WIN32
        mkdir("sim_data");
        #else
        mkdir("sim_data", 0755);
        #endif
    }
    
    if (needs_data_generation("sim_data")) {
        printf("[PREP] Generating sample data...\n");
        generate_sample_nodes("sim_data", node_count);
    } else {
        int existing = count_node_files("sim_data");
        if (existing != node_count) {
            printf("[SCALE] Node count changed (%d -> %d). Clearing sim_data/...\n", existing, node_count);
            clear_sim_data("sim_data");
            generate_sample_nodes("sim_data", node_count);
        }
    }
    
    if (!dir_exists("output")) {
        #ifdef _WIN32
        mkdir("output");
        #else
        mkdir("output", 0755);
        #endif
    }
    
    Graph3D* graph = graph3d_create();
    if (!graph) {
        fprintf(stderr, "FATAL: Failed to create graph\n");
        return 1;
    }
    
    graph->sigma = sigma;
    graph->threshold = threshold;
    
    printf("\n[1/5] Loading nodes from flat directory...\n");
    if (!graph3d_load_nodes_from_flat_dir(graph, "sim_data")) {
        fprintf(stderr, "FATAL: Failed to load nodes\n");
        graph3d_destroy(graph);
        return 1;
    }
    printf("[1/5] Loading nodes... DONE\n\n");
    
    printf("[2/5] Computing distance matrix...\n");
    if (!graph3d_compute_distance_matrix(graph)) {
        fprintf(stderr, "FATAL: Failed to compute distance matrix\n");
        graph3d_destroy(graph);
        return 1;
    }
    printf("[2/5] Computing distance matrix... DONE\n\n");
    
    printf("[3/5] Building Gaussian adjacency...\n");
    if (!graph3d_compute_gaussian_adjacency(graph)) {
        fprintf(stderr, "FATAL: Failed to compute adjacency matrix\n");
        graph3d_destroy(graph);
        return 1;
    }
    printf("[3/5] Building Gaussian adjacency... DONE\n\n");
    
    printf("[4/5] Exporting data files...\n");
    if (!graph3d_export_node_positions_csv(graph, "output/node_positions.csv")) {
        fprintf(stderr, "FATAL: Failed to export node positions\n");
        graph3d_destroy(graph);
        return 1;
    }
    if (!graph3d_export_distance_matrix_csv(graph, "output/distance_matrix.csv")) {
        fprintf(stderr, "FATAL: Failed to export distance matrix\n");
        graph3d_destroy(graph);
        return 1;
    }
    if (!graph3d_export_adjacency_matrix_csv(graph, "output/adjacency_matrix.csv")) {
        fprintf(stderr, "FATAL: Failed to export adjacency matrix\n");
        graph3d_destroy(graph);
        return 1;
    }
    graph3d_export_obj(graph, "output/robot_scene.obj");
    graph3d_export_edges_json(graph, "output/edges.json");
    graph3d_export_metadata_csv(graph, "output/metadata.csv");
    printf("[4/5] Exporting data files... DONE\n");
    
    printf("[5/5] Printing summary...\n");
    graph3d_print_summary(graph);
    
    graph3d_destroy(graph);
    
    auto t_end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(t_end - t_start).count();
    
    printf("\n========================================\n");
    printf("    PIPELINE COMPLETE - READY FOR R     \n");
    printf("[PERF] Execution time: %.4f seconds\n", elapsed);
    printf("========================================\n\n");
    
    return 0;
}
