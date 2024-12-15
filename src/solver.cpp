#include "fpga_solver.h"
#include "discover.h"
#include "learn.h"
#include "backtrack.h"
#include "copy_in.h"

void sendTime(hls::stream<ap_axiu<64,0,0,0>>& timerValueStream, hls::stream<ap_axiu<1,0,0,0>>& conditionStream, 
    const unsigned int code, volatile uint64_t* store){
    #pragma HLS inline off
    IO_WAIT:{
        ap_axiu<1,0,0,0> pktTimer;
        ap_axiu<64,0,0,0> value;

        pktTimer.data = code;
        conditionStream.write(pktTimer);

        if(code == 1){
            value = timerValueStream.read();
            *store = value.data;
        }
    }
}

void copyStats(ap_uint<64>* learnedStats, ap_uint<64>* longestClause, ap_uint<64>* litStoreAccessStats, ap_uint<64>* cycleCounter, int* miscCounters){
    #pragma HLS inline off

    int copyMe[48];
    copyMe[0] = learnedStats[0].range(31,0);
    copyMe[1] = learnedStats[0].range(63,32);
    copyMe[2] = learnedStats[1].range(31,0);
    copyMe[3] = learnedStats[1].range(63,32);
    copyMe[4] = learnedStats[2].range(31,0);
    copyMe[5] = learnedStats[2].range(63,32);
    copyMe[6] = learnedStats[3].range(31,0);
    copyMe[7] = learnedStats[3].range(63,32);
    copyMe[8] = learnedStats[4].range(31,0);
    copyMe[9] = learnedStats[4].range(63,32);

    copyMe[10] = longestClause[0].range(31,0);
    copyMe[11] = longestClause[0].range(63,32);
    copyMe[12] = longestClause[1].range(31,0);
    copyMe[13] = longestClause[1].range(63,32);
    copyMe[14] = checkCnt;

    copyMe[15] = litStoreAccessStats[0].range(31,0);
    copyMe[16] = litStoreAccessStats[0].range(63,32);
    copyMe[17] = litStoreAccessStats[1].range(31,0);
    copyMe[18] = litStoreAccessStats[1].range(63,32);

    copyMe[19] = litStoreAccessStats[2].range(31,0);
    copyMe[20] = litStoreAccessStats[2].range(63,32);
    copyMe[21] = litStoreAccessStats[3].range(31,0);
    copyMe[22] = litStoreAccessStats[3].range(63,32);

    copyMe[23] = cycleCounter[0].range(31,0);
    copyMe[24] = cycleCounter[0].range(63,32);

    copyMe[25] = cycleCounter[1].range(31,0);
    copyMe[26] = cycleCounter[1].range(63,32);

    copyMe[27] = cycleCounter[2].range(31,0);
    copyMe[28] = cycleCounter[2].range(63,32);

    copyMe[29] = cycleCounter[3].range(31,0);
    copyMe[30] = cycleCounter[3].range(63,32);

    copyMe[31] = cycleCounter[4].range(31,0);
    copyMe[32] = cycleCounter[4].range(63,32);

    copyMe[33] = cycleCounter[5].range(31,0);
    copyMe[34] = cycleCounter[5].range(63,32);

    copyMe[35] = cycleCounter[6].range(31,0);
    copyMe[36] = cycleCounter[6].range(63,32);

    copyMe[37] = cycleCounter[7].range(31,0);
    copyMe[38] = cycleCounter[7].range(63,32);

    copyMe[39] = cycleCounter[8].range(31,0);
    copyMe[40] = cycleCounter[8].range(63,32);

    copyMe[41] = overhead;
    copyMe[42] = splitResidualCnt;

    memcpy(miscCounters+7, copyMe, sizeof(int) * 43);
}

extern "C"{
void solver(clsStatePCIE* clsStates, ap_int<512>* litStore, lit* answerStack,
    literalMetaDataPCIE* lmd, int* miscCounters,
    hls::stream<ap_axiu<32,0,0,0>>& pqHandlerValue, hls::stream<ap_axiu<32,0,0,0>>& pqHandlerInput,
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1, hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream2,
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream1, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream2,
     hls::stream<ap_axiu<32,0,0,0>>& locationOutputStream,
    hls::stream<ap_axiu<32,0,0,0>>& restartValueStream, hls::stream<ap_axiu<1,0,0,0>>& stopStream,
    hls::stream<ap_axiu<64,0,0,0>>& timerValueStream, hls::stream<ap_axiu<1,0,0,0>>& conditionStream,
    hls::stream<ap_axiu<96,0,0,0>>& messageStream){

    #pragma HLS INTERFACE m_axi port=clsStates offset=slave bundle=gmem5 latency=40
    #pragma HLS INTERFACE m_axi port=litStore offset=slave bundle=gmemLitStore1 latency=40
    #pragma HLS INTERFACE m_axi port=answerStack offset=slave bundle=gmem7 latency=40
    #pragma HLS INTERFACE m_axi port=lmd offset=slave bundle=gmem10 latency=40
    #pragma HLS INTERFACE m_axi port=miscCounters offset=slave bundle=gmem7 latency=40

    #pragma HLS INTERFACE axis port=pqHandlerValue
    #pragma HLS INTERFACE axis port=pqHandlerInput
    #pragma HLS INTERFACE axis port=restartValueStream
    #pragma HLS INTERFACE axis port=stopStream
    #pragma HLS INTERFACE axis port=timerValueStream
    #pragma HLS INTERFACE axis port=conditionStream
    #pragma HLS INTERFACE axis port=messageStream

    #pragma HLS INTERFACE s_axilite port=clsStates
	#pragma HLS INTERFACE s_axilite port=litStore
    #pragma HLS INTERFACE s_axilite port=answerStack 
	#pragma HLS INTERFACE s_axilite port=lmd
	#pragma HLS INTERFACE s_axilite port=miscCounters

	#pragma HLS INTERFACE s_axilite port=return

    unsigned int literalElements = miscCounters[0];
    unsigned int numClauses = miscCounters[1];
    const unsigned int NUM_LITERALS = miscCounters[2];
    unsigned int fixedDecisionStackHeight = miscCounters[3];
    const ap_uint<1> POSITIVE_LIT_PHASE_VAL = miscCounters[4];
    const unsigned int MAX_LITERAL_ELEMENTS = miscCounters[5];
    const unsigned int LITERAL_PAGE_SIZE = miscCounters[6];
    const unsigned int RESET_MULTIPLIER = miscCounters[7];

    lit mAnswerStack[_FPGA_MAX_LITERALS];
    #pragma HLS bind_storage variable=mAnswerStack type=RAM_S2P impl=BRAM latency=1

    ap_uint<512> mLitStore[_FPGA_MAX_LITERAL_ELEMENTS/16];
    #pragma HLS bind_storage variable=mLitStore type=RAM_S2P impl=URAM latency=1

    literalMetaData mlmd[_FPGA_MAX_LITERALS];
    #pragma HLS aggregate variable=mlmd compact=auto
    #pragma HLS bind_storage variable=mlmd type=RAM_S2P impl=URAM latency=1

    cls unitByCls[_FPGA_MAX_LITERALS];
    #pragma HLS bind_storage variable=unitByCls type=RAM_T2P impl=auto latency=1

    literalMinimizeMetaData mlmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS];
    #pragma HLS array_partition variable=mlmmd dim=1 complete

    mmuStream<unsigned int, _MAX_PAGES_LIT_STORE_> freeLitPageAddresses(literalElements,_FPGA_MAX_LITERAL_ELEMENTS,LITERAL_PAGE_SIZE);
    #pragma HLS bind_storage variable=freeLitPageAddresses.array type=RAM_S2P impl=URAM

    lit literalCommit;

    clsState mClsStates[_FPGA_CLS_STATES_PARTITION][_FPGA_MAX_CLAUSES/_FPGA_CLS_STATES_PARTITION];
    #pragma HLS aggregate variable=mClsStates compact=auto
    #pragma HLS array_partition variable=mClsStates dim=1 complete
    #pragma HLS bind_storage variable=mClsStates type=RAM_S2P impl=URAM latency=2

    ap_uint<64> cycleCounter[9] = {0,0,0,0,0,0,0,0,0};
    #pragma HLS array_partition variable=cycleCounter dim=0 complete
    volatile uint64_t store[2];

    sendTime(timerValueStream, conditionStream, 1, &store[0]);

    copy_in_dataflow_wrapper(mClsStates, mLitStore, mlmd, mlmmd, 
        mAnswerStack,
        clsStates, litStore, lmd, answerStack,
        literalElements, numClauses, NUM_LITERALS, store);

    sendTime(timerValueStream, conditionStream, 1, &store[1]);
    cycleCounter[0] += store[1]-store[0];
    
    overhead = 0;
    splitResidualCnt = 0;
    checkCnt = 0;

    bool doBackTrack = false;
    unsigned int answerStackHeight = 0;
    int decisionLevel = (fixedDecisionStackHeight == 0) ? 1 : 0;
    
    myStream<cls,64,7> unsatClauses;
    unsatClauses.head = 0;
    unsatClauses.tail = 0;

    double multiplier = 1.0;

    flippedLiteral litToCheck;
    bool useFlipped = false;
    bool firstIteration = false;
    unsigned int totalCount = 0;
    unsigned int decideCount = 0;
    unsigned int retryCount = 0;
    unsigned int backtrackCount = 0;
    unsigned int resetCount = 0;

    unsigned int restartCount = 1;
    unsigned int limit = RESET_MULTIPLIER;
    unsigned int limitCount = 0;

    ap_uint<64> learnedStats[5] = {0,0,0,0,0};
    #pragma HLS array_partition variable=learnedStats dim=0 complete
    ap_uint<64> longestClause[2] = {0,0};
    #pragma HLS array_partition variable=longestClause dim=0 complete
    ap_uint<64> litStoreAccessStats[4] = {0,0,0,0};
    #pragma HLS array_partition variable=litStoreAccessStats dim=0 complete

    ap_axiu<96,0,0,0> messageValue;

    SOLVE_ITERATION: while(true){
        ap_axiu<32,0,0,0> sendPQHandler;

        #pragma HLS loop_tripcount min=1024 max=1024
        totalCount++;

        volatile uint64_t store[2];

        messageValue.data.range(95,64) = 1;
        messageValue.data.range(63,32) = totalCount;
        messageValue.data.range(31,0) = 0;
        messageStream.write(messageValue);

        if(!doBackTrack){
            if(useFlipped){
                retryCount++;
            }else{
                decideCount++;
            }

            messageValue.data.range(95,64) = 2;
            messageValue.data.range(63,32) = decideCount+retryCount;
            messageValue.data.range(31,0) = 1;
            messageStream.write(messageValue);

            lit topLiteral;

            if(!useFlipped && decisionLevel != 0){
                sendTime(timerValueStream, conditionStream, 1, &store[0]);

                
                literalMetaData getLmd;
                LMD_IS_IN_STACK(getLmd.compactlmd) = true;
                
                FIND_TOP: while(true){
                    #pragma HLS loop_tripcount min=16 max=16
                    if(!LMD_IS_IN_STACK(getLmd.compactlmd)){
                        break;
                    }
                    sendPQHandler.data = pq::GET_UNDECIDED;
                    pqHandlerInput.write(sendPQHandler);
                                
                    ap_wait();
                    
                    ap_axiu<32,0,0,0> getPQHandler = pqHandlerValue.read();

                    topLiteral = getPQHandler.data;
                    getLmd = mlmd[topLiteral-1];     
                }
                sendPQHandler.data = pq::EXIT;
                pqHandlerInput.write(sendPQHandler);
                
                sendTime(timerValueStream, conditionStream, 1, &store[1]);
                cycleCounter[1] += store[1]-store[0];
            }
            
            sendTime(timerValueStream, conditionStream, 1, &store[0]);

            ap_axiu<96,0,0,0> sendClauseInputCommand;
            sendClauseInputCommand.data.range(31,0) = 0;
            sendClauseInputCommand.data.range(95,64) = csh::SEND_LEN_BCP;
            clauseStoreInputStream1.write(sendClauseInputCommand);

            bcp_discover_dataflow_wrapper(mClsStates,
                mAnswerStack, mlmd, mlmmd, unitByCls,
                mLitStore,
                answerStackHeight, unsatClauses, fixedDecisionStackHeight, literalCommit, doBackTrack,
                topLiteral, litToCheck, fixedDecisionStackHeight, decisionLevel, useFlipped, firstIteration, 
                LITERAL_PAGE_SIZE, POSITIVE_LIT_PHASE_VAL,
                litStoreAccessStats, store, pqHandlerInput, clauseStoreInputStream1, clauseStoreOutputStream1);
            
            sendClauseInputCommand.data.range(31,0) = csh::EXIT;
            sendClauseInputCommand.data.range(95,64) = csh::SEND_LEN_BCP;
            clauseStoreInputStream1.write(sendClauseInputCommand);

            mlmd[decisionLevel].decisionLevelStackEnd = answerStackHeight;

            decisionLevel++;
            if(doBackTrack){
                decisionLevel--;
            }

            sendTime(timerValueStream, conditionStream, 1, &store[1]);
            cycleCounter[2] += store[1]-store[0];

            //Fixed decision stack lead to conflict so its unsolvable
            if(decisionLevel == 0){
                miscCounters[0] = totalCount;
                miscCounters[1] = decideCount;
                miscCounters[2] = retryCount;
                miscCounters[3] = backtrackCount;
                miscCounters[4] = resetCount;
                miscCounters[5] = answerStackHeight;
                miscCounters[6] = 0;

                copyStats(learnedStats, longestClause, litStoreAccessStats, cycleCounter, miscCounters);

                ap_axiu<1,0,0,0> pkt;
                pkt.data = 1;
                stopStream.write(pkt);

                sendPQHandler.data = pq::EXIT;
                pqHandlerInput.write(sendPQHandler);

                FLUSH_RESTART_1: while(true){
                    #pragma HLS loop_tripcount min=16 max=16
                    if(restartValueStream.read().data == 0){
                        break;
                    }
                }

                sendTime(timerValueStream, conditionStream, 0, nullptr);
                messageValue.data.range(95,64) = 0;
                messageValue.data.range(63,32) = 0;
                messageValue.data.range(31,0) = -1;
                messageStream.write(messageValue);

                ap_axiu<96,0,0,0> sendClauseInputCommand;
                sendClauseInputCommand.data.range(95,64) = csh::EXIT;

                clauseStoreInputStream1.write(sendClauseInputCommand);

                return;
            }

            if(answerStackHeight == NUM_LITERALS && !doBackTrack){
                miscCounters[0] = totalCount;
                miscCounters[1] = decideCount;
                miscCounters[2] = retryCount;
                miscCounters[3] = backtrackCount;
                miscCounters[4] = resetCount;
                miscCounters[5] = answerStackHeight;
                miscCounters[6] = 1;

                copyStats(learnedStats, longestClause, litStoreAccessStats, cycleCounter, miscCounters);

                COPY_OUT: for(unsigned int i = 0; i < NUM_LITERALS; i++){
                    #pragma HLS loop_tripcount min=1024 max=1024
                    answerStack[i] = mAnswerStack[i];
                }

                ap_axiu<1,0,0,0> pkt;
                pkt.data = 1;
                stopStream.write(pkt);

                sendPQHandler.data = pq::EXIT;
                pqHandlerInput.write(sendPQHandler);

                FLUSH_RESTART_2: while(true){
                    #pragma HLS loop_tripcount min=16 max=16
                    if(restartValueStream.read().data == 0){
                        break;
                    }
                }

                sendTime(timerValueStream, conditionStream, 0, nullptr);
                messageValue.data.range(95,64) = 0;
                messageValue.data.range(63,32) = 0;
                messageValue.data.range(31,0) = -1;
                messageStream.write(messageValue);

                ap_axiu<96,0,0,0> sendClauseInputCommand;
                sendClauseInputCommand.data.range(95,64) = csh::EXIT;

                clauseStoreInputStream1.write(sendClauseInputCommand);

                return;
            }
            firstIteration = true;
            useFlipped = false;
        }else{
            backtrackCount++;
        
            bool resetAll = false;
            limitCount++;
            if(limit == limitCount){
                limitCount = 0;
                limit = RESET_MULTIPLIER * restartValueStream.read().data;
                resetAll = true;
                resetCount++;
            }

            messageValue.data.range(95,64) = 3;
            messageValue.data.range(63,32) = backtrackCount;
            messageValue.data.range(31,0) = 2;
            messageStream.write(messageValue);

            int error = 0;
            lit insertPropagate[2] = {0,0};
            int givenClsID;

            learnClause(mClsStates,
                mLitStore,
                mlmd, mlmmd, insertPropagate, freeLitPageAddresses,
                decisionLevel, givenClsID,
                mAnswerStack, unitByCls, literalCommit, answerStackHeight,
                POSITIVE_LIT_PHASE_VAL, resetAll, unsatClauses, 
                NUM_LITERALS, MAX_LITERAL_ELEMENTS, LITERAL_PAGE_SIZE,
                learnedStats, litStoreAccessStats, longestClause, error,
                clauseStoreInputStream1, clauseStoreInputStream2, clauseStoreOutputStream1, clauseStoreOutputStream2, 
                pqHandlerInput, pqHandlerValue, timerValueStream, conditionStream, cycleCounter);
            

            if(error < 0){
                miscCounters[0] = totalCount;
                miscCounters[1] = decideCount;
                miscCounters[2] = retryCount;
                miscCounters[3] = backtrackCount;
                miscCounters[4] = resetCount;
                miscCounters[5] = answerStackHeight;
                
                copyStats(learnedStats, longestClause, litStoreAccessStats, cycleCounter, miscCounters);

                messageValue.data.range(95,64) = 0;
                messageValue.data.range(63,32) = 0;
                messageValue.data.range(31,0) = error;
                messageStream.write(messageValue);

                ap_axiu<1,0,0,0> pkt;
                pkt.data = 1;
                stopStream.write(pkt);

                sendPQHandler.data = pq::EXIT;
                pqHandlerInput.write(sendPQHandler);

                FLUSH_RESTART_3: while(true){
                    #pragma HLS loop_tripcount min=16 max=16
                    if(restartValueStream.read().data == 0){
                        break;
                    }
                }
                sendTime(timerValueStream, conditionStream, 0, nullptr);

                ap_axiu<96,0,0,0> sendClauseInputCommand;
                sendClauseInputCommand.data.range(95,64) = csh::EXIT;

                clauseStoreInputStream1.write(sendClauseInputCommand);

                return;
            }

            messageValue.data.range(95,64) = 3;
            messageValue.data.range(63,32) = backtrackCount;
            messageValue.data.range(31,0) = 4;
            messageStream.write(messageValue);

            if(resetAll){
                sendTime(timerValueStream, conditionStream, 1, &store[0]);

                decisionLevel = 0;

                ap_axiu<96,0,0,0> sendClauseInputCommand;
                sendClauseInputCommand.data.range(95,64) = csh::DELETE;

                clauseStoreInputStream1.write(sendClauseInputCommand);

                deleteTransposedClauses(mLitStore,
                    mlmd, freeLitPageAddresses, LITERAL_PAGE_SIZE, 
                    clauseStoreInputStream1, clauseStoreOutputStream1, locationOutputStream);

                sendTime(timerValueStream, conditionStream, 1, &store[1]);
                cycleCounter[8] += store[1]-store[0];
            }

            messageValue.data.range(95,64) = 3;
            messageValue.data.range(63,32) = backtrackCount;
            messageValue.data.range(31,0) = 5;
            messageStream.write(messageValue);
            
            unsatClauses.head = 0;
            unsatClauses.tail = 0;
            
            doBackTrack = false;
            if(decisionLevel == 0){
                if(insertPropagate[0] != 0){
                    mAnswerStack[answerStackHeight] = insertPropagate[0];
                    fixedDecisionStackHeight++;
                }else{
                    decisionLevel = 1;
                }
                if(fixedDecisionStackHeight == 0){
                    decisionLevel = 1;
                }
                useFlipped = false;
            }else{
                useFlipped = true;

                litToCheck.literal = insertPropagate[1];
                litToCheck.lmd = mlmd[abs(litToCheck.literal)-1];
                litToCheck.lmmd = mlmmd[0][abs(litToCheck.literal)-1];

                LMD_INSERT_LVL(litToCheck.lmd.compactlmd) = answerStackHeight;
                LMD_DEC_LVL(litToCheck.lmd.compactlmd) = decisionLevel;
                LMD_IS_IN_STACK(litToCheck.lmd.compactlmd) = true;
                LMD_UNIT_BY_LIT(litToCheck.lmd.compactlmd) = 0;

                unitByCls[abs(litToCheck.literal)-1] = givenClsID+1;
                LMMD_IS_DECIDE(litToCheck.lmmd.compactlmmd) = false;

                if(litToCheck.literal > 0){
                    LMD_PHASE(litToCheck.lmd.compactlmd) = POSITIVE_LIT_PHASE_VAL;
                }else{
                    LMD_PHASE(litToCheck.lmd.compactlmd) = !POSITIVE_LIT_PHASE_VAL;
                }
            }
        }
    }

	
}
}
