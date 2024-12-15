#include "minimize.h"

void minimize_dispatch(hls::stream<lit>& toMinimizeStream,
    hls::stream<lit> splitMinimizeStream[2], bool foundAbsolute){
    #pragma HLS inline off

    if(foundAbsolute){
        return;
    }
    
    if(!toMinimizeStream.empty()){
        ap_uint<1> select = 0;
        unsigned int idx = 0;

        lit getLit = toMinimizeStream.read();
        DISPATCH: while(true){
            #pragma HLS loop_tripcount min=32 max=32

            if(splitMinimizeStream[select].write_nb(getLit)){
                if(toMinimizeStream.empty()){
                    break;
                }
                getLit = toMinimizeStream.read();
            }
            #ifdef FPGA_HW
            select++;
            #endif
        }
    }
    splitMinimizeStream[0].write(0);
    splitMinimizeStream[1].write(0);
}

void minimize_resolution_sort(hls::stream<lit_resolve>& mrsp2Stream, hls::stream<lit>& fromClsStore, 
    ap_uint<2> mergeScratchPad[_FPGA_MAX_LITERALS], ap_uint<512> validBit[_FPGA_MAX_LITERALS/512],
    unsigned int& numElements, ap_uint<64>& learnedStats){
    #pragma HLS inline off

    const int RESOLVE_DEP = _FPGA_RESOLVE_DEP_DIST;
    ap_uint<2> mergeScratchPadResolved = 0;
    ap_uint<512> getBitReg[_FPGA_RESOLVE_DEP_DIST];
    #pragma HLS array_partition variable=getBitReg complete
    unsigned int validBitAddrIndexReg[_FPGA_RESOLVE_DEP_DIST];
    #pragma HLS array_partition variable=validBitAddrIndexReg complete

    for(unsigned int i = 0; i < _FPGA_RESOLVE_DEP_DIST; i++){
        #pragma HLS unroll
        validBitAddrIndexReg[i] = _FPGA_MAX_LITERALS;
    }

    RESOLVE_MINIMIZE: while(true){
        #pragma HLS loop_tripcount min=16 max=16
        #pragma HLS pipeline 
        #pragma HLS dependence variable=mergeScratchPad inter false
        #pragma HLS dependence variable=validBit inter distance=RESOLVE_DEP true

        lit readFromClsStore = fromClsStore.read();
        if(readFromClsStore == 0){
            break;
        }
        ap_uint<2> mergeStatus = mergeScratchPad[abs(readFromClsStore)-1];

        unsigned int validBitAddrIndex = (abs(readFromClsStore)-1)/512;
        unsigned int validBitAddrSubIndex = (abs(readFromClsStore)-1)%512;
        ap_uint<512> getBit = validBit[validBitAddrIndex];

        for(unsigned int j = 0; j < _FPGA_RESOLVE_DEP_DIST; j++){
            if(validBitAddrIndexReg[j] == validBitAddrIndex){
                getBit = getBitReg[j];
            }
        }

        if(getBit.range(validBitAddrSubIndex,validBitAddrSubIndex) == 0){
            mergeStatus = 0;
            getBit.range(validBitAddrSubIndex,validBitAddrSubIndex) = 1;
        }
        validBit[validBitAddrIndex] = getBit;
        
        for(unsigned int j = 0; j < _FPGA_RESOLVE_DEP_DIST-1; j++){
            validBitAddrIndexReg[j] = validBitAddrIndexReg[j+1];
            getBitReg[j] = getBitReg[j+1];
        }
        validBitAddrIndexReg[_FPGA_RESOLVE_DEP_DIST-1] = validBitAddrIndex;
        getBitReg[_FPGA_RESOLVE_DEP_DIST-1] = getBit;

        bool insert = false;
        learnedStats++;

        if(readFromClsStore> 0){
            if(mergeStatus.range(0,0) == 0){
                mergeStatus.range(0,0) = 1;
                insert = true;
            }
        }else{
            if(mergeStatus.range(1,1) == 0){
                mergeStatus.range(1,1) = 1;
                insert = true;
            }
        }

        if(mergeStatus == 3){
            numElements--;
        }else if(insert){
            numElements++;
            mergeScratchPad[abs(readFromClsStore)-1] = mergeStatus;

            mrsp2Stream.write((lit_resolve){.literal=readFromClsStore,.resolved=false});
        }
    }

    mrsp2Stream.write((lit_resolve){.literal=0,.resolved=false});
    mrsp2Stream.write((lit_resolve){.literal=(int)numElements,.resolved=false});
}

void minimize_resolution_sort_part_2(myStream<lit,_FPGA_MAX_LEARN_ELE,_FPGA_MAX_LEARN_ELE_BITS>& nextLiteralMinimize, hls::stream<lit_resolve>& mrsp2Stream, 
    const literalMinimizeMetaData lmmd[_FPGA_MAX_LITERALS], unsigned int& countMarked, int& exitCondition){
    #pragma HLS inline off

    bool hitUnmarkDecided = false;
    MINIMIZE_SORT_2: while(true){
        #pragma HLS loop_tripcount min=1024 max=1024
        #pragma HLS pipeline II=1

        lit_resolve get;
        if(mrsp2Stream.read_nb(get)){
            if(get.literal == 0){
                break;
            }

            if(!get.resolved){
                literalMinimizeMetaData checkMinimizedLmd = lmmd[abs(get.literal)-1];
                bool cmp = LMMD_MIN_KEEP(checkMinimizedLmd.compactlmmd) > 0 || LMMD_IS_IN_FIX_STACK(checkMinimizedLmd.compactlmmd) == 1;

                if(cmp && !hitUnmarkDecided){
                    countMarked++;
                }
                if(!cmp && !hitUnmarkDecided){
                    nextLiteralMinimize.array[nextLiteralMinimize.head] = abs(get.literal);
                    nextLiteralMinimize.head++;
                }

                if(LMMD_IS_DECIDE(checkMinimizedLmd.compactlmmd) == 1 && !cmp && !hitUnmarkDecided){
                    hitUnmarkDecided = true;
                }
            }
        }
    }

    lit_resolve get = mrsp2Stream.read();

    //get.literal is actually number of elements here
    if(!hitUnmarkDecided && countMarked == (unsigned int)get.literal){
        exitCondition = 1;
    }
    if(hitUnmarkDecided){
        exitCondition = 2;
    }
}

void minimize_dataflow_wrapper_layer_2(myStream<lit,_FPGA_MAX_LEARN_ELE,_FPGA_MAX_LEARN_ELE_BITS>& nextLiteralMinimize,
    ap_uint<2> mergeScratchPad[_FPGA_MAX_LITERALS], ap_uint<512> validBit[_FPGA_MAX_LITERALS/512],
    const literalMinimizeMetaData lmmd[_FPGA_MAX_LITERALS], unsigned int& numElements, 
    unsigned int& countMarked, int& exitCondition, ap_uint<64>& learnedStats,
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream){
    #pragma HLS inline off

    hls::stream<cls> prefetchClsStore;
    #pragma HLS stream variable=prefetchClsStore depth=16
    hls::stream<lit_resolve> mrsp2Stream;
    #pragma HLS stream variable=mrsp2Stream depth=16

    #pragma HLS dataflow

    clause_store_prefetch(prefetchClsStore, clauseStoreOutputStream);
    minimize_resolution_sort(mrsp2Stream, prefetchClsStore, 
        mergeScratchPad, validBit, numElements, learnedStats);
    minimize_resolution_sort_part_2(nextLiteralMinimize, mrsp2Stream, 
        lmmd, countMarked, exitCondition);
}

void minimize_dataflow_wrapper_layer_1(hls::stream<lit>& toSplitStream,
    literalMinimizeMetaData lmmd[_FPGA_MAX_LITERALS], 
    ap_uint<512> validBitMinimize[_FPGA_MAX_LITERALS/512],
    ap_uint<2> mergeScratchPadMinimize[_FPGA_MAX_LITERALS],
    unsigned int& nonRemovableCount, bool& didSimplify, 
    const cls unitByCls[_FPGA_MAX_LITERALS], const bool foundAbsolute,
    const unsigned int clearIterations, ap_uint<64> minimizeStats[2], unsigned int& overheadClear,  
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream){
    #pragma HLS inline off

    if(foundAbsolute){
        return;
    }
    
    bool doCompute = false;

    lit getLit;
    literalMinimizeMetaData getLmmd;

    unsigned int minimizeElements;
    unsigned int countMarked;

    myStream<lit,_FPGA_MAX_LEARN_ELE,_FPGA_MAX_LEARN_ELE_BITS> nextLiteralMinimize;
    cls nextClauseMergeMinimize;
    ap_axiu<96,0,0,0> sendClauseInputCommand;

    MINIZE: while(true){
        #pragma HLS loop_tripcount min=32 max=32

        if(!doCompute){
            nextLiteralMinimize.head = 0;
            nextLiteralMinimize.tail = 0;

            minimizeElements = 0;
            countMarked = 0;

            getLit = toSplitStream.read();

            if(getLit == 0){
                break;
            }

            getLmmd = lmmd[abs(getLit)-1];
            nextClauseMergeMinimize = unitByCls[abs(getLit)-1];
            if(LMMD_IS_DECIDE(getLmmd.compactlmmd) == 1 && LMMD_IS_IN_FIX_STACK(getLmmd.compactlmmd) == 0){
                nonRemovableCount++;
            }

            if(!(LMMD_IS_IN_FIX_STACK(getLmmd.compactlmmd) == 1 || LMMD_IS_DECIDE(getLmmd.compactlmmd) == 1)){
                doCompute = true;
            }else{
                continue;
            }   
        }

        int exitCondition = 0;
        minimizeStats[0]++;

        sendClauseInputCommand.data.range(31,0) = nextClauseMergeMinimize-1;
        sendClauseInputCommand.data.range(63,32) = 0;
        sendClauseInputCommand.data.range(95,64) = csh::SEND_CLS;    
        clauseStoreInputStream.write(sendClauseInputCommand);
        
        minimize_dataflow_wrapper_layer_2(nextLiteralMinimize,
            mergeScratchPadMinimize, validBitMinimize, lmmd,
            minimizeElements, countMarked, exitCondition, 
            minimizeStats[1], clauseStoreOutputStream);

        bool needClear = false;

        if(exitCondition == 0){
            lit nextLiteral = nextLiteralMinimize.array[nextLiteralMinimize.tail];
            nextClauseMergeMinimize = unitByCls[nextLiteral-1];

            nextLiteralMinimize.tail++;     
        }else if(exitCondition == 1){
            LMMD_MIN_KEEP(getLmmd.compactlmmd) = 2;
            didSimplify = true;

            lmmd[abs(getLit)-1] = getLmmd;
                            
            doCompute = false;
            needClear = true;
        }else if(exitCondition == 2){
            doCompute = false;
            needClear = true;
            nonRemovableCount++;
        }

        if(needClear){
            ZERO_SEQ_2: for(unsigned int j = 0; j < clearIterations; j++){
                #pragma HLS loop_tripcount min=64 max=64
                #pragma HLS pipeline 
                validBitMinimize[j] = 0;        
            }
            
            overheadClear += clearIterations;
        } 
    
    }
}