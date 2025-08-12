#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <cstdint>

void wrapTimer(hls::stream<ap_uint<1>>& wrapConditionStream, hls::stream<ap_axiu<64,0,0,0>>& value){
    #pragma HLS inline off

    volatile uint64_t counter = 0;
    TIMER: while(true){
        #pragma HLS loop_tripcount min=1000 max=1000

        ap_uint<1> read;
        if(wrapConditionStream.read_nb(read)){
            if(read == 0){
                break;
            }else{
                ap_axiu<64,0,0,0> send;
                send.data = counter;
                value.write(send);
            }
        }
        counter++;
    }
}

void wrapCondition(hls::stream<ap_uint<1>>& wrapConditionStream, hls::stream<ap_axiu<1,0,0,0>>& condition){
    #pragma HLS inline off

    WRAP: while(true){
        #pragma HLS loop_tripcount min=10 max=10
        ap_axiu<1,0,0,0> read = condition.read();
        wrapConditionStream.write(read.data);
        if(read.data == 0){
            break;
        }
    }
}

void timerDataflow(hls::stream<ap_axiu<1,0,0,0>>& condition, hls::stream<ap_axiu<64,0,0,0>>& value){
    #pragma HLS inline off

    hls::stream<ap_uint<1>> wrapConditionStream;

    #pragma HLS dataflow
    wrapCondition(wrapConditionStream, condition);
    wrapTimer(wrapConditionStream, value);
}

extern "C"{
void timer(hls::stream<ap_axiu<1,0,0,0>>& condition, hls::stream<ap_axiu<64,0,0,0>>& value){

	#pragma HLS INTERFACE axis port=value
    #pragma HLS INTERFACE axis port=condition

	#pragma HLS INTERFACE s_axilite port=return

    #ifndef FPGA_HW
    volatile uint64_t counter = 0;
    while(true){
        ap_axiu<1,0,0,0> read;
        if(condition.read_nb(read)){
            if(read.data == 0){
                break;
            }else{
                ap_axiu<64,0,0,0> send;
                send.data = counter;
                value.write(send);
            }
        }
        counter++;
    }
    #else
    timerDataflow(condition, value);
    #endif

}
}