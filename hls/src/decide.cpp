#include "decide.h"

void checkUndecided(hls::stream<colorAssignment>& toCommitStream,
    lit answerStack[_FPGA_MAX_LITERALS], literalMetaData lmd[_FPGA_MAX_LITERALS], 
    literalMinimizeMetaData lmmd[_FPGA_PARALLEL_MINIMIZE][_FPGA_MAX_LITERALS],
    cls unitByCls[_FPGA_MAX_LITERALS],
    unsigned int& answerStackHeight, unsigned int& duplicateCount, const lit topLiteral,
    const unsigned int fixedDecisionStackHeight, const unsigned int decisionLevel, const ap_uint<1> POSITIVE_LIT_PHASE_VAL){
    #pragma HLS inline

    if(decisionLevel == 0){
        WRITE_FIXED_DECISION: for(unsigned int i = answerStackHeight; i < fixedDecisionStackHeight; i++){
            #pragma HLS loop_tripcount min=256 max=256
            #pragma HLS pipeline
            #pragma HLS dependence variable=lmd inter false
            #pragma HLS dependence variable=lmmd inter false

            lit getUndecided = answerStack[i];
            literalMetaData getLmd = lmd[abs(getUndecided)-1];
            literalMinimizeMetaData getLmmd = lmmd[0][abs(getUndecided)-1];

            LMD_UNIT_BY_LIT(getLmd.compactlmd) = 0;
            LMD_INSERT_LVL(getLmd.compactlmd) = answerStackHeight;
            LMD_DEC_LVL(getLmd.compactlmd) = decisionLevel;
            LMD_IS_IN_STACK(getLmd.compactlmd) = true;

            LMMD_IS_DECIDE(getLmmd.compactlmmd) = true;
            LMMD_IS_IN_FIX_STACK(getLmmd.compactlmmd) = true;

            if(getUndecided > 0){
                LMD_PHASE(getLmd.compactlmd) = POSITIVE_LIT_PHASE_VAL;
            }else{
                LMD_PHASE(getLmd.compactlmd) = !POSITIVE_LIT_PHASE_VAL;
            }

            lmd[abs(getUndecided)-1] = getLmd;
            for(unsigned int j = 0; j < _FPGA_PARALLEL_MINIMIZE; j++){
                lmmd[j][abs(getUndecided)-1] = getLmmd;
            }

            ap_uint<1> selectSide = 0;
            if(getUndecided > 0){
                selectSide = 1;   
            }

            unsigned int numElements = (unsigned int)LMD_NUM_ELE(getLmd.compactlmd,selectSide);
            toCommitStream.write((colorAssignment){.addressStart=(unsigned int)LMD_ADDR_START(getLmd.compactlmd,selectSide),
                .numElements=numElements,.depthCount=0,.literal=getUndecided,.eos=false});
            if(numElements == 0){
                duplicateCount++;
            }

            answerStackHeight++;
        }
    }else{
        lit getUndecided = topLiteral;
        lit absLit = abs(getUndecided)-1;
        literalMetaData getLmd = lmd[absLit];
        literalMinimizeMetaData getLmmd = lmmd[0][absLit];
    
        LMD_UNIT_BY_LIT(getLmd.compactlmd) = 0;
        LMD_INSERT_LVL(getLmd.compactlmd) = answerStackHeight;
        LMD_DEC_LVL(getLmd.compactlmd) = decisionLevel;
        LMD_IS_IN_STACK(getLmd.compactlmd) = true;
        
        LMMD_IS_DECIDE(getLmmd.compactlmmd) = true;
        LMMD_IS_IN_FIX_STACK(getLmmd.compactlmmd) = false;

        if(POSITIVE_LIT_PHASE_VAL == 0){
            if(LMD_PHASE(getLmd.compactlmd) == 1){
                getUndecided = -getUndecided;
            }
        }else{
            if(LMD_PHASE(getLmd.compactlmd) == 0){
                getUndecided = -getUndecided;
            }
        }

        
        answerStack[answerStackHeight] = getUndecided;
        
        lmd[absLit] = getLmd;
        for(unsigned int j = 0; j < _FPGA_PARALLEL_MINIMIZE; j++){
            #pragma HLS unroll
            lmmd[j][absLit] = getLmmd;
        }

        ap_uint<1> selectSide = 0;
        if(getUndecided > 0){
            selectSide = 1;   
        }

        unsigned int numElements = (unsigned int)LMD_NUM_ELE(getLmd.compactlmd,selectSide);
        toCommitStream.write((colorAssignment){.addressStart=(unsigned int)LMD_ADDR_START(getLmd.compactlmd,selectSide),
            .numElements=numElements,.depthCount=0,.literal=getUndecided,.eos=false});
        if(numElements == 0){
            duplicateCount++;
        }
        answerStackHeight++;
    }
}


