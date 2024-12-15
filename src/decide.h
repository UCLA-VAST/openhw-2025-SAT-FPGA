#ifndef DECIDE_H
#define DECIDE_H

#include "fpga_solver.h"

#include "decide.h"
#include "priority_queue_functions.h"
#include "data_structures.h"

void checkUndecided(hls::stream<colorAssignment>& toCommitStream,
    lit answerStack[_FPGA_MAX_LITERALS], literalMetaData lmd[_FPGA_MAX_LITERALS], 
    literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    cls unitByCls[_FPGA_MAX_LITERALS],
    unsigned int& answerStackHeight, unsigned int& duplicateCount, const lit topLiteral, 
    const unsigned int fixedDecisionStackHeight, const unsigned int decisionLevel, const ap_uint<1> POSITIVE_LIT_PHASE_VAL);

#endif 