#ifndef COPY_IN_H
#define COPY_IN_H

#include "fpga_solver.h"
#include "data_structures.h"

void copy_clsStates(clsState mClsStates[_FPGA_CLS_STATES_PARTITION][_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION], clsStatePCIE* clsStates, const unsigned int numClauses);
void copy_clsStore(ap_uint<512> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/16], ap_uint<512>* clauseStore, const unsigned int clauseElements);
void copy_lmd(literalMetaData mlmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData mlmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS], literalMetaDataPCIE* lmd, const unsigned int NUM_LITERALS);
void copy_answerStack(lit mAnswerStack[_FPGA_MAX_LITERALS], lit* answerStack, const unsigned int NUM_LITERALS);
void copy_litStore(ap_uint<512> mLitStore[(_FPGA_MAX_LITERAL_ELEMENTS/16)/2], ap_int<512>* litStore, const unsigned int literalElements);

void copy_in_dataflow_wrapper(clsState mClsStates[_FPGA_CLS_STATES_PARTITION][_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION], 
    ap_uint<512> mLitStore[_FPGA_MAX_LITERAL_ELEMENTS/16],
    literalMetaData mlmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData mlmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS], 
    lit mAnswerStack[_FPGA_MAX_LITERALS], 
    clsStatePCIE* clsStates, ap_int<512>* litStore1, literalMetaDataPCIE* lmd, lit* answerStack,
    const unsigned int literalElements, const unsigned int numClauses, const unsigned int NUM_LITERALS, volatile uint64_t store[2]);
#endif