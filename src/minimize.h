#ifndef MINIMIZE_H
#define MINIMIZE_H

#include "fpga_solver.h"
#include "hls_burst_maxi.h"
#include "ap_utils.h"
#include "manage.h"
#include "data_structures.h"
#include "backtrack.h"
#include "learn.h"

#include "minimize.h"

void minimize_dispatch(hls::stream<lit>& toMinimizeStream,
    hls::stream<lit> splitMinimizeStream[2], bool foundAbsolute);

void minimize_resolution_sort(hls::stream<lit_resolve>& mrsp2Stream, hls::stream<lit>& fromClsStore, 
    ap_uint<2> mergeScratchPad[_FPGA_MAX_LITERALS], ap_uint<512> validBit[_FPGA_MAX_LITERALS/512],
    unsigned int& numElements, ap_uint<64>& learnedStats);

void minimize_resolution_sort_part_2(myStream<lit,_FPGA_MAX_LEARN_ELE,_FPGA_MAX_LEARN_ELE_BITS>& nextLiteralMinimize, hls::stream<lit_resolve>& mrsp2Stream, 
    const literalMinimizeMetaData lmmd[_FPGA_MAX_LITERALS], unsigned int& countMarked, int& exitCondition);

void minimize_dataflow_wrapper_layer_2(myStream<lit,_FPGA_MAX_LEARN_ELE,_FPGA_MAX_LEARN_ELE_BITS>& nextLiteralMinimize,
    ap_uint<2> mergeScratchPad[_FPGA_MAX_LITERALS], ap_uint<512> validBit[_FPGA_MAX_LITERALS/512],
    const literalMinimizeMetaData lmmd[_FPGA_MAX_LITERALS], unsigned int& numElements, 
    unsigned int& countMarked, int& exitCondition, ap_uint<64>& learnedStats,
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream);

void minimize_dataflow_wrapper_layer_1(hls::stream<lit>& toSplitStream,
    //const lit resolutionClause[1024], const unsigned int numElements,
    literalMinimizeMetaData lmmd[_FPGA_MAX_LITERALS], 
    ap_uint<512> validBitMinimize[_FPGA_MAX_LITERALS/512],
    ap_uint<2> mergeScratchPadMinimize[_FPGA_MAX_LITERALS],
    unsigned int& nonRemovableCount, bool& didSimplify,
    const cls unitByCls[_FPGA_MAX_LITERALS], const bool foundAbsolute,
    const unsigned int clearIterations, ap_uint<64> minimizeStats[2], unsigned int& overheadClear,  
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream);
#endif