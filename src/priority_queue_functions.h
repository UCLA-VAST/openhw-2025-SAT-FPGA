#ifndef PRIORITY_QUEUE_FUNCTIONS_H
#define PRIORITY_QUEUE_FUNCTIONS_H

#include "fpga_solver.h"
#include "data_structures.h"

inline int comp_fp64(double op1, double op2);
void loadPositioning(pqPosition mPositioning[_FPGA_MAX_LITERALS], pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], unsigned int NUM_LITERALS);
void swapLower(pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], 
    const pqData initialpqData, const unsigned int initialPosition, const unsigned int remainingLiterals);
void swapHigher(pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], 
    const pqData initialpqData, const unsigned int initialPosition);
void hideElement(hls::stream<lit>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], unsigned int& remainingLiterals);
void unhideElement(hls::stream<lit>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], unsigned int& remainingLiterals);
void decayEveryElement(hls::stream<lit>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], 
    const unsigned int remainingLiterals, double& multiplier, const double decayFactor, const unsigned int NUM_LITERALS);
void axiStreamBuffer(hls::stream<ap_axiu<32,0,0,0>>& input, hls::stream<lit>& intermediateStream);
void hide_wrapper(hls::stream<ap_axiu<32,0,0,0>>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], unsigned int& remainingLiterals);
void unhide_wrapper(hls::stream<ap_axiu<32,0,0,0>>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], unsigned int& remainingLiterals);
void unhide_wrapper(hls::stream<ap_axiu<32,0,0,0>>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], 
    const unsigned int remainingLiterals, double& multiplier, const double decayFactor, const unsigned int NUM_LITERALS);
#endif
