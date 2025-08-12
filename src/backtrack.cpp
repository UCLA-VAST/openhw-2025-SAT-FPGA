#include "backtrack.h"

void undoStates(hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput, hls::stream<colorAssignment>& toCommitStream,
    const lit answerStack[_FPGA_MAX_LITERALS], literalMetaData lmd[_FPGA_MAX_LITERALS],
    const lit literalCommit, unsigned int& answerStackHeight, 
    const unsigned int backtrackHeight, const ap_uint<1> POSITIVE_LIT_PHASE_VAL){
    #pragma HLS inline off

    bool foundLastCommit = false;
    unsigned int answerStackHeightCopy = answerStackHeight;

    GET_UNDO_LITERALS: for(unsigned int i = 0; i < backtrackHeight; i++){
        #pragma HLS loop_tripcount min=1024 max=1024
        #pragma HLS dependence variable=lmd inter false
        #pragma HLS pipeline II=1

        lit getLit = answerStack[--answerStackHeight];
        literalMetaData getLmd = lmd[abs(getLit)-1];

        if(literalCommit == getLit){
            foundLastCommit = true;
        }

        if(foundLastCommit){
            ap_uint<1> selectSide = 0;
            if(getLit > 0){
                selectSide = 1;   
            }

            
            unsigned int numElements = (unsigned int)LMD_NUM_ELE(getLmd.compactlmd,selectSide);
            if(numElements > 0){
                toCommitStream.write((colorAssignment){.addressStart=(unsigned int)LMD_ADDR_START(getLmd.compactlmd,selectSide),
                    .numElements=numElements,.depthCount=0,.literal=getLit,.eos=false});
            }     
            ap_axiu<32,0,0,0> sendPQHandler;
            sendPQHandler.data = abs(getLit);
            pqHandlerInput.write(sendPQHandler);
        }

        literalMetaData wipeMetalmd = getLmd;

        LMD_IS_IN_STACK(wipeMetalmd.compactlmd) = false;
        if(getLit > 0){
            LMD_PHASE(wipeMetalmd.compactlmd) = POSITIVE_LIT_PHASE_VAL;
        }else{
            LMD_PHASE(wipeMetalmd.compactlmd) = !POSITIVE_LIT_PHASE_VAL;
        }

        lmd[abs(getLit)-1] = wipeMetalmd;
        
    }

    toCommitStream.write((colorAssignment){.addressStart=0,.depthCount=0,.literal=0,.eos=true});
}

void updateStatesBackward(hls::stream<colorValue>& toStateUpdater, 
    clsState clsStates[_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION], const unsigned int id){
    #pragma HLS inline off

    const int DEPENDENCY = _FPGA_CLS_DEP_DIST-1;
    colorValue get;
    cls clsID = 0;
    get.clsID = 0;
    get.streamEos = false;
    get.clsEos = false;
    bool didGet = false;
    const unsigned int READ_SIZE_CHUNK = _FPGA_CLS_STATES_PARTITION;
    int currentIndex = READ_SIZE_CHUNK-1;

    unsigned int cacheAddr[_FPGA_CLS_DEP_DIST];
    #pragma HLS array_partition variable=cacheAddr complete
    clsState cacheState[_FPGA_CLS_DEP_DIST];
    #pragma HLS array_partition variable=cacheState complete

    INVALID_ADDR: for(unsigned int i = 0; i < _FPGA_CLS_DEP_DIST; i++){
        cacheAddr[i] = _FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION;
    }

    UPDATE_BACKWARDS: while(true){
        #pragma HLS loop_tripcount min=512 max=512
        #pragma HLS dependence variable=clsStates inter true distance=DEPENDENCY
        #pragma HLS pipeline II=1

        if(didGet){
            if(get.streamEos){
                break; 
            }else{
                if(clsID != 0){
                    unsigned int addr = (clsID-1)/_FPGA_CLS_STATES_PARTITION;
                    clsState getState = clsStates[addr];

                    for(unsigned int i = 0; i < _FPGA_CLS_DEP_DIST; i++){
                        if(addr == cacheAddr[i]){
                            getState = cacheState[i];
                        }
                    }
                    
                    getState.remainingUnassigned++;
                    getState.compressedList ^= (-get.litID);
                        
                    clsStates[reg(addr)] = reg(getState);

                    for(unsigned int i = 0; i < _FPGA_CLS_DEP_DIST-1; i++){
                        cacheState[i] = cacheState[i+1];
                        cacheAddr[i] = cacheAddr[i+1];
                    }
                    cacheState[_FPGA_CLS_DEP_DIST-1] = getState;
                    cacheAddr[_FPGA_CLS_DEP_DIST-1] = addr;
                }
            }

        }

        if(currentIndex == READ_SIZE_CHUNK-1){
            didGet = toStateUpdater.read_nb(get);
        }
        if(didGet){
            clsID = 0;
            currentIndex = READ_SIZE_CHUNK-1;
            for(int i = READ_SIZE_CHUNK-1; i >= 0; i--){
                cls tmpID = get.clsID.range(32*i+31,32*i);
                if((tmpID-1)%_FPGA_CLS_STATES_PARTITION == id && tmpID != 0){
                    clsID = tmpID;
                    currentIndex = i;
                }
            }
            get.clsID.range(32*currentIndex+31,32*currentIndex) = 0;
        }
    }
}

void undo_states_dataflow_wrapper(hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput, 
    clsState clsStates[_FPGA_CLS_STATES_PARTITION][_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION],
    const lit answerStack[_FPGA_MAX_LITERALS], literalMetaData lmd[_FPGA_MAX_LITERALS],
    const ap_uint<512> literalStore[_FPGA_MAX_LITERAL_ELEMENTS/16],
    const lit literalCommit, const unsigned int backtrackHeight, unsigned int& answerStackHeight,
    const unsigned int LITERAL_PAGE_SIZE, const ap_uint<1> POSITIVE_LIT_PHASE_VAL, ap_uint<64> litStoreAccessStats[4]){

    #pragma HLS inline off

    hls::stream<colorAssignment> toColorStream;
    #pragma HLS stream variable=toColorStream depth=4

    hls::stream<colorValue> toStateUpdater[_FPGA_CLS_STATES_PARTITION];
    #pragma HLS stream variable=toStateUpdater depth=4
    #pragma HLS array_partition variable=toStateUpdater complete

    #pragma HLS dataflow

    undoStates(pqHandlerInput, toColorStream,
        answerStack, lmd, literalCommit, answerStackHeight, 
        backtrackHeight, POSITIVE_LIT_PHASE_VAL);

    colorStream(toStateUpdater, toColorStream, nullptr, literalStore, LITERAL_PAGE_SIZE, nullptr, 1, litStoreAccessStats);

    updateStatesBackward(toStateUpdater[0], clsStates[0], 0);
    updateStatesBackward(toStateUpdater[1], clsStates[1], 1);
    updateStatesBackward(toStateUpdater[2], clsStates[2], 2);
    updateStatesBackward(toStateUpdater[3], clsStates[3], 3);
    updateStatesBackward(toStateUpdater[4], clsStates[4], 4);
    updateStatesBackward(toStateUpdater[5], clsStates[5], 5);
    updateStatesBackward(toStateUpdater[6], clsStates[6], 6);
    updateStatesBackward(toStateUpdater[7], clsStates[7], 7);
}
