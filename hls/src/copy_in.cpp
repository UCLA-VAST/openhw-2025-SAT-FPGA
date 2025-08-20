#include "copy_in.h"

void copy_clsStates(clsState mClsStates[_FPGA_CLS_STATES_PARTITION][_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION], clsStatePCIE* clsStates, const unsigned int numClauses){
    #pragma HLS inline off
    COPY_CLS_STATES: for(unsigned int i = 0; i < numClauses; i++){
        #pragma HLS loop_tripcount min=1024 max=1024
        clsStatePCIE get = reg(reg(clsStates[i]));
        clsState transfer;

        transfer.compressedList = get.compressedList;
        transfer.remainingUnassigned = get.remainingUnassigned;
        mClsStates[i%_FPGA_CLS_STATES_PARTITION][i/_FPGA_CLS_STATES_PARTITION] = transfer;
    }
}

void copy_lmd(literalMetaData mlmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData mlmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS], literalMetaDataPCIE* lmd, const unsigned int NUM_LITERALS){
    #pragma HLS inline off
    COPY_LMD: for(unsigned int i = 0; i < NUM_LITERALS; i++){
        #pragma HLS loop_tripcount min=1024 max=1024
        #pragma HLS pipeline
        for(unsigned int j = 0; j < _FPGA_PARALLEL_MINIMIZE; j++){
            mlmmd[j][i].compactlmmd = 0;
        }

        literalMetaDataPCIE get = (reg(reg(lmd[i])));
        literalMetaData transfer;
        transfer.compactlmd = get.compactlmd(385,0);
        transfer.decisionLevelStackEnd = get.compactlmd(417,386);
        
        mlmd[i] = transfer;
    }
}

void copy_answerStack(lit mAnswerStack[_FPGA_MAX_LITERALS], lit* answerStack, const unsigned int NUM_LITERALS){
    #pragma HLS inline off
    COPY_ANSWER_STACK: for(unsigned int i = 0; i < NUM_LITERALS; i++){
        #pragma HLS loop_tripcount min=1024 max=1024
        mAnswerStack[i] = reg(reg(answerStack[i]));
    }
}

void copy_litStore(ap_uint<512> mLitStore[_FPGA_MAX_LITERAL_ELEMENTS/16], ap_int<512>* litStore, const unsigned int literalElements){
    #pragma HLS inline off

    unsigned int literalElements16 = literalElements/16;
    if(literalElements%16 != 0){
        literalElements16++;
    }
    COPY_LIT_STORE: for(unsigned int i = 0; i < literalElements16; i++){
        #pragma HLS loop_tripcount min=1024 max=1024
        mLitStore[i] = reg(reg(litStore[i]));
    }
}

void copy_in_dataflow_wrapper(clsState mClsStates[_FPGA_CLS_STATES_PARTITION][_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION], 
    ap_uint<512> mLitStore[_FPGA_MAX_LITERAL_ELEMENTS/16],
    literalMetaData mlmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData mlmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS], 
    lit mAnswerStack[_FPGA_MAX_LITERALS], 
    clsStatePCIE* clsStates, ap_int<512>* litStore1, literalMetaDataPCIE* lmd, 
    lit* answerStack,
    const unsigned int literalElements, const unsigned int numClauses, const unsigned int NUM_LITERALS, 
    volatile uint64_t store[2]){
    #pragma HLS inline off
    #pragma HLS dataflow

    store[0]++;
    store[1]++;

    copy_clsStates(mClsStates, clsStates, numClauses);
    copy_litStore(mLitStore, litStore1, literalElements);
    copy_lmd(mlmd, mlmmd, lmd, NUM_LITERALS);
    copy_answerStack(mAnswerStack, answerStack, NUM_LITERALS);
    
}