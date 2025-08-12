#ifndef COLOR_H
#define COLOR_H

#include "fpga_solver.h"
#include "hls_burst_maxi.h"
#include "ap_utils.h"
#include "data_structures.h"

void colorStream(hls::stream<colorValue>* toStateUpdater, 
    hls::stream<colorAssignment>& toColorStream, hls::stream<bool>* stopSending,
    const ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16], const unsigned int LITERAL_PAGE_SIZE,
    lit* literalCommit, const unsigned int type, ap_uint<64>* litStoreAccessStats);

#endif