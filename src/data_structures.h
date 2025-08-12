#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include "ap_int.h"
#include "fpga_solver.h"

typedef int lit;
typedef int cls;

namespace lh{
    enum lh_codes{EXIT=-1,SEND=1,SAVE=2,UPDATE=3};
}
namespace csh{
    enum csh_codes{EXIT=-1,SEND_LEN=1,SEND_LEN_BCP=2,SEND_CLS=3,SEND_CLS_MIN=4,SAVE=5,BUCKET=6,DELETE=7,MULTI_DEC=8};
}

namespace pq{
    enum pq_codes{EXIT=-1,GET_UNDECIDED=1,UPDATE=2,UNHIDE_ELE=3,CHECK_SCORE=4};
}

namespace solverCode{
    enum sr_codes{UNIT=0,BACKTRACK=1,CLS2=2,EOS_CNT=3};
}

struct colorAssignment{
    unsigned int addressStart;
    unsigned int numElements;
    unsigned int depthCount;
    lit literal;
    bool eos;
};

struct colorValue{
    ap_int<256> clsID;
    lit litID;
    unsigned int depthCount;
    bool didSolve;
    bool clsEos;
    bool streamEos;
};

struct clsState{
    int compressedList;
    int remainingUnassigned;
};

struct clsStatePCIE{
    int compressedList;
    int remainingUnassigned;
};

struct clauseMetaData{
    unsigned int addressStart;
    unsigned int numElements;
};

struct bcpPacket{
    lit literalUnitted;
    unsigned int unitByClause;
    lit unitByLit;
    unsigned int depthCount;
};

struct clsStateControlPacket{
    int eosCount;
    bcpPacket bcp;
    ap_uint<2> pktType;
};

struct minimumStreamTracker{
    unsigned int insertIdx;
    unsigned int accessIdx;
    unsigned int usedCount;
};

template <typename T, unsigned int elements, unsigned int bits>
struct myStream{
    T array[elements];
    ap_uint<bits> head;
    ap_uint<bits> tail;
};

template <typename T, unsigned int elements>
class mmuStream{
    private:
        unsigned int m_insertIdx;
        unsigned int m_accessIdx;
        unsigned int m_usedCount;
        unsigned int m_initialStart;
        unsigned int m_LIMIT_VALUE;
        unsigned int m_INCREMENT_COUNT;
    public:
        T array[elements];
        
        mmuStream(unsigned int initialStart, unsigned int LIMIT_VALUE, unsigned int INCREMENT_COUNT){
            m_insertIdx = 0;
            m_accessIdx = 0;
            m_usedCount = (LIMIT_VALUE-initialStart)/INCREMENT_COUNT;
            m_initialStart = initialStart;
            m_LIMIT_VALUE = LIMIT_VALUE;
            m_INCREMENT_COUNT = INCREMENT_COUNT;
        }
        unsigned int read(){
            if(m_initialStart != m_LIMIT_VALUE){
                unsigned int returnVal = m_initialStart;
                m_initialStart += m_INCREMENT_COUNT;
                m_usedCount--;
                return returnVal;
            }else{
                unsigned int get = array[m_accessIdx];
                m_accessIdx = (m_accessIdx+1)%elements;
                m_usedCount--;
                return get;
            }
        }
        void write(T data){
            array[m_insertIdx] = data;
            m_insertIdx = (m_insertIdx+1)%elements;
            m_usedCount++;
        }
        unsigned int size(){
            return m_usedCount;
        }
        bool empty(){
            if(m_usedCount == 0){
                return true;
            }else{
                return false;
            }
        }
};

struct debugStrct{
    int* ptr;
    int value;
    int index;
    int code;
};
#define LMD_SHORTEST_CLS_LENGTH(compact) (compact.range(385,354))
#define LMD_UNIT_BY_LIT(compact) (compact.range(353,322))
#define LMD_ADDR_START(compact,index) (compact.range(258+32*index+31,258+32*index))
#define LMD_NUM_ELE(compact,index) (compact.range(194+32*index+31,194+32*index))
#define LMD_LATEST_PAGE(compact,index) (compact.range(130+32*index+31,130+32*index))
#define LMD_FREE_SPACE(compact,index) (compact.range(66+32*index+31,66+32*index))
#define LMD_INSERT_LVL(compact) (compact.range(65,34))
#define LMD_DEC_LVL(compact) (compact.range(33,2))
#define LMD_IS_IN_STACK(compact) (compact.range(1,1))
#define LMD_PHASE(compact) (compact.range(0,0))

#define LMMD_MIN_KEEP(compact) (compact.range(3,2))
#define LMMD_IS_DECIDE(compact) (compact.range(1,1))
#define LMMD_IS_IN_FIX_STACK(compact) (compact.range(0,0))

struct literalMinimizeMetaData{
    ap_uint<4> compactlmmd;

    //(3,2) MIN 0-2
    //(1,1) IS_DECIDE
    //(0,0) IS_IN_FIX_STACK
};

struct minimizeStreamData{
    lit litToCheck;
    cls unitByCls;
    literalMinimizeMetaData lmmd;
};

struct literalMetaData{
    ap_uint<386> compactlmd;
    ap_uint<32> decisionLevelStackEnd;

    //(321,258) addressStart
    //(257,194) numElements
    //(193,130) latestPage
    //(129,66) freeSpace
    //(65,34) insertedLevel
    //(33,2) decisionLevel
    //(1,1) IS_IN_STACK
    //(0,0) PHASE
};

struct literalMetaDataPCIE{
    ap_uint<512> compactlmd;

    //(354,322) contains deicisionLevelStackEnd
    //(321,258) addressStart
    //(257,194) numElements
    //(193,130) latestPage
    //(129,66) freeSpace
    //(65,34) insertedLevel
    //(33,2) decisionLevel
    //(1,1) IS_IN_STACK
    //(0,0) PHASE
};

struct lit_resolve{
    lit literal;
    bool resolved;
};

struct flippedLiteral{
    lit literal;
    literalMetaData lmd;
    literalMinimizeMetaData lmmd;
};

struct pqData{    
    double score;
    lit literal;
};

struct pqPosition{
    unsigned int pos;
};

struct pqPD{
    pqData data;
    pqPosition position;
};

struct miscellaneousData{
    unsigned int numLiterals;
    unsigned int numClauses;

    unsigned int literalElements;
    unsigned int clauseElements;

    double decayFactor;

    int miscCounters[256];
};

struct problemData{
    cls* clauseStore;
    clauseMetaData* cmd;
    lit* litStore;
    lit* answerStack;

    literalMetaDataPCIE* lmd;
    clsStatePCIE* clsStates;

    miscellaneousData md;
};




#endif