#ifndef MANAGE_H
#define MANAGE_H

#include "fpga_solver.h"
#include "data_structures.h"

void allocatePage(hls::stream<lit>& litNewPage, mmuStream<unsigned int, _MAX_PAGES_LIT_STORE_>& freeLitPageAddresses,
    literalMetaData lmd[_FPGA_MAX_LITERALS], 
    ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16], int& error,
    const unsigned int LITERAL_PAGE_SIZE);

void deleteTransposedClauses(ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16],
    literalMetaData lmd[_FPGA_MAX_LITERALS],
    mmuStream<unsigned int, _MAX_PAGES_LIT_STORE_>& freeLitPageAddresses, const unsigned int LITERAL_PAGE_SIZE, 
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream1, hls::stream<ap_axiu<32,0,0,0>>& locationOutputStream);
#endif