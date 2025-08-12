#include "color.h"

int splitResidualCnt = 0;
unsigned int checkCnt = 0;
void colorStream(hls::stream<colorValue>* toStateUpdater, 
    hls::stream<colorAssignment>& toColorStream, hls::stream<bool>* stopSending,
    const ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16], const unsigned int LITERAL_PAGE_SIZE,
    lit* literalCommit, const unsigned int type, ap_uint<64>* litStoreAccessStats){
    #pragma HLS inline off

    int state = 0;
    colorAssignment readOne;
    unsigned int numElementsRead = 0;
    unsigned int pageWalkIndex = 0;
    bool once = false;

    ap_int<512> val = 0;

    readOne.eos = false;
    readOne.literal = 0;
    readOne.addressStart = 0;

    unsigned int address = 0;
    unsigned int tmpAddr = 0;

    ap_uint<64> localAccessStats[4] = {0,0,0,0};
    #pragma HLS array_partition variable=localAccessStats complete

    bool stopSendingRead = false;
    bool didRead = false;

    const unsigned int READ_CHUNK_SIZE=_FPGA_CLS_STATES_PARTITION;

    COLOR_STREAM: while(true){
        #pragma HLS loop_tripcount min=1024 max=1024
        #pragma HLS pipeline II=1

        if(type == 0){
            if(stopSending->read_nb(stopSendingRead)){
                didRead = true;
            }
        }

        val = litStore[address/16];

        if(state == 0){
            if(readOne.eos || stopSendingRead){
                break;
            }else{
                if(toColorStream.read_nb(readOne)){
                    if(!readOne.eos){
                        state = 0;
                        if(readOne.numElements != 0){
                            state = 1;
                        }
                    
                        if(type == 0){
			    checkCnt++;
                            *literalCommit = readOne.literal;
                        }

                        once = false;
                        address = readOne.addressStart;
                    }
                }
            }
        }else if(state == 1){
            colorValue put;
            put.clsID = val.range(32*(pageWalkIndex%16)+32*READ_CHUNK_SIZE-1,32*(pageWalkIndex%16));
            put.litID = readOne.literal;
            put.depthCount = readOne.depthCount;

            put.clsEos = false;
            put.didSolve = false;
            put.streamEos = false;

            address += READ_CHUNK_SIZE;
            pageWalkIndex += READ_CHUNK_SIZE;
            numElementsRead += READ_CHUNK_SIZE;
            if(pageWalkIndex + (16-READ_CHUNK_SIZE) == LITERAL_PAGE_SIZE){
                tmpAddr = reg((unsigned int)val.range(511,480));
            }else if(pageWalkIndex == LITERAL_PAGE_SIZE){
                pageWalkIndex = 0;
                put.clsID.range(32*READ_CHUNK_SIZE-1,32*(READ_CHUNK_SIZE-2)) = 0;

                address = tmpAddr;
                numElementsRead -= 2;
            }
            if(numElementsRead >= readOne.numElements){
                state = 0;
                numElementsRead = 0;
                pageWalkIndex = 0;

                put.clsEos = true;
            }

            if(type == 0){
                for(unsigned int i = 0; i < _FPGA_CLS_STATES_PARTITION/2; i++){
                    toStateUpdater[i].write(put);
                }
            }else{
                for(unsigned int i = 0; i < _FPGA_CLS_STATES_PARTITION; i++){
                    toStateUpdater[i].write(put);
                }
            }

            if(type == 0){
                localAccessStats[2]++;
                if(pageWalkIndex == 0 && !once){
                    localAccessStats[0]++;
                    once = true;
                }
            }else if(type == 1){
                localAccessStats[3]++;
                if(pageWalkIndex == 0 && !once){
                    localAccessStats[1]++;
                    once = true;
                }
            }
        }
    }

    if(type == 0){
        litStoreAccessStats[2] += localAccessStats[2];
        litStoreAccessStats[0] += localAccessStats[0];    
    }else if(type == 1){
        litStoreAccessStats[3] += localAccessStats[3];
        litStoreAccessStats[1] += localAccessStats[1];           
    }

    if(type == 0){
        SEND_END_COLOR_FWD: for(unsigned int i = 0; i < _FPGA_CLS_STATES_PARTITION/2; i++){
            toStateUpdater[i].write((colorValue){.clsID=0,.litID=0,.depthCount=0,.didSolve=false,.clsEos=false,.streamEos=true});
        }
    }else{
        SEND_END_COLOR_BCK: for(unsigned int i = 0; i < _FPGA_CLS_STATES_PARTITION; i++){
            toStateUpdater[i].write((colorValue){.clsID=0,.litID=0,.depthCount=0,.didSolve=false,.clsEos=false,.streamEos=true});
        }
    }

    if(type == 0){
        FLUSH_SPLIT_ACCESS: while(!readOne.eos){
            #pragma HLS loop_tripcount min=16 max=16

            readOne = toColorStream.read();
            splitResidualCnt++;
        }

        if(!didRead){
            stopSendingRead = stopSending->read();
        }
    }
    
}
