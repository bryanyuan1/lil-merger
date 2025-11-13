#ifndef MERGE_H_
#define MERGE_H_

#include <tapa.h>
#include <ap_int.h>
#include <hls_vector.h>

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// void MergeKernel(
//     tapa::mmap<int> array_a,
//     tapa::mmap<int> array_b,
//     tapa::mmap<int> merged_result,
//     tapa::mmap<int> cycle_count,
//     const int size_a,
//     const int size_b
// );

struct ComparatorResult {
    bool is_geq;
    bool is_boundary;
    int output_value;
    int group_id;
};

void MergeKernel(
    tapa::mmap<hls::vector<int, 4>> array_a,
    tapa::mmap<hls::vector<int, 4>> array_b,
    tapa::mmap<hls::vector<int, 8>> merged_result,
    tapa::mmap<int> cycle_count,
    const int size_a,
    const int size_b
);

#endif