INC_XCL := 
#-I /opt/xilinx/xrt/include/
GXX_FLAGS := -w -O2 -std=c++17
LIB := -ltapa -lfrt -lglog -lgflags -lOpenCL
SRC := ./src
Platform := xilinx_u55c_gen3x16_xdma_3_202210_1

.DEFAULT_GOAL := merge

merge.o: $(SRC)/merge.cpp
	tapa g++ -- $(GXX_FLAGS) -c $^ $(INC_XCL)

main.o: $(SRC)/main.cpp
	tapa g++ -- $(GXX_FLAGS) -c $^ $(INC_XCL)

merge: merge.o main.o
	tapa g++ -- $(GXX_FLAGS) -o $@ $^ $(INC_XCL) $(LIB)

swsim: merge
	./merge --skip_kernel=false --size_a=1000 --size_b=1000

hls: $(SRC)/merge.cpp
	tapa compile --top MergeKernel \
	--platform xilinx_u55c_gen3x16_xdma_3_202210_1 \
	--clock-period 4.4 \
	-f $^ \
	-o merge.xo

hwemu: merge.xo
	./merge --skip_kernel=false --bitstream=./merge.xo --size_a=1000 --size_b=1000

hw: $(SRC)/merge.cpp
	tapa compile --top MergeKernel \
	--platform xilinx_u55c_gen3x16_xdma_3_202210_1 \
	--clock-period 4.4 \
	--connectivity link_config.cfg \
	-f $^ \
	-o merge.xclbin

bitstream: hw

run_hw: merge
	./merge --skip_kernel=false --bitstream=./merge.xclbin --size_a=10000 --size_b=10000

clean:
	rm -f *.o merge

cleanall:
	rm -f *.o merge merge.xo merge.xclbin
	rm -rf work.out _x .Xil

.PHONY: swsim hls hwemu hw bitstream run_hw clean cleanall


# --------------------------------------------------------------------
# Additional hardware test cases (same style as hwemu / run_hw)
# --------------------------------------------------------------------

# Strongly unbalanced: A large, B small
run_hw_unbalanced_largeA: merge
	./merge --skip_kernel=false --bitstream=./merge.xo \
		--size_a=1800 --size_b=200

# Balanced: A and B equal and moderately large
run_hw_balanced: merge
	./merge --skip_kernel=false --bitstream=./merge.xo \
		--size_a=1000 --size_b=1000

# Very long arrays: stress test (adjust down if this is too big)
run_hw_long: merge
	./merge --skip_kernel=false --bitstream=./merge.xo \
		--size_a=4000 --size_b=4000

# Convenience target to run all HW tests (requires merge.xclbin prebuilt via `make bitstream`)
tests_hw: run_hw_unbalanced_largeA run_hw_unbalanced_largeB run_hw_balanced run_hw_long

.PHONY: run_hw_unbalanced_largeA run_hw_unbalanced_largeB run_hw_balanced run_hw_long tests_hw
