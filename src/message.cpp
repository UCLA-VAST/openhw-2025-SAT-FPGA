#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include "data_structures.h"

void debugWrite(debugStrct& debug, bool ENABLE_DBG){
    #pragma HLS inline off

    if(ENABLE_DBG){
        if(debug.index > 0){
            ((volatile int*)debug.ptr)[debug.index] = debug.value;
        }
        ((volatile int*)debug.ptr)[5] = ((volatile int*)debug.ptr)[4];
        ((volatile int*)debug.ptr)[4] = debug.code;
    }
}

extern "C"{
void message(int* debug, bool enableDebug, hls::stream<ap_axiu<96,0,0,0>>& messageStream){

    #pragma HLS INTERFACE m_axi port=debug offset=slave bundle=debug_gmem latency=40

	#pragma HLS INTERFACE axis port=messageStream

	#pragma HLS INTERFACE s_axilite port=debug 

	#pragma HLS INTERFACE s_axilite port=return

    const bool ENABLE_DBG = enableDebug;
    debugStrct dbg;
    dbg.ptr = debug;

    while(true){
        ap_axiu<96,0,0,0> read = messageStream.read();

        dbg.index = read.data.range(95,64);
        dbg.value = read.data.range(63,32);
        dbg.code = read.data.range(31,0);

        if(dbg.code < 0){
            break;
        }

        debugWrite(dbg, ENABLE_DBG);
    }
    ((volatile int*)debug)[0] = dbg.code;

}
}