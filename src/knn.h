#ifndef KNN_H_
#define KNN_H_

#include <tapa.h>

void KNNKernel(
    tapa::mmap<uint32_t> train_image_0,
    tapa::mmap<uint32_t> train_image_1,
    tapa::mmap<uint32_t> train_image_2,
    tapa::mmap<uint32_t> train_image_3,
    tapa::mmap<uint32_t> train_image_4,
    tapa::mmap<uint32_t> train_image_5,
    tapa::mmap<uint32_t> train_image_6,
    tapa::mmap<uint32_t> train_image_7,
    tapa::mmap<uint32_t> train_image_8,
    tapa::mmap<uint32_t> train_image_9,
    tapa::mmap<uint32_t> test_image,
    tapa::mmap<uint32_t> predict_label,
    tapa::mmap<uint32_t> cycle_count,
    const int test_image_num,
    const int train_image_each_class_num);

#endif
