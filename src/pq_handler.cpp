#include <hls_stream.h>
#include <ap_utils.h>
#include <ap_axi_sdata.h>
#include "data_structures.h"
#include "priority_queue_functions.h"

extern "C"{
void pqHandler(int num_literals, double decay, hls::stream<ap_axiu<32,0,0,0>>& input, hls::stream<ap_axiu<32,0,0,0>>& output){

    #pragma HLS INTERFACE axis port=input
    #pragma HLS INTERFACE axis port=output

    #pragma HLS INTERFACE s_axilite port=num_literals
    #pragma HLS INTERFACE s_axilite port=decay

	#pragma HLS INTERFACE s_axilite port=return

    pqData mPriorityQueue[2][_FPGA_MAX_LITERALS];
    #pragma HLS array_partition variable=mPriorityQueue dim=1 complete
    #pragma HLS aggregate variable=mPriorityQueue compact=auto
    #pragma HLS bind_storage variable=mPriorityQueue type=RAM_S2P impl=URAM latency=1

    pqPosition mPositioning[_FPGA_MAX_LITERALS];
    #pragma HLS bind_storage variable=mPositioning type=RAM_S2P impl=URAM latency=1

    unsigned int remainingLiterals = num_literals;

    unsigned int NUM_LITERALS = num_literals;

    double multiplier = 1.0;
    double decayFactor = decay;

    loadPositioning(mPositioning, mPriorityQueue, NUM_LITERALS);

    int keepState = -1;
    while(true){
        ap_axiu<32,0,0,0> read;
        unsigned int code;

        if(keepState == -1){
            read = input.read();
            code = read.data;
        }else{
            code = keepState;
        }
        

        if(code == pq::EXIT){
            break;
        }else if(code == pq::GET_UNDECIDED){
            hls::stream<lit> removeLiterals;
            unsigned int streamSize = 0;
            #pragma HLS stream variable=removeLiterals depth=1024
            unsigned int inc = 0;
            keepState = -1;

            SCAN_LINEAR_FIND: while(true){
                #pragma HLS loop trip_count min=8 max=8

                if(code == pq::EXIT || streamSize == 1024-1){
                    if(streamSize == 1024-1 && code != pq::EXIT){
                        keepState = pq::GET_UNDECIDED;
                    }
                    break;
                }

                lit getUndecided = mPriorityQueue[0][inc].literal;

                removeLiterals.write(abs(getUndecided));
                streamSize++;
                inc++;

                ap_axiu<32,0,0,0> send;
                send.data = getUndecided;
                output.write(send);
                ap_wait();
                read = input.read();
                code = read.data;
            }

            removeLiterals.write(pq::EXIT);
            hideElement(removeLiterals, mPriorityQueue, mPositioning, remainingLiterals);
        }else if(code == pq::UPDATE){
            unhide_wrapper(input, mPriorityQueue, mPositioning, remainingLiterals, multiplier, decayFactor, NUM_LITERALS);
        }else if(code == pq::UNHIDE_ELE){
            unhide_wrapper(input, mPriorityQueue, mPositioning, remainingLiterals);
        }
    }

}
}
