#include <tapa.h>
#include "merge.h"

// Read array A from HBM and stream to merge unit
void read_array_a(
    tapa::mmap<int> array_mem,
    const int size,
    tapa::ostream<int> &q_out
) {
    for (int i = 0; i < size; i++) {
        #pragma HLS PIPELINE II=1
        q_out.write(array_mem[i]);
    }
}

// Read array B from HBM and stream to merge unit
void read_array_b(
    tapa::mmap<int> array_mem,
    const int size,
    tapa::ostream<int> &q_out
) {
    for (int i = 0; i < size; i++) {
        #pragma HLS PIPELINE II=1
        q_out.write(array_mem[i]);
    }
}

// Merge two sorted streams
void merge_streams(
    tapa::istream<int> &q_a,
    tapa::istream<int> &q_b,
    tapa::ostream<int> &q_out,
    const int size_a,
    const int size_b
) {
    int idx_a = 0;
    int idx_b = 0;
    int val_a = 0;
    int val_b = 0;
    bool valid_a = false;
    bool valid_b = false;
    
    // Read first elements
    if (size_a > 0) {
        val_a = q_a.read();
        valid_a = true;
        idx_a = 1;
    }
    if (size_b > 0) {
        val_b = q_b.read();
        valid_b = true;
        idx_b = 1;
    }
    
    // Merge loop
    while (valid_a || valid_b) {
        #pragma HLS PIPELINE II=1
        
        if (!valid_a) {
            // Only B has values left
            q_out.write(val_b);
            if (idx_b < size_b) {
                val_b = q_b.read();
                idx_b++;
            } else {
                valid_b = false;
            }
        } else if (!valid_b) {
            // Only A has values left
            q_out.write(val_a);
            if (idx_a < size_a) {
                val_a = q_a.read();
                idx_a++;
            } else {
                valid_a = false;
            }
        } else {
            // Both have values - compare and merge
            if (val_a <= val_b) {
                q_out.write(val_a);
                if (idx_a < size_a) {
                    val_a = q_a.read();
                    idx_a++;
                } else {
                    valid_a = false;
                }
            } else {
                q_out.write(val_b);
                if (idx_b < size_b) {
                    val_b = q_b.read();
                    idx_b++;
                } else {
                    valid_b = false;
                }
            }
        }
    }
}

// Write merged result to HBM
void write_result(
    tapa::istream<int> &q_in,
    tapa::mmap<int> result_mem,
    const int total_size,
    tapa::ostream<bool> &q_done
) {
    for (int i = 0; i < total_size; i++) {
        #pragma HLS PIPELINE II=1
        result_mem[i] = q_in.read();
    }
    q_done.write(true);
}

// Timer to measure cycle count
void timer(
    tapa::istream<bool> &q,
    tapa::mmap<int> cycle_mem
) {
    int cycle_count = 0;
    while (q.empty()) {
        #pragma HLS PIPELINE II=1
        ++cycle_count;
    }
    q.read();
    cycle_mem[0] = cycle_count;
}

// Top-level kernel
// void MergeKernel(
//     tapa::mmap<int> array_a,
//     tapa::mmap<int> array_b,
//     tapa::mmap<int> merged_result,
//     tapa::mmap<int> cycle_count,
//     const int size_a,
//     const int size_b
// ) {
//     tapa::stream<int, 2> q_a("stream_a");
//     tapa::stream<int, 2> q_b("stream_b");
//     tapa::stream<int, 2> q_merged("stream_merged");
//     tapa::stream<bool, 2> q_done("stream_done");
    
//     const int total_size = size_a + size_b;
    
//     tapa::task()
//         .invoke(read_array_a, array_a, size_a, q_a)
//         .invoke(read_array_b, array_b, size_b, q_b)
//         .invoke(merge_streams, q_a, q_b, q_merged, size_a, size_b)
//         .invoke(write_result, q_merged, merged_result, total_size, q_done)
//         .invoke(timer, q_done, cycle_count)
//         ;
// }

ComparatorResult comparator_unit(
    int a_value, 
    int b_value,        
    bool top_is_less,
    bool left_is_geq,
    int row_idx,           
    int col_idx
) {
    ComparatorResult result;

    bool is_padding = (row_idx == 4 && col_idx == 4);
    
    result.is_geq = (a_value >= b_value);
    result.group_id = row_idx + col_idx;
    result.is_boundary = false;
    
    if (is_padding) {
        result.is_boundary = false;
    } else if (row_idx == 0 && col_idx == 0) {
        result.is_boundary = true;
    } else if (row_idx == 0 && result.is_geq) {
        result.is_boundary = true;
    } else if (result.is_geq && top_is_less) {
        result.is_boundary = true;
    } else if (!result.is_geq && left_is_geq) {
        result.is_boundary = true;
    }
    
    if (result.is_geq) {
        result.output_value = b_value;
    } else {
        result.output_value = a_value;
    }
    
    return result;
}

void comparator_array_4x4_with_padding( 
    int a_values[4],
    int b_values[4],    
    int output_values[8]
) {
    const int INF_VALUE = 0x7FFFFFFF;
    
    ComparatorResult comp_results[5][5];
    #pragma HLS ARRAY_PARTITION variable=comp_results complete dim=0
    
    bool is_geq_matrix[5][5];
    #pragma HLS ARRAY_PARTITION variable=is_geq_matrix complete dim=0
    
    for (int i = 0; i < 5; i++) {
    #pragma HLS UNROLL
        for (int j = 0; j < 5; j++) {
        #pragma HLS UNROLL
            int a_val = (i < 4) ? a_values[i] : INF_VALUE;
            int b_val = (j < 4) ? b_values[j] : INF_VALUE;
            is_geq_matrix[i][j] = (a_val >= b_val);
        }
    }
    
    for (int i = 0; i < 5; i++) {
    #pragma HLS UNROLL
        for (int j = 0; j < 5; j++) {
        #pragma HLS UNROLL
            
            bool top_is_less = (i > 0) ? (!is_geq_matrix[i-1][j]) : false;
            bool left_is_geq = (j > 0) ? is_geq_matrix[i][j-1] : true;
            
            
            int a_value_val = (i < 4) ? a_values[i] : INF_VALUE;
            int b_value_val = (j < 4) ? b_values[j] : INF_VALUE;
            
            comp_results[i][j] = comparator_unit(
                a_value_val,
                b_value_val,
                top_is_less,
                left_is_geq,
                i, j
            );
        }
    }
    
    for (int group = 0; group < 8; group++) {
    #pragma HLS UNROLL
        int out_value = 0;
        
        for (int i = 0; i < 5; i++) {
        #pragma HLS UNROLL
            for (int j = 0; j < 5; j++) {
            #pragma HLS UNROLL
                
                bool is_valid_tile = !(i == 4 && j == 4);
                
                if (i + j == group && 
                    is_valid_tile && 
                    comp_results[i][j].is_boundary) {
                    out_value = comp_results[i][j].output_value;
                }
            }
        }
        
        output_values[group] = out_value;
    }
}

void read_array_a_parallel(
    tapa::mmap<hls::vector<int, 4>> array_mem,
    const int size,
    tapa::ostream<hls::vector<int, 4>> &q_out
) {
    int num_vectors = (size + 3) / 4;
    for (int i = 0; i < num_vectors; i++) {
    #pragma HLS pipeline II=1
        q_out.write(array_mem[i]);
    }
}

void read_array_b_parallel(
    tapa::mmap<hls::vector<int, 4>> array_mem,
    const int size,
    tapa::ostream<hls::vector<int, 4>> &q_out
) {
    int num_vectors = (size + 3) / 4;
    for (int i = 0; i < num_vectors; i++) {
    #pragma HLS pipeline II=1
        q_out.write(array_mem[i]);
    }
}

void merge_streams_parallel(
    tapa::istream<hls::vector<int, 4>> &q_a,
    tapa::istream<hls::vector<int, 4>> &q_b,
    tapa::ostream<hls::vector<int, 8>> &q_out,
    const int size_a,
    const int size_b
) {
    int remaining_a = size_a;
    int remaining_b = size_b;

    int a_values[4];
    #pragma HLS ARRAY_PARTITION variable=a_values complete
    int b_values[4];
    #pragma HLS ARRAY_PARTITION variable=b_values complete
    int output_values[8];
    #pragma HLS ARRAY_PARTITION variable=output_values complete
    
    for (int i = 0; i < size_a; i += 4) {
    #pragma HLS pipeline II=1
        hls::vector<int, 4> vec_a = q_a.read();
        hls::vector<int, 4> vec_b = q_b.read();
        
        for (int i = 0; i < 4; i++) {
            #pragma HLS UNROLL
            a_values[i] = vec_a[i];
            b_values[i] = vec_b[i];
        }
        
        comparator_array_4x4_with_padding(
            a_values, b_values,
            output_values
        );
        
        hls::vector<int, 8> vec_out;
        for (int i = 0; i < 8; i++) {
        #pragma HLS UNROLL
            vec_out[i] = output_values[i];
        }
        
        q_out.write(vec_out);
    }
}

void write_result_parallel(
    tapa::istream<hls::vector<int, 8>> &q_in,
    tapa::mmap<hls::vector<int, 8>> result_mem,
    const int total_size,
    tapa::ostream<bool> &q_done
) {
    int num_vectors = (total_size + 7) / 8;
    for (int i = 0; i < num_vectors; i++) {
    #pragma HLS pipeline II=1
        result_mem[i] = q_in.read();
    }
    q_done.write(true);
}


void MergeKernel(
    tapa::mmap<hls::vector<int, 4>> array_a,
    tapa::mmap<hls::vector<int, 4>> array_b,
    tapa::mmap<hls::vector<int, 8>> merged_result,
    tapa::mmap<int> cycle_count,
    const int size_a,
    const int size_b
) {
    tapa::stream<hls::vector<int, 4>, 2> q_a("stream_a");
    tapa::stream<hls::vector<int, 4>, 2> q_b("stream_b");
    tapa::stream<hls::vector<int, 8>, 2> q_merged("stream_merged");
    tapa::stream<bool, 2> q_done("stream_done");
    
    const int total_size = size_a + size_b;
    
    tapa::task()
        .invoke(read_array_a_parallel, array_a, size_a, q_a)
        .invoke(read_array_b_parallel, array_b, size_b, q_b)
        .invoke(merge_streams_parallel, q_a, q_b, q_merged, size_a, size_b)
        .invoke(write_result_parallel, q_merged, merged_result, total_size, q_done)
        .invoke(timer, q_done, cycle_count)
        ;
}