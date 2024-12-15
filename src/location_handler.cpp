#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_utils.h>
#include "data_structures.h"


extern "C"{
void location_handler(
    hls::stream<ap_axiu<64,0,0,0>>& locationInputStream, hls::stream<ap_axiu<32,0,0,0>>& locationOutputStream){

	#pragma HLS INTERFACE axis port=locationInputStream
    #pragma HLS INTERFACE axis port=locationOutputStream
	#pragma HLS INTERFACE s_axilite port=return

    ap_uint<128> mClsToLitStorePos[_FPGA_MAX_LITERAL_ELEMENTS/4];
    #pragma HLS bind_storage variable=mClsToLitStorePos type=RAM_S2P impl=URAM latency=2

    ap_uint<128> mLitToClsStorePos[_FPGA_MAX_LITERAL_ELEMENTS/4];
    #pragma HLS bind_storage variable=mLitToClsStorePos type=RAM_S2P impl=URAM latency=2

    LOCATION_HANDLE_LOOP: while(true){
        #pragma HLS loop_tripcount min=16 max=16
        ap_axiu<64,0,0,0> getCommand = locationInputStream.read();
        unsigned int code = getCommand.data.range(31,0);

        if(code == lh::EXIT){
            break;
        }else if(code == lh::SEND){
            SEND_LIT_STORE_LOCATION: while(true){
                #pragma HLS loop_tripcount min=16 max=16
                ap_axiu<64,0,0,0> value = locationInputStream.read();

                if(value.data == lh::EXIT){
                    break;
                }
                unsigned int addr = value.data.range(31,0);
                ap_uint<128> getAddr = mClsToLitStorePos[addr/4];    
                ap_axiu<32,0,0,0> send;
                send.data = getAddr.range(32*(addr%4)+31,32*(addr%4));
                locationOutputStream.write(send);
            }
        }else if(code == lh::SAVE){
            ap_uint<128> getAddr = 0;
            SET_LIT_AND_CLS_STORE_LOCATION: while(true){
                #pragma HLS loop_tripcount min=16 max=16
                #pragma HLS depence variable=mLitToClsStorePos inter false
                ap_axiu<64,0,0,0> value = locationInputStream.read();

                if(value.data == lh::EXIT){
                    break;
                }

                unsigned int clsToLitAddr = value.data.range(31,0);    
                if(clsToLitAddr%4 == 0){
                    getAddr = 0;
                }
                getAddr.range(32*(clsToLitAddr%4)+31,32*(clsToLitAddr%4)) = value.data.range(63,32);
                mClsToLitStorePos[clsToLitAddr/4] = getAddr;

                unsigned int litToClsAddr = value.data.range(63,32);
                ap_uint<128> getAddr2 = mLitToClsStorePos[litToClsAddr/4];    
                getAddr2.range(32*(litToClsAddr%4)+31,32*(litToClsAddr%4)) = value.data.range(31,0);

                mLitToClsStorePos[litToClsAddr/4] = getAddr2;
            }
        }else if(code == lh::UPDATE){
            UPDATE_LIT_AND_CLS_STORE_LOCATION: while(true){
                #pragma HLS loop_tripcount min=16 max=16
                #pragma HLS dependence variable=mLitToClsStorePos inter false

                ap_axiu<64,0,0,0> value = locationInputStream.read();

                if(value.data == lh::EXIT){
                    break;
                }

                unsigned int swapAddr = value.data.range(31,0);
                unsigned int replaceAddr = value.data.range(63,32);

                ap_uint<128> getSwap = mLitToClsStorePos[swapAddr/4];
                ap_uint<128> getReplace = mLitToClsStorePos[replaceAddr/4];

                unsigned int litToClsAddr = getSwap.range(32*(swapAddr%4)+31,32*(swapAddr%4));
                getReplace.range(32*(replaceAddr%4)+31,32*(replaceAddr%4)) = litToClsAddr;
                mLitToClsStorePos[replaceAddr/4] = getReplace;

                ap_uint<128> getClsToLitAddrs = mClsToLitStorePos[litToClsAddr/4];
                getClsToLitAddrs.range(32*(litToClsAddr%4)+31,32*(litToClsAddr%4)) = replaceAddr;
                mClsToLitStorePos[litToClsAddr/4] = getClsToLitAddrs;
            }
        }
    }

}
}