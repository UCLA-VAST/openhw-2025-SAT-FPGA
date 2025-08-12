#ifndef DISCOVER_H
#define DISCOVER_H

#include "fpga_solver.h"
#include "decide.h"
#include "priority_queue_functions.h"
#include "color.h"
#include "data_structures.h"

void discover(hls::stream<colorAssignment>& toSplitAccessStream,  hls::stream<bcpPacket>& toDecide,
    hls::stream<int>& duplicateCountStream, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream,
    const flippedLiteral litToCheck,
    lit answerStack[_FPGA_MAX_LITERALS], literalMetaData lmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    cls unitByCls[_FPGA_MAX_LITERALS],
    unsigned int& answerStackHeight, const unsigned int decisionLevel, unsigned int& fixedDecisionStackHeight,
    const lit topLiteral, const bool useFlipped, const ap_uint<1> POSITIVE_LIT_PHASE_VAL, bool& skipCPUOnce);
void updateStatesForward(hls::stream<clsStateControlPacket>& toControlSinkMUX, 
    hls::stream<colorValue>& toStateUpdater, clsState clsStates1[_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION], clsState clsStates2[_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION], 
    const unsigned int id);
void muxControlSink(hls::stream<clsStateControlPacket>& toControlSink, 
    hls::stream<clsStateControlPacket> toControlSinkMUX[_FPGA_CLS_STATES_PARTITION]);
void controlSink(hls::stream<bcpPacket>& toOverflow,
    hls::stream<clsStateControlPacket>& toControlSink,
    hls::stream<int>& duplicateCountStream,
    hls::stream<bool>& stopSending, hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1,
    myStream<cls,64,7>& unsatClause, lit& literalCommit, bool& doBackTrack,
    const unsigned int decisionLevel, const unsigned int fixedDecisionStackHeight, const bool firstIteration,
    bool& flushSignal);
void axiStreamBufferFWD(hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput, hls::stream<lit>& intermediateStream);
void bcp_discover_dataflow_wrapper(clsState clsStates[_FPGA_CLS_STATES_PARTITION][_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION],
    lit answerStack[_FPGA_MAX_LITERALS], literalMetaData lmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    cls unitByCls[_FPGA_MAX_LITERALS],
    const ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16],
    unsigned int& answerStackHeight,
    myStream<cls,64,7>& unsatClause, unsigned int& fixedDecisionStackHeight, lit& literalCommit, bool& doBackTrack,
    const lit topLiteral, const flippedLiteral litToCheck, const unsigned int fixedDecisionStackHeightCopy,
    const unsigned int decisionLevel, const bool useFlipped, const bool firstIteration, const unsigned int LITERAL_PAGE_SIZE, const ap_uint<1> POSITIVE_LIT_PHASE_VAL,
    ap_uint<64> litStoreAccessStats[4], volatile uint64_t store[2], hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput, 
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream);

#endif