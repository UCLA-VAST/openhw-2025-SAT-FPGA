#include <hls_stream.h>
#include <ap_axi_sdata.h>

#ifndef FPGA_HW
#include <chrono>
#include <thread>
#endif

void readStopStream(hls::stream<bool>& stopInternal, hls::stream<ap_axiu<1,0,0,0>>& stop){
    #pragma HLS inline off

    stopInternal.write(stop.read().data);
}

#ifdef FPGA_HW
void calculate(hls::stream<unsigned int>& valueInternal, hls::stream<bool>& stopInternal){
#else
void calculate(hls::stream<ap_axiu<32,0,0,0>>& value, hls::stream<ap_axiu<1,0,0,0>>& stop){
#endif
    #pragma HLS inline off
    unsigned int count = 1;
    CALCULATE: while(true){
        #pragma HLS loop_tripcount min=16 max=16

        unsigned int newCount = count;
        bool didWrite = false;
        bool getStop;

        #ifdef FPGA_HW
        if(!stopInternal.read_nb(getStop)){
        #endif

            unsigned int size, seq;
            RESTART_LAYER: for(size = 1, seq = 0; size < newCount+1; seq++, size = 2*size+1){
                #pragma HLS loop_tripcount min=16 max=16
            }
            RESTART_LAYER_2: while (size-1 != newCount){
                #pragma HLS loop_tripcount min=16 max=16
                size = (size-1)>>1;
                seq--;
                newCount = newCount % size;
            }
                
            #ifdef FPGA_HW
                valueInternal.write(1 << seq);
            #else
                ap_axiu<32,0,0,0> pktOut;
                pktOut.data = 1 << seq;
                value.write(pktOut);
            #endif
            
        #ifdef FPGA_HW
            count++;
        }else{
            if(getStop){
                valueInternal.write(0);
                break;
            }
        }
        #else
        count++;
        ap_axiu<1,0,0,0> pkt;
        stop.read_nb(pkt);
        if(pkt.data){
            break;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        #endif
    }
}

void writeValueStream(hls::stream<ap_axiu<32,0,0,0>>& value, hls::stream<unsigned int>& valueInternal){
    #pragma HLS inline off

    WRITE_OUT: while(true){
        #pragma HLS loop_tripcount min=16 max=16
        unsigned int getValue = valueInternal.read();
        ap_axiu<32,0,0,0> data;
        data.data = getValue;
        value.write(data);

        if(getValue == 0){
            break;
        }
    }
}

extern "C"{
void restartCalculator(hls::stream<ap_axiu<32,0,0,0>>& value, hls::stream<ap_axiu<1,0,0,0>>& stop){

	#pragma HLS INTERFACE axis port=value
    #pragma HLS INTERFACE axis port=stop

	#pragma HLS INTERFACE s_axilite port=return

    hls::stream<bool> stopInternal;
    hls::stream<unsigned int> valueInternal;
    #pragma HLS stream variable=valueInternal depth=16

    #pragma HLS dataflow

    #ifdef FPGA_HW

    readStopStream(stopInternal, stop);
    calculate(valueInternal, stopInternal);
    writeValueStream(value, valueInternal);

    #else
    calculate(value, stop);
    ap_axiu<32,0,0,0> pkt;
    pkt.data = 0;
    value.write(pkt);
    #endif

}
}