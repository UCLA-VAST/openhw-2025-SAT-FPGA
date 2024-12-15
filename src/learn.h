#ifndef LEARN_H
#define LEARN_H

#include "fpga_solver.h"
#include "hls_burst_maxi.h"
#include "ap_utils.h"
#include "manage.h"
#include "data_structures.h"
#include "backtrack.h"
#include "minimize.h"

void clause_store_prefetch(hls::stream<cls>& prefetchClsStore, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream);

void merge_resolution_sort(hls::stream<lit_resolve>& mrsp2Stream, hls::stream<lit>& fromClsStore,
    lit resolutionClause[_FPGA_MAX_LEARN_ELE], ap_uint<_FPGA_MAX_LEARN_ELE_BITS+2> mergeScratchPad[_FPGA_MAX_LITERALS], ap_uint<512> validBit[_FPGA_MAX_LITERALS/512],
    unsigned int& numElements, ap_uint<64>& learnedStats);
void merge_resolution_sort_part_2(hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput, hls::stream<lit_resolve>& mrsp2Stream, 
    const literalMetaData lmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    unsigned int& streamSize, unsigned int& highestIL, unsigned int& fixedStackCount, const int decisionLevel);
void resolution_dataflow_wrapper(hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput,
    ap_uint<_FPGA_MAX_LEARN_ELE_BITS+2> mergeScratchPad[_FPGA_MAX_LITERALS], ap_uint<512> validBit[_FPGA_MAX_LITERALS/512],
    const literalMetaData lmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    lit resolutionClause[_FPGA_MAX_LEARN_ELE],
    unsigned int& numElements, unsigned int& streamSize, unsigned int& highestIL, unsigned int& fixedStackCount,
    const int decisionLevel, ap_uint<64>& learnedStats, 
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream);

void undo_states_and_minimize_task_parallel_wrapper(hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput, 
    hls::stream<lit>& toMinimizeStream,
    clsState clsStates[_FPGA_CLS_STATES_PARTITION][_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION],
    literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS], literalMetaData lmd[_FPGA_MAX_LITERALS], 
    ap_uint<512> validBitMinimize[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS/512],
    ap_uint<2> mergeScratchPadMinimize[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    unsigned int nonRemovableCount[_FPGA_PARALLEL_MINIMIZE], bool didSimplify[_FPGA_PARALLEL_MINIMIZE], const cls unitByCls[_FPGA_MAX_LITERALS],
    const ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16],
    const lit answerStack[_FPGA_MAX_LITERALS],  
    const lit literalCommit, const unsigned int backtrackHeight, unsigned int& answerStackHeight, 
    const bool foundAbsolute, const unsigned int LITERAL_PAGE_SIZE, const ap_uint<1> POSITIVE_LIT_PHASE_VAL, const unsigned int clearIterations,
    ap_uint<64> litStoreAccessStats[4], ap_uint<64> minimizeStats[2][2], unsigned int overheadClear[_FPGA_PARALLEL_MINIMIZE],
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1, hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream2,
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream1, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream2, volatile uint64_t store[2]);

void findNextCls(hls::stream<cls>& nextClauseReg, hls::stream<bool>& stopSignal, 
    unsigned int& trailEndIndex, const ap_uint<12> mergeScratchPad[_FPGA_MAX_LITERALS], const ap_uint<512> validBit[_FPGA_MAX_LITERALS/512],
    const lit answerStack[_FPGA_MAX_LITERALS], const cls unitByCls[_FPGA_MAX_LITERALS],
    const literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS], ap_uint<64>& learnedStats);
void findNextClsCompare(hls::stream<cls>& nextClauseReg, hls::stream<bool>& stopSignal, cls& nextClauseMerge);
void findNextClsDataflow(unsigned int& trailEndIndex, cls& nextClauseMerge,
    const ap_uint<_FPGA_MAX_LEARN_ELE_BITS+2> mergeScratchPad[_FPGA_MAX_LITERALS], const ap_uint<512> validBit[_FPGA_MAX_LITERALS/512],
    const lit answerStack[_FPGA_MAX_LITERALS], const cls unitByCls[_FPGA_MAX_LITERALS],
    const literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS], ap_uint<64>& learnedStats);
void writeClauseStream(hls::stream<ap_int<96>>& toSaveClause, hls::stream<lit>& litNewPage,
    literalMetaData lmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    clsState& mNewClauseState,
    const lit resolutionClause[_FPGA_MAX_LEARN_ELE],
    const unsigned int numElements, const unsigned int givenClsID, const int levelBefore,
    const unsigned int LITERAL_PAGE_SIZE,
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream);
void saveClause(hls::stream<ap_int<96>>& toSaveClause, ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16]);
void saveClauseDataflow(hls::stream<lit>& litNewPage, ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16],
    literalMetaData lmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    clsState& mNewClauseState,
    const lit resolutionClause[_FPGA_MAX_LEARN_ELE],
    const unsigned int numElements, const unsigned int givenClsID, const int levelBefore, 
    const unsigned int LITERAL_PAGE_SIZE, hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream);

void learnClause(clsState clsStates[_FPGA_CLS_STATES_PARTITION][_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION],
    ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16],
    literalMetaData lmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    lit insertPropagate[2], mmuStream<unsigned int, _MAX_PAGES_LIT_STORE_>& freeLitPageAddresses,
    int& decisionLevel, int& givenClsID,
    const lit answerStack[_FPGA_MAX_LITERALS], const cls unitByCls[_FPGA_MAX_LITERALS], const lit literalCommit, unsigned int& answerStackHeight,
    const ap_uint<1> POSITIVE_LIT_PHASE_VAL, const bool resetAll,
    const myStream<cls,64,7>& unsatClauses, 
    const unsigned int NUM_LITERALS, const unsigned int MAX_LITERAL_ELEMENTS, 
    const unsigned int LITERAL_PAGE_SIZE,
    ap_uint<64> learnedStats[5], ap_uint<64>* litStoreAccessStats, ap_uint<64>* longestClause, int& error,
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1, hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream2,
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream1, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream2,
    hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput, hls::stream<ap_axiu<32,0,0,0>>& pqHandlerValue,
    hls::stream<ap_axiu<64,0,0,0>>& timerValueStream, hls::stream<ap_axiu<1,0,0,0>>& conditionStream, ap_uint<64>* cycleCounter);

#endif
