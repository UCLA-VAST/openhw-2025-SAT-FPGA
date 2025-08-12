#include "discover.h"

bool seenSecond = false;
void discover(hls::stream<colorAssignment>& toCommitStream, hls::stream<bcpPacket>& toDecide,
    hls::stream<int>& duplicateCountStream, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream,
    const flippedLiteral litToCheck,
    lit answerStack[_FPGA_MAX_LITERALS], literalMetaData lmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    cls unitByCls[_FPGA_MAX_LITERALS],
    unsigned int& answerStackHeight, const unsigned int decisionLevel, unsigned int& fixedDecisionStackHeight,
    const lit topLiteral, const bool useFlipped, const ap_uint<1> POSITIVE_LIT_PHASE_VAL, bool& skipCPUOnce){
    #pragma HLS inline off

    unsigned int duplicateCount = 0;
    #ifndef FPGA_HW
    if(!skipCPUOnce){
    #endif
    
    if(!useFlipped){
        checkUndecided(toCommitStream,
            answerStack, lmd, lmmd, unitByCls,
            answerStackHeight, duplicateCount, topLiteral,
            fixedDecisionStackHeight, decisionLevel, POSITIVE_LIT_PHASE_VAL);
    }else{
        lmd[abs(litToCheck.literal)-1] = litToCheck.lmd;
        for(unsigned int j = 0; j < _FPGA_PARALLEL_MINIMIZE; j++){
            #pragma HLS unroll
            lmmd[j][abs(litToCheck.literal)-1] = litToCheck.lmmd;
        }

        answerStack[answerStackHeight] = litToCheck.literal;
        
        ap_uint<1> selectSide = 0;
        if(litToCheck.literal > 0){
            selectSide = 1;   
        }

        unsigned int numElements = (unsigned int)LMD_NUM_ELE(litToCheck.lmd.compactlmd,selectSide);
        toCommitStream.write((colorAssignment){.addressStart=(unsigned int)LMD_ADDR_START(litToCheck.lmd.compactlmd,selectSide),
            .numElements=numElements,.depthCount=0,.literal=litToCheck.literal,.eos=false});
        if(numElements == 0){
            duplicateCount++;
        }

        answerStackHeight++;
    }

    #ifndef FPGA_HW
    skipCPUOnce = true;
    }
    #endif

    lit cacheLit[_FPGA_DISC_LMD_DEP_DIST];
    #pragma HLS array_partition variable=cacheLit complete
    literalMinimizeMetaData cacheLmmd[_FPGA_DISC_LMD_DEP_DIST];
    #pragma HLS array_partition variable=cacheLmmd complete
    literalMetaData cacheLmd[_FPGA_DISC_LMD_DEP_DIST];
    #pragma HLS array_partition variable=cacheLmd complete

    INVALID_ADDR_DISCOVER: for(unsigned int i = 0; i < _FPGA_DISC_LMD_DEP_DIST; i++){
        cacheLit[i] = _FPGA_MAX_LITERALS;
    }
    
    bcpPacket get;
    get.literalUnitted = _FPGA_MAX_LITERALS;
    get.unitByClause = 0;
    get.unitByLit = 0;
    const int DEPENDENCY = _FPGA_DISC_LMD_DEP_DIST;
    literalMinimizeMetaData getLmmd;
    literalMetaData getLmd;
    #ifndef FPGA_HW
    BCP_DISCOVER: while(!toDecide.empty()){
    #else
    BCP_DISCOVER: while(true){
    #endif
        #pragma HLS loop_tripcount min=1024 max=1024
        #pragma HLS dependence variable=lmd inter true distance=DEPENDENCY
        #pragma HLS dependence variable=lmmd inter true distance=DEPENDENCY

        if(get.literalUnitted == 0){
            break;
        }
        const bool dataAvailable = toDecide.read_nb(get);
        
        lit absLit = abs(get.literalUnitted)-1;
        ap_uint<1> selectSide = 0;
        if(get.literalUnitted > 0){
            selectSide = 1;   
        }

        if(dataAvailable && get.literalUnitted != 0){
            unsigned int clsLength = clauseStoreOutputStream.read().data;
            bool found = false;

            for(unsigned int i = 0; i < _FPGA_DISC_LMD_DEP_DIST; i++){
                if(cacheLit[i] == absLit){
                    getLmmd = cacheLmmd[i];
                    getLmd = cacheLmd[i];
                    found = true;
                }
            }
            if(!found){
                getLmmd = lmmd[0][absLit];
                getLmd = lmd[absLit];
            }
            bool cmp = answerStack[(int)LMD_INSERT_LVL(getLmd.compactlmd)] == get.literalUnitted && 
                    (int)LMD_UNIT_BY_LIT(getLmd.compactlmd) == get.unitByLit && clsLength < LMD_SHORTEST_CLS_LENGTH(getLmd.compactlmd);

            if(!LMD_IS_IN_STACK(getLmd.compactlmd)){
                LMD_INSERT_LVL(getLmd.compactlmd) = answerStackHeight;
                LMD_DEC_LVL(getLmd.compactlmd) = decisionLevel;
                LMD_IS_IN_STACK(getLmd.compactlmd) = true;
                LMD_UNIT_BY_LIT(getLmd.compactlmd) = get.unitByLit;

                unitByCls[absLit] = get.unitByClause;
                LMD_SHORTEST_CLS_LENGTH(getLmd.compactlmd) = clsLength;
                LMMD_IS_DECIDE(getLmmd.compactlmmd) = false;

                if(decisionLevel == 0){
                    LMMD_IS_IN_FIX_STACK(getLmmd.compactlmmd) = true;
                    fixedDecisionStackHeight++;
                }

                unsigned int numElements = (unsigned int)LMD_NUM_ELE(getLmd.compactlmd,selectSide);
                    
                toCommitStream.write((colorAssignment){.addressStart=(unsigned int)LMD_ADDR_START(getLmd.compactlmd,selectSide),
                    .numElements=numElements,.depthCount=get.depthCount,.literal=get.literalUnitted,.eos=false});
                if(numElements == 0){
                   duplicateCount++; 
                }

                answerStack[answerStackHeight] = get.literalUnitted;
                answerStackHeight++;
                
                lmd[absLit] = getLmd;
                for(unsigned int j = 0; j < _FPGA_PARALLEL_MINIMIZE; j++){
                    lmmd[j][absLit] = getLmmd;
                }
            }else{
                if(cmp){
                    unitByCls[absLit] = get.unitByClause;
                    LMD_SHORTEST_CLS_LENGTH(getLmd.compactlmd) = clsLength;
                    lmd[absLit] = getLmd;
                }
                duplicateCount++;
            }
        }else if(duplicateCount != 0){
            if(duplicateCountStream.write_nb(duplicateCount)){
                duplicateCount = 0;
            }
        }
        for(unsigned int i = 0; i < _FPGA_DISC_LMD_DEP_DIST-1; i++){
            cacheLit[i] = cacheLit[i+1];
            cacheLmd[i] = cacheLmd[i+1];
            cacheLmmd[i] = cacheLmmd[i+1];
        }
        cacheLit[_FPGA_DISC_LMD_DEP_DIST-1] = absLit;
        cacheLmd[_FPGA_DISC_LMD_DEP_DIST-1] = getLmd;
        cacheLmmd[_FPGA_DISC_LMD_DEP_DIST-1] = getLmmd;
    }
    
    toCommitStream.write((colorAssignment){.addressStart=0,.depthCount=0,.literal=0,.eos=true});
    duplicateCountStream.write(-1);
}

void updateStatesForward(hls::stream<clsStateControlPacket>& toControlSink, 
    hls::stream<colorValue>& toStateUpdater,
    clsState clsStates1[_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION], clsState clsStates2[_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION],
    const unsigned int id){
    #pragma HLS inline off

    const int DEPENDENCY = _FPGA_CLS_DEP_DIST-1;
    clsStateControlPacket ctrlPkt;
    colorValue get;
    cls clsID = 0;
    get.clsID = 0;
    get.streamEos = false;
    get.clsEos = false;
    bool doBacktrack = false;
    bool didGet = false;
    int clsEosCount = 0;
    const unsigned int READ_SIZE_CHUNK = _FPGA_CLS_STATES_PARTITION;
    int currentIndex = READ_SIZE_CHUNK-1;
    bool shifted = false;
    bool getNew = true;

    unsigned int cacheClsAddr[_FPGA_CLS_DEP_DIST];
    #pragma HLS array_partition variable=cacheClsAddr complete
    clsState cacheClsState[_FPGA_CLS_DEP_DIST];
    #pragma HLS array_partition variable=cacheClsState complete

    INVALID_ADDR: for(unsigned int i = 0; i < _FPGA_CLS_DEP_DIST; i++){
        cacheClsAddr[i] = _FPGA_MAX_CLAUSES;
    }

    UPDATE_FORWARD: while(true){
        #pragma HLS loop_tripcount min=512 max=512
        #pragma HLS dependence variable=clsStates1 inter true distance=DEPENDENCY
        #pragma HLS dependence variable=clsStates2 inter true distance=DEPENDENCY
        #pragma HLS pipeline II=1

        if(didGet){
            if(get.streamEos){
                break;
            }else{
                if(clsID != 0){
                    unsigned int bigAddr = (clsID-1)%2;
                    unsigned int subAddr = (clsID-1)/_FPGA_CLS_STATES_PARTITION;
                    
                    clsState getState;
                    if(bigAddr == 0){
                        getState = clsStates1[subAddr];
                    }else if(bigAddr == 1){
                        getState = clsStates2[subAddr];
                    }

                    for(unsigned int i = 0; i < _FPGA_CLS_DEP_DIST; i++){
                        if(clsID-1 == cacheClsAddr[i]){
                            getState = cacheClsState[i];
                        }
                    }

                    getState.remainingUnassigned--;
                    getState.compressedList ^= (-get.litID);

                    if(getState.remainingUnassigned == 1 && !doBacktrack){
                        ctrlPkt.pktType = solverCode::UNIT;
                        ctrlPkt.eosCount = 0;
                        ctrlPkt.bcp.unitByClause = clsID;
                        ctrlPkt.bcp.literalUnitted = getState.compressedList;
                        ctrlPkt.bcp.unitByLit = get.litID;
                        ctrlPkt.bcp.depthCount = get.depthCount;

                        toControlSink.write(ctrlPkt);
                    }else if(getState.remainingUnassigned == 0){
                        ctrlPkt.pktType = solverCode::BACKTRACK;
                        ctrlPkt.eosCount = 0;
                        ctrlPkt.bcp.unitByClause = clsID;
                        ctrlPkt.bcp.literalUnitted = 0;
                        ctrlPkt.bcp.unitByLit = 0;
                        ctrlPkt.bcp.depthCount = get.depthCount;

                        toControlSink.write(ctrlPkt);

                        doBacktrack = true;
                    }

                    if(bigAddr == 0){
                        clsStates1[reg(subAddr)] = reg(getState);
                    }else if(bigAddr == 1){
                        clsStates2[reg(subAddr)] = reg(getState);
                    }
                    for(unsigned int i = 0; i < _FPGA_CLS_DEP_DIST-1; i++){
                        cacheClsState[i] = cacheClsState[i+1];
                        cacheClsAddr[i] = cacheClsAddr[i+1];
                    }
                    cacheClsState[_FPGA_CLS_DEP_DIST-1] = getState;
                    cacheClsAddr[_FPGA_CLS_DEP_DIST-1] = clsID-1;

                }
                if(get.clsEos){
                    clsEosCount++;   
                    get.clsEos = false;
                }
            }  
        }else{
            if(clsEosCount != 0){
                ctrlPkt.pktType = solverCode::EOS_CNT;
                ctrlPkt.eosCount = clsEosCount;
                ctrlPkt.bcp.unitByClause = 0;
                ctrlPkt.bcp.literalUnitted = 0;
                ctrlPkt.bcp.unitByLit = 0;

                toControlSink.write(ctrlPkt);

                clsEosCount = 0;
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
                if(((tmpID-1)%_FPGA_CLS_STATES_PARTITION == id || (tmpID-1)%_FPGA_CLS_STATES_PARTITION == id+1) && tmpID != 0){
                    clsID = tmpID;
                    currentIndex = i;
                }
            }
            get.clsID.range(32*currentIndex+31,32*currentIndex) = 0;
        }
    }

    toControlSink.write((clsStateControlPacket){.eosCount=-1,.bcp=(bcpPacket){.literalUnitted=0,.unitByClause=0,.unitByLit=0,.depthCount=0},.pktType=solverCode::EOS_CNT});
}

void muxControlSink(hls::stream<clsStateControlPacket>& toControlSink, 
    hls::stream<clsStateControlPacket> toControlSinkMUX[_FPGA_CLS_STATES_PARTITION/2]){
    #pragma HLS inline off

    clsStateControlPacket ctrlPkt;
    ap_uint<_FPGA_CLS_STATES_PARTITION/2> exit = 0;
    bool didGet = false;
    ap_uint<_FPGA_CLS_STATES_PARTITION/2> selectMask = 0;
    const int EXIT_VALUE = pow(2,_FPGA_CLS_STATES_PARTITION/2)-1;

    CONTROL_MUX: while(true){
        #pragma HLS loop_tripcount min=1024 max=1024
        #pragma HLS pipeline II=1

        if(exit == EXIT_VALUE){
            break;
        }

        if(selectMask == 0){
            for(unsigned int i = 0; i < _FPGA_CLS_STATES_PARTITION/2; i++){
                selectMask.range(i,i) = !toControlSinkMUX[i].empty();
                
            }
        }

        ap_uint<_FPGA_CLS_STATES_SELECT_BITS> select = 0;
        for(unsigned int i = 0; i < _FPGA_CLS_STATES_PARTITION/2; i++){
            if(selectMask.range(i,i)){
                select = i;
                selectMask.range(i,i) = 0;
                didGet = true;
                break;
            }
        }

        if(didGet){
            if(toControlSinkMUX[select].read_nb(ctrlPkt)){
                if(ctrlPkt.bcp.unitByClause == 0 && ctrlPkt.eosCount == -1){
                    exit.range(select,select) = 1;
                }
                toControlSink.write(ctrlPkt);
                didGet = false;
            }
        }
    }
}

void controlSink(hls::stream<bcpPacket>& toDecide,
    hls::stream<clsStateControlPacket>& toControlSink,
    hls::stream<int>& duplicateCountStream,
    hls::stream<bool>& stopSending, hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1,
    myStream<cls,64,7>& unsatClauses, bool& doBackTrack, 
    const unsigned int decisionLevel, const unsigned int fixedDecisionStackHeight, const bool firstIteration,
    bool& flushSignal){
    #pragma HLS inline off

    volatile int sendToStack = _FPGA_CLS_STATES_PARTITION/2;
    bool writeOnce = false;
    clsStateControlPacket ctrlPkt;
    ap_uint<_FPGA_CLS_STATES_SELECT_BITS-2+1> exit = 0;
    bool finishCount = false;
    unsigned int currentDepth = UINT_MAX;

    if(!firstIteration){
        sendToStack = fixedDecisionStackHeight*(_FPGA_CLS_STATES_PARTITION/2);
        if(sendToStack == 0){
            sendToStack = _FPGA_CLS_STATES_PARTITION/2;
        }
    }

    int readCount = 0;
    CONTROL_LOOP: while(true){
        #pragma HLS loop_tripcount min=1024 max=1024
        #pragma HLS pipeline II=1

        if(exit == _FPGA_CLS_STATES_PARTITION/2){
            break;
        }

        bool dataAvailable = toControlSink.read_nb(ctrlPkt);
        if(dataAvailable){
            if(ctrlPkt.bcp.unitByClause == 0 && ctrlPkt.eosCount == -1){
                exit++;
            }else if(ctrlPkt.eosCount != 0){
                sendToStack -= ctrlPkt.eosCount;
            }else{
                if(ctrlPkt.pktType == solverCode::BACKTRACK){
                    if(ctrlPkt.bcp.depthCount < currentDepth){
                        unsatClauses.head = 1;
                        unsatClauses.tail = 0;
                        currentDepth = ctrlPkt.bcp.depthCount;
                        unsatClauses.array[0] = ctrlPkt.bcp.unitByClause;
                    }else if(ctrlPkt.bcp.depthCount == currentDepth){
                        if(unsatClauses.head < 2){
                            unsatClauses.array[unsatClauses.head] = ctrlPkt.bcp.unitByClause;
                            unsatClauses.head++;
                        }
                    }

                    if(!doBackTrack){
                        stopSending.write(true);

                        doBackTrack = true;
                        writeOnce = true;
                    }
                }else if(ctrlPkt.pktType == solverCode::UNIT){// && !doBackTrack){
                    ap_axiu<96,0,0,0> sendClauseInputCommand;
                    sendClauseInputCommand.data.range(95,64) = 0;
                    sendClauseInputCommand.data.range(63,32) = 0;
                    sendClauseInputCommand.data.range(31,0) = ctrlPkt.bcp.unitByClause-1;

                    clauseStoreInputStream1.write(sendClauseInputCommand);

                    ctrlPkt.bcp.depthCount = ctrlPkt.bcp.depthCount+1;
                    toDecide.write(ctrlPkt.bcp);
                    sendToStack += _FPGA_CLS_STATES_PARTITION/2;
                }
            }
        }
        #ifdef FPGA_HW
        else if(duplicateCountStream.read_nb(readCount)){
            if(readCount != -1){
                sendToStack -= (readCount*_FPGA_CLS_STATES_PARTITION/2);
            }else{
                finishCount = true;
            }
        }else if(sendToStack == 0 && !writeOnce && !doBackTrack){
            writeOnce = true;
            stopSending.write(false);
            
            toDecide.write((bcpPacket){.literalUnitted=0,.unitByClause=0,.unitByLit=0,.depthCount=0});
        }
        #endif
    }

    #ifndef FPGA_HW
    if(!doBackTrack){
        stopSending.write(false);
    }
    if(toDecide.empty()){
        flushSignal = true;
    }
    toDecide.write((bcpPacket){.literalUnitted=0,.unitByClause=0,.unitByLit=0,.depthCount=0});
    #else
    if(doBackTrack){
        toDecide.write((bcpPacket){.literalUnitted=0,.unitByClause=0,.unitByLit=0,.depthCount=0});
    }
    #endif
    if(!finishCount){
        FLUSH_COUNT: while(true){
            #pragma HLS loop_tripcount min=4 max=4
            int flush = duplicateCountStream.read();
            if(flush == -1){
                break;
            }
        }
    }   
}

void bcp_discover_dataflow_wrapper(clsState clsStates[_FPGA_CLS_STATES_PARTITION][_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION],
    lit answerStack[_FPGA_MAX_LITERALS], literalMetaData lmd[_FPGA_MAX_LITERALS], literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    cls unitByCls[_FPGA_MAX_LITERALS],
    const ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16],
    unsigned int& answerStackHeight,
    myStream<cls,64,7>& unsatClauses, unsigned int& fixedDecisionStackHeight, lit& literalCommit, bool& doBackTrack,
    const lit topLiteral, const flippedLiteral litToCheck, const unsigned int fixedDecisionStackHeightCopy,
    const unsigned int decisionLevel, const bool useFlipped, const bool firstIteration, const unsigned int LITERAL_PAGE_SIZE, const ap_uint<1> POSITIVE_LIT_PHASE_VAL,
    ap_uint<64> litStoreAccessStats[4], volatile uint64_t store[2], hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput,
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream){

    #pragma HLS inline off
    
    store[0]++;
    store[1]++;

    hls::stream<colorAssignment> toColorStream("colorStream");
    #pragma HLS stream variable=toColorStream depth=MAX_STREAM_DEPTH
    
    hls::stream<colorValue> toStateUpdater[_FPGA_CLS_STATES_PARTITION/2];
    toStateUpdater[0].set_name("Update1");
    toStateUpdater[1].set_name("Update2");
    toStateUpdater[2].set_name("Update3");
    toStateUpdater[3].set_name("Update4");

    #pragma HLS stream variable=toStateUpdater depth=2
    #pragma HLS array_partition variable=toStateUpdater complete

    hls::stream<clsStateControlPacket> toControlSinkMUX[_FPGA_CLS_STATES_PARTITION/2];
    toControlSinkMUX[0].set_name("sinkMUX1");
    toControlSinkMUX[1].set_name("sinkMUX2");
    toControlSinkMUX[2].set_name("sinkMUX3");
    toControlSinkMUX[3].set_name("sinkMUX4");

    #pragma HLS stream variable=toControlSinkMUX depth=2
    #pragma HLS array_partition variable=toControlSinkMUX complete

    hls::stream<clsStateControlPacket> toControlSink("controllerStream");
    #pragma HLS stream variable=toControlSink depth=4

    hls::stream<int> duplicateCountStream("countStream");
    #pragma HLS stream variable=duplicateCountStream depth=8

    hls::stream<bool> stopSending("earlyStopStream");
    #pragma HLS stream variable=stopSending depth=2

    hls::stream<bcpPacket> toDecide("updateLMDStream");
    #pragma HLS stream variable=toDecide depth=2

    bool skipCPUOnce = false;
    bool flushSignal = false;
    bool firstIterationCopy = firstIteration;

    #pragma HLS dataflow

    #ifndef FPGA_HW
    stopSending.write(false);
    while(true){
    if(doBackTrack){
        flushSignal = true;
    }
    #endif

    discover(toColorStream, toDecide,
        duplicateCountStream, clauseStoreOutputStream,
        litToCheck, answerStack, lmd, lmmd, unitByCls,
        answerStackHeight, decisionLevel, fixedDecisionStackHeight, topLiteral,
        useFlipped, POSITIVE_LIT_PHASE_VAL, skipCPUOnce);

    colorStream(toStateUpdater, toColorStream, &stopSending, litStore, LITERAL_PAGE_SIZE, &literalCommit, 0, litStoreAccessStats);

    updateStatesForward(toControlSinkMUX[0], toStateUpdater[0], clsStates[0], clsStates[1], 0);
    updateStatesForward(toControlSinkMUX[1], toStateUpdater[1], clsStates[2], clsStates[3], 2);
    updateStatesForward(toControlSinkMUX[2], toStateUpdater[2], clsStates[4], clsStates[5], 4);
    updateStatesForward(toControlSinkMUX[3], toStateUpdater[3], clsStates[6], clsStates[7], 6);

    muxControlSink(toControlSink, toControlSinkMUX);

    controlSink(toDecide, toControlSink,
        duplicateCountStream,
        stopSending, clauseStoreInputStream1, unsatClauses, doBackTrack,
        decisionLevel, fixedDecisionStackHeightCopy, firstIterationCopy,
        flushSignal);

    #ifndef FPGA_HW
    firstIterationCopy = true;
    if(flushSignal){
        
        if(!stopSending.empty()){
            stopSending.read();
        }
        while(!toDecide.empty()){
            toDecide.read();
        }
        break;
    }
    #endif

    #ifndef FPGA_HW
    }
    #endif
    
}
