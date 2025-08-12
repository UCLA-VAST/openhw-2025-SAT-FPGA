#include <hls_stream.h>
#include <ap_axi_sdata.h>
#include <ap_utils.h>
#include "data_structures.h"

void copyCls(ap_uint<128> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/4], ap_uint<128>* clauseStore, const unsigned int clauseElements){
    #pragma HLS inline off

    unsigned int clauseElements4 = clauseElements/4;
    if(clauseElements%4 != 0){
        clauseElements4++;
    }
    COPY_CLS_STORE: for(unsigned int i = 0; i < clauseElements4; i++){
        #pragma HLS loop_tripcount min=1024 max=1024
        mClsStore[i] = reg(reg(reg(clauseStore[i])));
    }
}

void copyCmd(clauseMetaData mCmd[_FPGA_MAX_CLAUSES], clauseMetaData* cmd, const unsigned int ORIGINAL_CLS_CNT){
    #pragma HLS inline off
    COPY_CMD: for(unsigned int i = 0; i < ORIGINAL_CLS_CNT; i++){
        #pragma HLS loop_tripcount min=1024 max=1024
        mCmd[i] = reg(reg(cmd[i]));
    }
}

void copy_cls_data(ap_uint<128> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/4], clauseMetaData mCmd[_FPGA_MAX_CLAUSES],
    ap_uint<128>* clauseStore, clauseMetaData* cmd,
    const unsigned int clauseElements, const unsigned int ORIGINAL_CLS_CNT){
    #pragma HLS inline off
    #pragma HLS dataflow

    copyCmd(mCmd, cmd, ORIGINAL_CLS_CNT);
    copyCls(mClsStore, clauseStore, clauseElements);
}

void axiStreamBuffer_sendDelete(hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream, hls::stream<ap_uint<96>>& intermediateStream){
    #pragma HLS inline off

    BUFFER_LOOP_DELETE: while(true){
        #pragma HLS loop_tripcount min=16 max=16
        ap_axiu<96,0,0,0> read = clauseStoreInputStream.read();

        if((int)read.data.range(95,64) == csh::EXIT){
            break;
        }

        ap_uint<96> getData = read.data;
        intermediateStream.write(getData);  
    }
}

#ifdef FPGA_HW
void axiStreamBuffer_sendLength(hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream, hls::stream<ap_uint<32>>& intermediateStream){
#else
void axiStreamBuffer_sendLength(hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream, 
    const clauseMetaData mCmd[_FPGA_MAX_CLAUSES]){
#endif
    #pragma HLS inline off
    BUFFER_LOOP: while(true){
        #pragma HLS loop_tripcount min=32 max=32
        ap_axiu<96,0,0,0> read = clauseStoreInputStream.read();

        cls getCls = read.data.range(31,0);

        #ifdef FPGA_HW
        ap_uint<32> send = getCls;
        intermediateStream.write(send);
        if(getCls == csh::EXIT){
            break;
        }
        #else

        if(getCls == csh::EXIT){
            break;
        }
        clauseMetaData cmd = mCmd[getCls];
        ap_axiu<32,0,0,0> sendData;
        sendData.data = cmd.numElements;
        clauseStoreOutputStream.write(sendData);
        #endif
    }
}

void sendLength(hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream, hls::stream<ap_uint<32>>& intermediateStream, 
    const clauseMetaData mCmd[_FPGA_MAX_CLAUSES]){
    #pragma HLS inline off
    LOOP_SEND_LENGTH: while(true){

        ap_uint<32> read;
        if(intermediateStream.read_nb(read)){
            cls clsID = read.range(31,0);
            if(clsID == csh::EXIT){
                break;
            }

            clauseMetaData cmd = mCmd[clsID];
            ap_axiu<32,0,0,0> sendData;
            sendData.data = cmd.numElements;
            clauseStoreOutputStream.write(sendData);
            
        }
    }
}

void sendLength_wrapper(hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream, 
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1,
    const clauseMetaData mCmd[_FPGA_MAX_CLAUSES]){
    #pragma HLS inline off

    hls::stream<ap_uint<32>> intermediateStream;
    #pragma HLS stream variable=intermediateStream depth=16

    #pragma HLS dataflow
    
    #ifdef FPGA_HW
    axiStreamBuffer_sendLength(clauseStoreInputStream1, intermediateStream);
    sendLength(clauseStoreOutputStream, intermediateStream, mCmd);
    #else
    axiStreamBuffer_sendLength(clauseStoreInputStream1, clauseStoreOutputStream, mCmd);
    #endif

}

void sendLoop(const ap_uint<128> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/4], const clauseMetaData mCmd[_FPGA_MAX_CLAUSES], 
    const unsigned int ORIGINAL_CLS_CNT, const unsigned int CLAUSE_PAGE_SIZE, const cls clsID, 
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream){
    unsigned int state = 0;

    clauseMetaData cmd;
    unsigned int usePageSize = CLAUSE_PAGE_SIZE;
    
    unsigned int subIndex = 0;
    unsigned int reqAddrLit = 0;
        
    unsigned int processed = 0;
    unsigned int tmpAddr = 0;    

    cmd = mCmd[clsID];
    if(clsID <= ORIGINAL_CLS_CNT-1){
        usePageSize = 4;
    }
    reqAddrLit = cmd.addressStart;

    SEND_DATA_2: while(true){
        if(state == 0){
            ap_uint<128> get = mClsStore[reqAddrLit/4];
            tmpAddr = get.range(127,96);
            
            ap_axiu<32,0,0,0> sendData;
            sendData.data = get.range(32*(subIndex%4)+31,32*(subIndex%4));
            clauseStoreOutputStream.write(sendData);

            subIndex++;
            reqAddrLit++;
            processed++;
            if(processed == cmd.numElements){
                break;
            }else if(subIndex == usePageSize-1){
                subIndex = 0;
                state = 1;
            }
        }else if(state == 1){
            state = 0;
            reqAddrLit = tmpAddr;
        }
    }
}

void sendData(const ap_uint<128> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/4], const clauseMetaData mCmd[_FPGA_MAX_CLAUSES], 
    const unsigned int ORIGINAL_CLS_CNT, const unsigned int CLAUSE_PAGE_SIZE,
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream){
    #pragma HLS inline off


    SEND_DATA_1: while(true){
        ap_axiu<96,0,0,0> readOne = clauseStoreInputStream.read();

        if((int)readOne.data.range(95,64) == csh::EXIT){
            break;
        }

        cls clsID = readOne.data.range(31,0);
        
        sendLoop(mClsStore, mCmd, ORIGINAL_CLS_CNT, CLAUSE_PAGE_SIZE, clsID, clauseStoreOutputStream);

        ap_axiu<32,0,0,0> sendData;
        sendData.data = 0;
        clauseStoreOutputStream.write(sendData);
    }
}

void sendData_dataflow(const ap_uint<128> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/4], const clauseMetaData mCmd[_FPGA_MAX_CLAUSES], 
    const unsigned int ORIGINAL_CLS_CNT, const unsigned int CLAUSE_PAGE_SIZE, 
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1, hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream2, 
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream1, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream2){
    #pragma HLS inline off
    #pragma HLS dataflow

    sendData(mClsStore, mCmd, ORIGINAL_CLS_CNT, CLAUSE_PAGE_SIZE, clauseStoreInputStream1, clauseStoreOutputStream1);
    sendData(mClsStore, mCmd, ORIGINAL_CLS_CNT, CLAUSE_PAGE_SIZE, clauseStoreInputStream2, clauseStoreOutputStream2);
}

void saveData(ap_uint<128> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/4],
    mmuStream<unsigned int, _MAX_PAGES_CLS_STORE_>& freeClsPageAddresses,
    const clauseMetaData cmd, const unsigned int CLAUSE_PAGE_SIZE, hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1,
    hls::stream<ap_axiu<64,0,0,0>>& locationInputStream){
    #pragma HLS inline off

    unsigned int reqAddrOffsetCls = 0;
    ap_uint<128> get = 0;
    unsigned int tmpAddr = cmd.addressStart;
    unsigned int processed = 0;
    unsigned int nextAddr = 0;

    ap_axiu<64,0,0,0> updateLitStorePos;
    updateLitStorePos.data.range(31,0) = lh::SAVE;
    locationInputStream.write(updateLitStorePos);

    for(unsigned int i = 0; i < cmd.numElements; i++){
        ap_axiu<96,0,0,0> getData = clauseStoreInputStream1.read();

        get.range(32*(reqAddrOffsetCls%4)+31,32*(reqAddrOffsetCls%4)) = getData.data.range(31,0);

        unsigned int clsToLitAddr = tmpAddr + (reqAddrOffsetCls%4);

        ap_axiu<64,0,0,0> updateLitStorePos;
        updateLitStorePos.data.range(31,0) = clsToLitAddr;
        updateLitStorePos.data.range(63,32) = getData.data.range(63,32);

        locationInputStream.write(updateLitStorePos);

        processed++;
        reqAddrOffsetCls++;

        bool useNewPage = false;
        if(reqAddrOffsetCls == CLAUSE_PAGE_SIZE-1){
            if(i != cmd.numElements){
                nextAddr = reg(freeClsPageAddresses.read());
                
                get.range(127,96) = nextAddr;
                useNewPage = true;
            }
            reqAddrOffsetCls = 0;
        }

        mClsStore[tmpAddr/4] = get; 

        if(reqAddrOffsetCls%4 == 0){
            if(useNewPage){
                tmpAddr = nextAddr;
            }else{
                tmpAddr += 4;
            }
            get = 0;
        }  
    }

    updateLitStorePos.data = lh::EXIT;
    locationInputStream.write(updateLitStorePos);
}

void getDeletedClsID(hls::stream<cls>& removeIDStream,
    cls* usedClsIDBuckets, minimumStreamTracker tracker[_FPGA_MAX_LBD_BUCKETS],
    cls lastInsertedID, unsigned int removeTotal,
    unsigned int LBDBucketCount[_FPGA_MAX_LBD_BUCKETS]){
    #pragma HLS inline off

    unsigned int removedCount = 0;
    unsigned int startBucketIndex = _FPGA_MAX_LBD_BUCKETS-1;
    bool skipped = false;
    GET_REMOVE_ID: while(true){
        #pragma HLS loop_tripcount min=16 max=16

        if(removedCount == removeTotal){
            break;
        }

        if(tracker[startBucketIndex].usedCount == 0 || skipped){
            skipped = false;
            startBucketIndex--;
            continue;
        }

        unsigned int toEndCount = _FPGA_MAX_CLAUSES - tracker[startBucketIndex].accessIdx;
        if(tracker[startBucketIndex].accessIdx < tracker[startBucketIndex].insertIdx){
            toEndCount = tracker[startBucketIndex].insertIdx - tracker[startBucketIndex].accessIdx;
        }
        unsigned int offset = tracker[startBucketIndex].accessIdx;

        if(toEndCount >= removeTotal-removedCount){
            toEndCount = removeTotal-removedCount;
        }

        bool skip = false;
        GET_DEL_CLS: for(unsigned int i = 0; i < toEndCount; i++){
            #pragma HLS pipeline
            #pragma HLS loop_tripcount min=16 max=16

            cls getClsID = usedClsIDBuckets[_FPGA_MAX_CLAUSES*startBucketIndex+offset+i];

            if(getClsID != lastInsertedID){
                LBDBucketCount[startBucketIndex]++;
                removedCount++;
                removeIDStream.write(getClsID);

            }else{
                skip = true;
                skipped = true;
            }
        }

        tracker[startBucketIndex].accessIdx = (tracker[startBucketIndex].accessIdx + (toEndCount-skip))%_FPGA_MAX_CLAUSES;
        tracker[startBucketIndex].usedCount -= (toEndCount-skip);
    }
}

#ifdef FPGA_HW
void deleteClauses(mmuStream<cls, _FPGA_MAX_CLAUSES>& freeClsID, mmuStream<unsigned int, _MAX_PAGES_CLS_STORE_>& freeClsPageAddresses,
    hls::stream<cls>& removeIDStream, hls::stream<ap_uint<96>>& intermediateStream,
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream1, hls::stream<ap_axiu<64,0,0,0>>& locationInputStream,
    const clauseMetaData mCmd[_FPGA_MAX_CLAUSES], ap_uint<128> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/4],
    const unsigned int CLAUSE_PAGE_SIZE){
#else
void deleteClauses(mmuStream<cls, _FPGA_MAX_CLAUSES>& freeClsID, mmuStream<unsigned int, _MAX_PAGES_CLS_STORE_>& freeClsPageAddresses,
    hls::stream<cls>& removeIDStream,
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream,
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream1, hls::stream<ap_axiu<64,0,0,0>>& locationInputStream,
    const clauseMetaData mCmd[_FPGA_MAX_CLAUSES], ap_uint<128> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/4], 
    const unsigned int CLAUSE_PAGE_SIZE){
#endif

    #pragma HLS inline off

    unsigned int removeID = removeIDStream.read();

    freeClsID.write(removeID);
    clauseMetaData getCmd = mCmd[removeID];

    ap_axiu<32,0,0,0> sendData;
    sendData.data = removeID;
    clauseStoreOutputStream1.write(sendData);
    sendData.data = getCmd.numElements;
    clauseStoreOutputStream1.write(sendData);

    unsigned int subIndex = 0;
    unsigned int reqAddrLit = getCmd.addressStart;
    unsigned int state = 0;
    unsigned int processed = 0;
    unsigned int tmpAddr = 0;
    unsigned int numElements = getCmd.numElements;

    ap_axiu<64,0,0,0> updateLitStorePos;
    updateLitStorePos.data.range(31,0) = lh::SEND;
    locationInputStream.write(updateLitStorePos);

    ap_uint<128> get = 0;
    SEND_DATA_DELETE_NO_WRAP: while(true){
        #pragma HLS loop_tripcount min=16 max=16

        if(state == 0){
            if(subIndex == 0){
                freeClsPageAddresses.write(reqAddrLit);
            }

            get = mClsStore[reqAddrLit/4];
            tmpAddr = get.range(127,96);

            ap_axiu<32,0,0,0> sendData;
            sendData.data = get.range(32*(subIndex%4)+31,32*(subIndex%4));
            clauseStoreOutputStream1.write(sendData);

            ap_axiu<64,0,0,0> updateLitStorePos;
            updateLitStorePos.data.range(31,0) = (reqAddrLit/4)*4+subIndex%4;
            locationInputStream.write(updateLitStorePos);
                
            subIndex++;
            reqAddrLit++;
            processed++;
                
        if(processed == numElements){
                state = 2;
            }else if(subIndex == CLAUSE_PAGE_SIZE-1){
                subIndex = 0;
                state = 1;
            }
        }else if(state == 1){
            state = 0;
            reqAddrLit = tmpAddr;
        }else if(state == 2){
            break;
        }
    }

    updateLitStorePos.data = lh::EXIT;
    locationInputStream.write(updateLitStorePos);

    ap_wait();

    updateLitStorePos.data.range(31,0) = lh::UPDATE;
    locationInputStream.write(updateLitStorePos);

    UPDATE_LIT_STORE_POS: for(unsigned int j = 0; j < numElements; j++){
        #pragma HLS loop_tripcount min=16 max=16

        ap_uint<96> getClsToUpdate;
        #ifdef FPGA_HW
        getClsToUpdate = intermediateStream.read();
        #else
        getClsToUpdate = clauseStoreInputStream.read().data;
        #endif
        
        unsigned int clsIDUpdate = getClsToUpdate.range(31,0);
        unsigned int swapAddr = getClsToUpdate.range(63,32);
        unsigned int replaceAddr = getClsToUpdate.range(95,64);

        if(clsIDUpdate-1 == removeID){
            continue;
        }

        updateLitStorePos.data.range(31,0) = swapAddr;
        updateLitStorePos.data.range(63,32) = replaceAddr;
        locationInputStream.write(updateLitStorePos);
    }

    updateLitStorePos.data = lh::EXIT;
    locationInputStream.write(updateLitStorePos);
}

void delete_wrapper(mmuStream<cls, _FPGA_MAX_CLAUSES>& freeClsID, 
    hls::stream<cls>& removeIDStream,
    mmuStream<unsigned int, _MAX_PAGES_CLS_STORE_>& freeClsPageAddresses,
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1,
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream1, hls::stream<ap_axiu<64,0,0,0>>& locationInputStream,
    const clauseMetaData mCmd[_FPGA_MAX_CLAUSES], ap_uint<128> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/4], 
    const unsigned int removeTotal, const unsigned int CLAUSE_PAGE_SIZE){
    #pragma HLS inline off

    hls::stream<ap_uint<96>> intermediateStream;
    #pragma HLS stream variable=intermediateStream depth=1024

    DELETE_LOOP: for(unsigned int i = 0; i < removeTotal; i++){
        #pragma HLS loop_tripcount min=16 max=16
        #pragma HLS dataflow

        axiStreamBuffer_sendDelete(clauseStoreInputStream1, intermediateStream);
        #ifdef FPGA_HW
        deleteClauses(freeClsID, freeClsPageAddresses, removeIDStream,
            intermediateStream, clauseStoreOutputStream1, locationInputStream,
            mCmd, mClsStore, CLAUSE_PAGE_SIZE);
        #endif
    }

}

void deleteClauses_wrapper(mmuStream<cls, _FPGA_MAX_CLAUSES>& freeClsID, 
    mmuStream<unsigned int, _MAX_PAGES_CLS_STORE_>& freeClsPageAddresses,
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1,
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream1, hls::stream<ap_axiu<64,0,0,0>>& locationInputStream,
    cls* usedClsIDBuckets, minimumStreamTracker tracker[_FPGA_MAX_LBD_BUCKETS], cls lastInsertedID,
    const clauseMetaData mCmd[_FPGA_MAX_CLAUSES], ap_uint<128> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/4],
    const unsigned int removeTotal, const unsigned int CLAUSE_PAGE_SIZE, unsigned int LBDBucketCount[_FPGA_MAX_LBD_BUCKETS]){
    #pragma HLS inline off

    
    hls::stream<cls> removeIDStream;
    #pragma HLS stream variable=removeIDStream depth=64

    #pragma HLS dataflow

    #ifdef FPGA_HW
    getDeletedClsID(removeIDStream, usedClsIDBuckets, tracker, lastInsertedID, removeTotal, LBDBucketCount);
    delete_wrapper(freeClsID, removeIDStream,
        freeClsPageAddresses,
        clauseStoreInputStream1,
        clauseStoreOutputStream1, locationInputStream,
        mCmd, mClsStore, removeTotal, CLAUSE_PAGE_SIZE);
    #else
    getDeletedClsID(removeIDStream, usedClsIDBuckets, tracker, lastInsertedID, removeTotal, LBDBucketCount);
    for(unsigned int i = 0; i < removeTotal; i++){
        deleteClauses(freeClsID, freeClsPageAddresses, removeIDStream,
            clauseStoreInputStream1, clauseStoreOutputStream1, locationInputStream,
            mCmd, mClsStore, CLAUSE_PAGE_SIZE);
    }
    #endif

}


extern "C"{
void clause_store_handler(ap_uint<128>* clauseStore, clauseMetaData* cmd, 
    cls* usedClsIDBuckets, unsigned int* trackLBD,
    const unsigned int initialClauseElements, const unsigned int maxClauseElements, 
    const unsigned int initialClauseCount, const unsigned int clausePageSize,
    const double prunePctage,
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1, hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream2,
    hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream1, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream2,
    hls::stream<ap_axiu<64,0,0,0>>& locationInputStream){

    #pragma HLS INTERFACE m_axi port=clauseStore offset=slave bundle=gmem13 latency=40
    #pragma HLS INTERFACE m_axi port=cmd offset=slave bundle=gmem14 latency=40
    #pragma HLS INTERFACE m_axi port=usedClsIDBuckets offset=slave bundle=gmem14 latency=40
    #pragma HLS INTERFACE m_axi port=trackLBD offset=slave bundle=gmem14 latency=40

    #pragma HLS INTERFACE s_axilite port=initialClauseElements
    #pragma HLS INTERFACE s_axilite port=maxClauseElements
    #pragma HLS INTERFACE s_axilite port=initialClauseCount
    #pragma HLS INTERFACE s_axilite port=clausePageSize
	#pragma HLS INTERFACE axis port=clauseStoreInputStream1
    #pragma HLS INTERFACE axis port=clauseStoreInputStream2
    #pragma HLS INTERFACE axis port=clauseStoreOutputStream1
    #pragma HLS INTERFACE axis port=clauseStoreOutputStream2
	#pragma HLS INTERFACE s_axilite port=return

    const unsigned int MAX_CLAUSE_ELEMENTS = maxClauseElements;
    const unsigned int CLAUSE_PAGE_SIZE = clausePageSize;
    unsigned int clauseElements = initialClauseElements;
    const unsigned int ORIGINAL_CLS_CNT = initialClauseCount;
    const double PRUNE_PERCENTAGE = prunePctage;

    unsigned int usedTotalIDCount = 0;

    mmuStream<unsigned int, _MAX_PAGES_CLS_STORE_> freeClsPageAddresses(clauseElements,_FPGA_MAX_LITERAL_ELEMENTS,CLAUSE_PAGE_SIZE);
    #pragma HLS bind_storage variable=freeClsPageAddresses.array type=RAM_S2P impl=URAM latency=1

    mmuStream<cls, _FPGA_MAX_CLAUSES> freeClsID(ORIGINAL_CLS_CNT,_FPGA_MAX_CLAUSES,1);
    #pragma HLS bind_storage variable=freeClsID.array type=RAM_S2P impl=URAM latency=2

    ap_uint<128> mClsStore[_FPGA_MAX_LITERAL_ELEMENTS/4];
    #pragma HLS bind_storage variable=mClsStore type=RAM_T2P impl=URAM latency=2

    clauseMetaData mCmd[_FPGA_MAX_CLAUSES];
    #pragma HLS aggregate variable=mCmd compact=auto
    #pragma HLS bind_storage variable=mCmd type=RAM_T2P impl=URAM latency=2

    unsigned int LBDBucketCount[2][_FPGA_MAX_LBD_BUCKETS];
    minimumStreamTracker tracker[_FPGA_MAX_LBD_BUCKETS];
    INITIALIZE_LBD_META: for(unsigned int i = 0; i < _FPGA_MAX_LBD_BUCKETS; i++){
        tracker[i].insertIdx = 0;
        tracker[i].accessIdx = 0;
        tracker[i].usedCount = 0;
        LBDBucketCount[0][i] = 0;
        LBDBucketCount[1][i] = 0;
    }

    copy_cls_data(mClsStore, mCmd,
        clauseStore, cmd,
        clauseElements, ORIGINAL_CLS_CNT);

    cls freeID;
    cls lastInsertedID;
    while(true){
        ap_axiu<96,0,0,0> getCommand = clauseStoreInputStream1.read();
        unsigned int code = getCommand.data.range(95,64);

        if(code == csh::EXIT){
            break;
        }else if(code == csh::SEND_LEN || code == csh::SEND_LEN_BCP){
            sendLength_wrapper(clauseStoreOutputStream1, clauseStoreInputStream1, mCmd);
        }else if(code == csh::SEND_CLS){
            
            sendData_dataflow(mClsStore, mCmd, 
                ORIGINAL_CLS_CNT, CLAUSE_PAGE_SIZE,
                clauseStoreInputStream1, clauseStoreInputStream2,
                clauseStoreOutputStream1, clauseStoreOutputStream2);
            
        }else if(code == csh::SAVE){
            clauseMetaData cmd;
            cmd.numElements = getCommand.data.range(31,0);

            ap_axiu<32,0,0,0> sendData;
            if((freeClsPageAddresses.size()*(CLAUSE_PAGE_SIZE-1) < cmd.numElements) || freeClsID.empty()){
                sendData.data = -4;
                clauseStoreOutputStream1.write(sendData);
            }else{
                cmd.addressStart = freeClsPageAddresses.read();
                
                freeID = freeClsID.read();
                lastInsertedID = freeID;
                mCmd[freeID] = cmd;

                sendData.data = freeID;
                clauseStoreOutputStream1.write(sendData);
                saveData(mClsStore, freeClsPageAddresses,
                    cmd, CLAUSE_PAGE_SIZE, clauseStoreInputStream1, locationInputStream);
            }            
        }else if(code == csh::BUCKET){
            unsigned int lbdLevelIndex = getCommand.data.range(31,0)-2;
            if(lbdLevelIndex > 9){
                lbdLevelIndex = 9;
            }

            usedClsIDBuckets[_FPGA_MAX_CLAUSES*lbdLevelIndex+tracker[lbdLevelIndex].insertIdx] = freeID;
            tracker[lbdLevelIndex].insertIdx = (tracker[lbdLevelIndex].insertIdx+1) % _FPGA_MAX_CLAUSES;
            tracker[lbdLevelIndex].usedCount++;
            LBDBucketCount[0][lbdLevelIndex]++;
            usedTotalIDCount++;
        }else if(code == csh::DELETE){
            unsigned int removeTotal = usedTotalIDCount * PRUNE_PERCENTAGE;
            usedTotalIDCount -= removeTotal;
            ap_axiu<32,0,0,0> sendData;
            sendData.data = removeTotal;
            clauseStoreOutputStream1.write(sendData);

            if(removeTotal > 0){
                deleteClauses_wrapper(freeClsID, freeClsPageAddresses,
                    clauseStoreInputStream1, clauseStoreOutputStream1, locationInputStream,
                    usedClsIDBuckets, tracker, lastInsertedID,
                    mCmd, mClsStore, removeTotal, CLAUSE_PAGE_SIZE, LBDBucketCount[1]);
            }
        }
    }

    WRITE_OUT_LBD_STATS: for(unsigned int i = 0; i < _FPGA_MAX_LBD_BUCKETS; i++){
        trackLBD[i] = LBDBucketCount[0][i];
        trackLBD[i+_FPGA_MAX_LBD_BUCKETS] = LBDBucketCount[1][i];
    }

    ap_axiu<64,0,0,0> updateLitStorePos;
    updateLitStorePos.data = lh::EXIT;
    locationInputStream.write(updateLitStorePos);

}
}
