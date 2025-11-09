#include <cmath>
#include <tapa.h>
#include "knn.h"

//using std::cout;
//using std::endl;

const int kbytes_img = 3072;
// each image has 3072 bytes (32*32*3)

void read_image(
  tapa::mmap<uint32_t> img_mem,
  const int img_num,
  tapa::ostream<uint32_t> &q_out,
  const int rp_time
) {
  for (int rp = 0; rp < rp_time; rp++) {
    for (int i = 0; i < img_num * kbytes_img / 4; i++) {
    #pragma HLS PIPELINE II=1
      q_out.write(img_mem[i]);
    }
  }
}

void knn(
  tapa::istream<uint32_t> &q_in_0,
  tapa::istream<uint32_t> &q_in_1,
  tapa::istream<uint32_t> &q_in_2,
  tapa::istream<uint32_t> &q_in_3,
  tapa::istream<uint32_t> &q_in_4,
  tapa::istream<uint32_t> &q_in_5,
  tapa::istream<uint32_t> &q_in_6,
  tapa::istream<uint32_t> &q_in_7,
  tapa::istream<uint32_t> &q_in_8,
  tapa::istream<uint32_t> &q_in_9,
  tapa::istream<uint32_t> &q_test,
  tapa::ostream<uint32_t> &q_prediction,
  const int test_image_num,
  const int train_image_each_class_num
){
  for (int t = 0; t < test_image_num; t++) {
    //cout << "t = " << t << endl;
    int best_label = 0;
    int best_dist = 0x7FFFFFFF;
    uint32_t test_img[kbytes_img / 4];
    for (int i = 0; i < kbytes_img / 4; i++) {
    #pragma HLS PIPELINE II=1
      test_img[i] = q_test.read();
    }
    for (int tr = 0; tr < train_image_each_class_num; tr++) {
      for (int c = 0; c < 10; ++c) {
        int dist = 0;
        for (int i = 0; i < kbytes_img / 4; ++i) {
        #pragma HLS PIPELINE II=1
          uint32_t train_image_4byte = (c == 0 ? q_in_0.read() :
                                        c == 1 ? q_in_1.read() :
                                        c == 2 ? q_in_2.read() :
                                        c == 3 ? q_in_3.read() :
                                        c == 4 ? q_in_4.read() :
                                        c == 5 ? q_in_5.read() :
                                        c == 6 ? q_in_6.read() :
                                        c == 7 ? q_in_7.read() :
                                        c == 8 ? q_in_8.read() :
                                                 q_in_9.read());
          for (int p = 0; p < 4; p++) {
            int d = (int)((train_image_4byte >> (p * 8)) & 0xFF)
                  - (int)((test_img[i] >> (p * 8)) & 0xFF);
            dist += d * d;
          }
        }
        if (dist < best_dist) {
          best_dist = dist;
          best_label = c;
        }
      }
    }
    q_prediction.write(best_label);
  }
}

void write_label(
  tapa::istream<uint32_t> &q_in,
  tapa::mmap<uint32_t> label_mem,
  const int img_num,
  tapa::ostream<bool> &q_done
) {
  for (int i = 0; i < img_num; i++) {
  #pragma HLS PIPELINE II=1
    label_mem[i] = q_in.read();
  }
  q_done.write(true);
}

void timer(tapa::istream<bool> &q,
           tapa::mmap<uint32_t> mem) {
  uint32_t cycle_count = 0;
  while (q.empty()) {
  #pragma HLS PIPELINE II=1
    ++cycle_count;
  }
  q.read();
  mem[0] = cycle_count;
}

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
  const int train_image_each_class_num
) {
  tapa::stream<uint32_t, 2> q_tr_img_0("q_trin_image_0");
  tapa::stream<uint32_t, 2> q_tr_img_1("q_trin_image_1");
  tapa::stream<uint32_t, 2> q_tr_img_2("q_trin_image_2");
  tapa::stream<uint32_t, 2> q_tr_img_3("q_trin_image_3");
  tapa::stream<uint32_t, 2> q_tr_img_4("q_trin_image_4");
  tapa::stream<uint32_t, 2> q_tr_img_5("q_trin_image_5");
  tapa::stream<uint32_t, 2> q_tr_img_6("q_trin_image_6");
  tapa::stream<uint32_t, 2> q_tr_img_7("q_trin_image_7");
  tapa::stream<uint32_t, 2> q_tr_img_8("q_trin_image_8");
  tapa::stream<uint32_t, 2> q_tr_img_9("q_trin_image_9");

  tapa::stream<uint32_t, 2> q_t_img("q_test_image");
  tapa::stream<uint32_t, 2> q_label("predict_label");

  tapa::stream<bool, 2> q_done("q_done");

  tapa::task()
    .invoke(read_image, train_image_0, train_image_each_class_num, q_tr_img_0, test_image_num)
    .invoke(read_image, train_image_1, train_image_each_class_num, q_tr_img_1, test_image_num)
    .invoke(read_image, train_image_2, train_image_each_class_num, q_tr_img_2, test_image_num)
    .invoke(read_image, train_image_3, train_image_each_class_num, q_tr_img_3, test_image_num)
    .invoke(read_image, train_image_4, train_image_each_class_num, q_tr_img_4, test_image_num)
    .invoke(read_image, train_image_5, train_image_each_class_num, q_tr_img_5, test_image_num)
    .invoke(read_image, train_image_6, train_image_each_class_num, q_tr_img_6, test_image_num)
    .invoke(read_image, train_image_7, train_image_each_class_num, q_tr_img_7, test_image_num)
    .invoke(read_image, train_image_8, train_image_each_class_num, q_tr_img_8, test_image_num)
    .invoke(read_image, train_image_9, train_image_each_class_num, q_tr_img_9, test_image_num)
    .invoke(read_image, test_image, test_image_num, q_t_img, 1)
    .invoke(knn, q_tr_img_0, q_tr_img_1, q_tr_img_2, q_tr_img_3, q_tr_img_4,
                 q_tr_img_5, q_tr_img_6, q_tr_img_7, q_tr_img_8, q_tr_img_9,
                 q_t_img, q_label,
                 test_image_num, train_image_each_class_num)
    .invoke(write_label, q_label, predict_label, test_image_num, q_done)
    .invoke(timer, q_done, cycle_count)
    ;
}
