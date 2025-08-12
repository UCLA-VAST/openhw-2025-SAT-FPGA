#include "priority_queue_functions.h"

void loadPositioning(pqPosition mPositioning[_FPGA_MAX_LITERALS], pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], unsigned int NUM_LITERALS){
    #pragma HLS inline off


    GET_POSITION_3: for(unsigned int i = 0; i < NUM_LITERALS; i++){
        #pragma HLS loop_tripcount min=1024 max=1024

        mPriorityQueue[0][i].literal = i+1;
        mPriorityQueue[1][i].literal = i+1;
        mPriorityQueue[0][i].score = 0;
        mPriorityQueue[1][i].score = 0;

        mPositioning[i].pos = i;
    }
}

inline int comp_fp64(double op1, double op2){
    // Credit to Linghao Song
    // return +1; op1 > op2
    // return -1; op1 < op2
    // return 0;
    //#pragma HLS inline
    
    ap_uint<64> u_op1 = *((ap_uint<64>*)(&op1));
    ap_uint<64> u_op2 = *((ap_uint<64>*)(&op2));

    if(u_op1.range(63,0) > u_op2.range(63,0)){
        return 1;
    }else if(u_op1.range(63,0) < u_op2.range(63,0)){
        return -1;
    }

    //sign
    /*if ((u_op1[63] == 0) & (u_op2[63] == 1)) {
        // op1 > 0.0, and op2 < 0.0
        return 1;
    }
    else if ((u_op1[63] == 1) & (u_op2[63] == 0)) {
        // op1 < 0.0, and op2 > 0.0
        return -1;
    }
    
    // exponent
    else if (
        ((u_op1[63] == 0) & (u_op1(62, 52) > u_op2(62, 52))) // both sign = 0, only need to check one sign
        | ((u_op1[63] == 1) & (u_op1(62, 52) < u_op2(62, 52))) // both sign = 1
    ) {
        // op1 > op2
        return 1;
    }
    else if (
        ((u_op1[63] == 0) & (u_op1(62, 52) < u_op2(62, 52))) // both sign = 0, only need to check one sign
        | ((u_op1[63] == 1) & (u_op1(62, 52) > u_op2(62, 52))) // both sign = 1
    ) {
        // op1 < op2
        return -1;
    }

    // fraction
    else if (
        ((u_op1[63] == 0) & (u_op1(51, 0) > u_op2(51, 0))) // both sign = 0, only need to check one sign
        | ((u_op1[63] == 1) & (u_op1(51, 0) < u_op2(51, 0))) // both sign = 1
    ) {
        // op1 > op2
        return 1;
    }
    else if (
        ((u_op1[63] == 0) & (u_op1(51, 0) < u_op2(51, 0))) // both sign = 0, only need to check one sign
        | ((u_op1[63] == 1) & (u_op1(51, 0) > u_op2(51, 0))) // both sign = 1
    ) {
        // op1 < op2
        return -1;
    }*/
    return 0;

    
}

void swapLower(pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], 
    const pqData initialpqData, const unsigned int initialPosition, const unsigned int remainingLiterals){
    #pragma HLS inline

    unsigned int getPosition = initialPosition;
    pqData toMoveLower = initialpqData;
    unsigned int savePosition = getPosition;

    pqData rightChild;
    pqData leftChild;

    bool exit = false;

    SWAP_LOWER: while(true){
        #pragma HLS loop_tripcount min=32 max=32
        #pragma HLS dependence variable=mPriorityQueue inter false
        #pragma HLS pipeline II=2

        if(exit){
            break;
        }
        
        bool childExists[2] = {false, false};
        #pragma HLS array_partition variable=childExists dim=0 complete

        int newPosition = 2*getPosition+1;
        leftChild = mPriorityQueue[0][newPosition];
        if(newPosition >= remainingLiterals){
            leftChild.score = 0;
        }
        
        newPosition++;
        rightChild = mPriorityQueue[1][newPosition];
        if(newPosition >= remainingLiterals){
            rightChild.score = 0;
        }

        ap_uint<64> u_op1 = *((ap_uint<64>*)(&(leftChild.score)));
        ap_uint<64> u_op2 = *((ap_uint<64>*)(&(toMoveLower.score)));
        ap_uint<64> u_op3 = *((ap_uint<64>*)(&(leftChild.score)));
        ap_uint<64> u_op4 = *((ap_uint<64>*)(&(rightChild.score)));
        ap_uint<64> u_op5 = *((ap_uint<64>*)(&(rightChild.score)));
        ap_uint<64> u_op6 = *((ap_uint<64>*)(&(toMoveLower.score)));

        bool expr1 = (u_op1.range(62,0) > u_op2.range(62,0));
        bool expr2 = (u_op3.range(62,0) >= u_op4.range(62,0));
        bool expr3 = (u_op5.range(62,0) > u_op6.range(62,0));

        pqData use = leftChild;
        savePosition = getPosition;
        getPosition = 2*getPosition+1;

        if(expr3 && !expr2){
            use = rightChild;
            getPosition++;
        }else if(!expr1 && !expr3){
            use = toMoveLower;
            exit = true;
        }

        mPriorityQueue[0][savePosition] = use;
        mPriorityQueue[1][savePosition] = use;
        mPositioning[use.literal-1].pos = savePosition;
    }

    mPositioning[toMoveLower.literal-1].pos = savePosition;
    mPriorityQueue[0][savePosition] = toMoveLower;
    mPriorityQueue[1][savePosition] = toMoveLower;
}

void swapHigher(pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], 
    const pqData initialpqData, const unsigned int initialPosition){
    #pragma HLS inline

    unsigned int getPosition = initialPosition;
    pqData toMoveHigher = initialpqData;

    int savePosition = getPosition;
    bool exit = false;

    SWAP_HIGHER: while(true){
        #pragma HLS loop_tripcount min=32 max=32
        #pragma HLS dependence variable=mPriorityQueue inter false
        #pragma HLS pipeline II=1

        int newPosition = ((int)getPosition-1)/2;
        if(exit || getPosition == 0){
            break;
        }

        pqData parent = mPriorityQueue[0][newPosition];
        int cmp = comp_fp64(parent.score,toMoveHigher.score);
        
        if(cmp == -1){

            mPriorityQueue[0][getPosition] = reg(parent);
            mPriorityQueue[1][getPosition] = reg(parent);
            mPositioning[parent.literal-1].pos = reg(getPosition);

            savePosition = newPosition;
        }else{   
            exit = true;
        }
        getPosition = newPosition;
    }

    mPositioning[toMoveHigher.literal-1].pos = savePosition;
    mPriorityQueue[0][savePosition] = toMoveHigher;
    mPriorityQueue[1][savePosition] = toMoveHigher;
}

void hideElement(hls::stream<lit>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], unsigned int& remainingLiterals){
    #pragma HLS inline off

    HIDE_LOOP: while(true){
        #pragma HLS loop_tripcount min=32 max=32

        lit getLit;
        if(input.read_nb(getLit)){
            if(getLit == pq::EXIT){
                break;
            }

            pqPosition swapPosition = mPositioning[getLit-1];
            unsigned int getRemaining = remainingLiterals-1;
        
            pqData swap = mPriorityQueue[0][swapPosition.pos];
            pqData last = mPriorityQueue[1][getRemaining];

            mPriorityQueue[0][swapPosition.pos] = last;
            mPriorityQueue[1][swapPosition.pos] = last;
            mPositioning[last.literal-1].pos = swapPosition.pos;

            mPriorityQueue[0][getRemaining] = swap;
            mPriorityQueue[1][getRemaining] = swap;
            mPositioning[getLit-1].pos = getRemaining;  
            remainingLiterals = getRemaining;
                
            swapLower(mPriorityQueue, mPositioning, last, swapPosition.pos, getRemaining);
        }
    }
}

void unhideElement(hls::stream<lit>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], unsigned int& remainingLiterals){
    #pragma HLS inline off
    
    UNHIDE_LOOP: while(true){
        #pragma HLS loop_tripcount min=32 max=32

        lit getLit;
        if(input.read_nb(getLit)){
        
            if(getLit == pq::EXIT){
                break;
            }

            if(mPositioning[getLit-1].pos < remainingLiterals){
                continue;
            }

            pqPosition swapPosition = mPositioning[getLit-1];
            unsigned int getRemaining = remainingLiterals;
        
            pqData swap = mPriorityQueue[0][swapPosition.pos];
            pqData last = mPriorityQueue[1][getRemaining];

            mPriorityQueue[0][swapPosition.pos] = mPriorityQueue[0][getRemaining];
            mPriorityQueue[1][swapPosition.pos] = mPriorityQueue[1][getRemaining];
            mPositioning[last.literal-1].pos = swapPosition.pos;

            mPriorityQueue[0][getRemaining] = swap;
            mPriorityQueue[1][getRemaining] = swap;
            mPositioning[getLit-1].pos = getRemaining; 

            swapHigher(mPriorityQueue, mPositioning, swap, getRemaining);
            getRemaining++;
            remainingLiterals = getRemaining;
        }
    }
}

void decayEveryElement(hls::stream<lit>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], 
    const unsigned int remainingLiterals, double& multiplier, const double decayFactor, const unsigned int NUM_LITERALS){
    #pragma HLS inline off

    DECAY_LOOP: while(true){
        #pragma HLS loop_tripcount min=32 max=32

        lit getLit;
        if(input.read_nb(getLit)){
            if(getLit == pq::EXIT){
                break;
            }

            pqPosition position = mPositioning[getLit-1];
            pqData getPq = mPriorityQueue[0][position.pos];

            mPriorityQueue[0][position.pos].score += multiplier;
            mPriorityQueue[1][position.pos].score += multiplier;
            getPq.score += multiplier;
        
            if(getPq.score > 1e100){
                ADJUST: for(unsigned int i = 0; i < NUM_LITERALS; i++){
                    #pragma HLS loop_tripcount min=1024 max=1024
                    #pragma HLS dependence variable=mPriorityQueue inter false

                    mPriorityQueue[0][i].score *= 1e-100;
                    mPriorityQueue[1][i].score *= 1e-100;
                }
                multiplier *= 1e-100;
            }

            if(position.pos < remainingLiterals){
                swapHigher(mPriorityQueue, mPositioning, getPq, position.pos);
            }
        }
    }
    multiplier *= 1/decayFactor;
}


void axiStreamBuffer(hls::stream<ap_axiu<32,0,0,0>>& input, hls::stream<lit>& intermediateStream){
    #pragma HLS inline off
    BUFFER_LOOP: while(true){
        #pragma HLS loop_tripcount min=32 max=32
        ap_axiu<32,0,0,0> read = input.read();

        lit getLiteral = read.data;
        intermediateStream.write(getLiteral);

        if(getLiteral == pq::EXIT){
            break;
        }
    }
}
void hide_wrapper(hls::stream<ap_axiu<32,0,0,0>>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], 
    unsigned int& remainingLiterals){
    #pragma HLS inline off
    hls::stream<lit> intermediateStream;
    #pragma HLS stream variable=intermediateStream depth=MAX_STREAM_DEPTH
    #pragma HLS bind_storage variable=intermediateStream type=FIFO impl=URAM

    #pragma HLS dataflow
    axiStreamBuffer(input, intermediateStream);
    hideElement(intermediateStream, mPriorityQueue, mPositioning, remainingLiterals);
}
void unhide_wrapper(hls::stream<ap_axiu<32,0,0,0>>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], 
    unsigned int& remainingLiterals){
    #pragma HLS inline off
    hls::stream<lit> intermediateStream;
    #pragma HLS stream variable=intermediateStream depth=MAX_STREAM_DEPTH
    #pragma HLS bind_storage variable=intermediateStream type=FIFO impl=URAM

    #pragma HLS dataflow
    axiStreamBuffer(input, intermediateStream);
    unhideElement(intermediateStream, mPriorityQueue, mPositioning, remainingLiterals);
}
void unhide_wrapper(hls::stream<ap_axiu<32,0,0,0>>& input, pqData mPriorityQueue[2][_FPGA_MAX_LITERALS], pqPosition mPositioning[_FPGA_MAX_LITERALS], 
    const unsigned int remainingLiterals, double& multiplier, const double decayFactor, const unsigned int NUM_LITERALS){
    #pragma HLS inline off
    hls::stream<lit> intermediateStream;
    #pragma HLS stream variable=intermediateStream depth=_FPGA_MAX_LEARN_ELE

    #pragma HLS dataflow
    axiStreamBuffer(input, intermediateStream);
    decayEveryElement(intermediateStream, mPriorityQueue, mPositioning, remainingLiterals, multiplier, decayFactor, NUM_LITERALS);
}

