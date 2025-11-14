#include <chrono>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <ctime>

#include "merge.h"

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using std::clog;
using std::endl;
using std::string;

template <typename T>
using aligned_vector = std::vector<T, tapa::aligned_allocator<T>>;

DEFINE_string(bitstream, "", "path to the bitstream file, run csim if empty");
DEFINE_int32(size_a, 1000, "size of first sorted array");
DEFINE_int32(size_b, 1000, "size of second sorted array");
DEFINE_bool(skip_kernel, false, "skip kernel execution, only host CPU if true");

// Host-side merge function for verification
void merge_host(
    const aligned_vector<int> &array_a,
    const aligned_vector<int> &array_b,
    aligned_vector<int> &merged_result
) {
    int size_a = array_a.size();
    int size_b = array_b.size();
    merged_result.resize(size_a + size_b);
    
    int idx_a = 0, idx_b = 0, idx_out = 0;
    
    while (idx_a < size_a && idx_b < size_b) {
        if (array_a[idx_a] <= array_b[idx_b]) {
            merged_result[idx_out++] = array_a[idx_a++];
        } else {
            merged_result[idx_out++] = array_b[idx_b++];
        }
    }
    
    // Copy remaining elements from array_a
    while (idx_a < size_a) {
        merged_result[idx_out++] = array_a[idx_a++];
    }
    
    // Copy remaining elements from array_b
    while (idx_b < size_b) {
        merged_result[idx_out++] = array_b[idx_b++];
    }
}

// Verify if result is correctly sorted
bool verify_sorted(const aligned_vector<int> &array) {
    for (size_t i = 1; i < array.size(); i++) {
        if (array[i] < array[i-1]) {
            clog << "Error: Result not sorted at index " << i 
                 << " (value " << array[i] << " < " << array[i-1] << ")" << endl;
            return false;
        }
    }
    return true;
}

// Compare two results
bool verify_results(
    const aligned_vector<int> &result_host,
    const aligned_vector<int> &result_kernel
) {
    if (result_host.size() != result_kernel.size()) {
        clog << "Error: Size mismatch - host: " << result_host.size() 
             << ", kernel: " << result_kernel.size() << endl;
        return false;
    }
    
    for (size_t i = 0; i < result_host.size(); i++) {
        if (result_host[i] != result_kernel[i]) {
            clog << "Error: Mismatch at index " << i 
                 << " - host: " << result_host[i] 
                 << ", kernel: " << result_kernel[i] << endl;
            return false;
        }
    }
    
    clog << "Results match perfectly!" << endl;
    return true;
}

bool end_with(const std::string &value, const std::string &ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

int main(int argc, char** argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, /*remove_flags=*/true);
    
    // Initialize random seed
    srand(time(NULL));
    
    // Generate two sorted arrays
    aligned_vector<int> array_a(FLAGS_size_a);
    aligned_vector<int> array_b(FLAGS_size_b);
    
    clog << "Generating sorted array A with " << FLAGS_size_a << " elements..." << endl;
    for (int i = 0; i < FLAGS_size_a; i++) {
        array_a[i] = rand() % 10000;
    }
    std::sort(array_a.begin(), array_a.end());
    
    clog << "Generating sorted array B with " << FLAGS_size_b << " elements..." << endl;
    for (int i = 0; i < FLAGS_size_b; i++) {
        array_b[i] = rand() % 10000;
    }
    std::sort(array_b.begin(), array_b.end());
    
    // Print first few elements for verification
    clog << "Array A first 5 elements: ";
    for (int i = 0; i < std::min(5, FLAGS_size_a); i++) {
        clog << array_a[i] << " ";
    }
    clog << endl;
    
    clog << "Array B first 5 elements: ";
    for (int i = 0; i < std::min(5, FLAGS_size_b); i++) {
        clog << array_b[i] << " ";
    }
    clog << endl;
    
    // Host merge for verification

    array_a = {1, 2, 3, 4};
    array_b = {2, 3, 4, 5};
    aligned_vector<int> merged_host;
    steady_clock::time_point t1 = steady_clock::now();
    merge_host(array_a, array_b, merged_host);
    steady_clock::time_point t2 = steady_clock::now();
    double time_host = duration_cast<milliseconds>(t2 - t1).count();
    
    clog << "Host CPU merge time: " << time_host << " milliseconds" << endl;
    clog << "Host merged array size: " << merged_host.size() << endl;
    
    // Verify host result is sorted
    if (verify_sorted(merged_host)) {
        clog << "Host result is correctly sorted!" << endl;
    } else {
        clog << "Host result verification FAILED!" << endl;
        return EXIT_FAILURE;
    }
    
    clog << "Host merged first 10 elements: ";
    for (int i = 0; i < std::min(10, (int)merged_host.size()); i++) {
        clog << merged_host[i] << " ";
    }
    clog << endl;
    
    if (FLAGS_skip_kernel) {
        clog << "Skipping kernel execution as requested." << endl;
        return EXIT_SUCCESS;
    }
    
    // Kernel execution
    if (FLAGS_bitstream.empty()) {
        clog << "Running merge kernel in csim mode" << endl;
    } else if (end_with(FLAGS_bitstream, ".xo")) {
        clog << "Running merge kernel in TAPA fast cosim using file: " << FLAGS_bitstream << endl;
    } else if (end_with(FLAGS_bitstream, ".hw_emu.xclbin")) {
        clog << "Running merge kernel in Vitis hardware emulation using file: " << FLAGS_bitstream << endl;
    } else if (end_with(FLAGS_bitstream, ".xclbin")) {
        clog << "Running merge kernel on FPGA card using bitstream file: " << FLAGS_bitstream << endl;
    } else {
        throw std::runtime_error("Unsupported bitstream file: " + FLAGS_bitstream);
    }
    
    // aligned_vector<int> merged_kernel(FLAGS_size_a + FLAGS_size_b);
    // aligned_vector<int> cycle_count(1);
    
    // double time_kernel = tapa::invoke(
    //     MergeKernel,
    //     FLAGS_bitstream,
    //     tapa::read_only_mmap<int>(array_a),
    //     tapa::read_only_mmap<int>(array_b),
    //     tapa::write_only_mmap<int>(merged_kernel),
    //     tapa::write_only_mmap<int>(cycle_count),
    //     FLAGS_size_a,
    //     FLAGS_size_b
    // );

    // Pack array_a into vec4 format
    int num_vec4_a = (FLAGS_size_a + 3) / 4;
    aligned_vector<hls::vector<int, 4>> array_a_vec4(num_vec4_a);
    for (int i = 0; i < num_vec4_a; i++) {
        for (int j = 0; j < 4; j++) {
            int idx = i * 4 + j;
            array_a_vec4[i][j] = (idx < FLAGS_size_a) ? array_a[idx] : 0x7FFFFFFF;
        }
    }

    // Pack array_b into vec4 format
    int num_vec4_b = (FLAGS_size_b + 3) / 4;
    aligned_vector<hls::vector<int, 4>> array_b_vec4(num_vec4_b);
    for (int i = 0; i < num_vec4_b; i++) {
        for (int j = 0; j < 4; j++) {
            int idx = i * 4 + j;
            array_b_vec4[i][j] = (idx < FLAGS_size_b) ? array_b[idx] : 0x7FFFFFFF;
        }
    }

    // Prepare output buffer (vec8 format)
    int total_size = FLAGS_size_a + FLAGS_size_b;
    int num_vec8_out = (total_size + 7) / 8;
    aligned_vector<hls::vector<int, 8>> merged_kernel_vec8(num_vec8_out);
    aligned_vector<int> cycle_count(1);

    double time_kernel = tapa::invoke(
        MergeKernel,
        FLAGS_bitstream,
        tapa::read_only_mmap<hls::vector<int, 4>>(array_a_vec4),
        tapa::read_only_mmap<hls::vector<int, 4>>(array_b_vec4),
        tapa::write_only_mmap<hls::vector<int, 8>>(merged_kernel_vec8),
        tapa::write_only_mmap<int>(cycle_count),
        FLAGS_size_a,
        FLAGS_size_b
    );

    // Unpack result from vec8 format
    aligned_vector<int> merged_kernel;
    for (int i = 0; i < num_vec8_out; i++) {
        for (int j = 0; j < 8; j++) {
            int val = merged_kernel_vec8[i][j];
            if (val != 0x7FFFFFFF && (int)merged_kernel.size() < total_size) {
                merged_kernel.push_back(val);
            }
        }
    }
    
    clog << "Merge kernel execution time: " << time_kernel * 1e-6 << " milliseconds" << endl;
    clog << "Merge kernel cycle count: " << cycle_count[0] << endl;
    clog << "Kernel merged array size: " << merged_kernel.size() << endl;
    clog << "Kernel merged first 10 elements: ";
    for (int i = 0; i < std::min(10, (int)merged_kernel.size()); i++) {
        clog << merged_kernel[i] << " ";
    }
    clog << endl;
    
    // Verify kernel result is sorted
    if (!verify_sorted(merged_kernel)) {
        clog << "Kernel result verification FAILED - not sorted!" << endl;
        return EXIT_FAILURE;
    }
    
    // Compare host and kernel results
    if (verify_results(merged_host, merged_kernel)) {
        clog << "Merge kernel test PASSED!" << endl;
        clog << "Speedup: " << time_host / (time_kernel * 1e-6) << "x" << endl;
    } else {
        clog << "Merge kernel test FAILED - results mismatch!" << endl;
        return EXIT_FAILURE;
    }
    
    return EXIT_SUCCESS;
}