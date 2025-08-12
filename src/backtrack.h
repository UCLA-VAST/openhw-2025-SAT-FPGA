#ifndef BACKTRACK_H
#define BACKTRACK_H

#include "fpga_solver.h"
#include "color.h"
#include "data_structures.h"

void undoStates(hls::stream<lit>& intermediateStream, 
    hls::stream<colorAssignment>& toCommitStream,
    const lit answerStack[_FPGA_MAX_LITERALS], literalMetaData lmd[_FPGA_MAX_LITERALS],
    const lit literalCommit, unsigned int& answerStackHeight, 
    const unsigned int backtrackHeight, hls::stream<bool>& finishSignal);
void updateStatesBackward(hls::stream<colorValue>& toStateUpdater, 
    clsState clsStates[_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION], const unsigned int id);
void axiStreamBufferBT(hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput, hls::stream<lit>& intermediateStream);
void undo_states_dataflow_wrapper(hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput, 
    clsState clsStates[_FPGA_CLS_STATES_PARTITION][_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION],
    const lit answerStack[_FPGA_MAX_LITERALS], literalMetaData lmd[_FPGA_MAX_LITERALS],
    const ap_uint<512> literalStore[_FPGA_MAX_LITERAL_ELEMENTS/16],
    const lit literalCommit, const unsigned int backtrackHeight, unsigned int& answerStackHeight,
    const unsigned int LITERAL_PAGE_SIZE, const ap_uint<1> POSITIVE_LIT_PHASE_VAL, ap_uint<64> litStoreAccessStats[4]);
#endif