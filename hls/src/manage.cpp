#include "manage.h"

void allocatePage(hls::stream<lit>& litNewPage, mmuStream<unsigned int, _MAX_PAGES_LIT_STORE_>& freeLitPageAddresses,
    literalMetaData lmd[_FPGA_MAX_LITERALS], 
    ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16], int& error,
    const unsigned int LITERAL_PAGE_SIZE){
    #pragma HLS inline off
    ALLOCATE_PAGE: while(true){
        #pragma HLS loop_tripcount min=32 max=32
        #pragma HLS dependence variable=lmd inter false
        #pragma HLS dependence variable=litStore inter false

        if(litNewPage.empty()){
            break;
        }
        lit getLit = litNewPage.read();
        ap_uint<1> select = 0;
        if(getLit < 0){
            select = 1;
        }
        if(freeLitPageAddresses.empty()){
            error = -5;
            break;
        }
        unsigned int freePageAddress = freeLitPageAddresses.read();

        literalMetaData getLmd = lmd[abs(getLit)-1];

        unsigned int reqAddrLit = LMD_LATEST_PAGE(getLmd.compactlmd,select)/16;

        ap_uint<512> fetchLine = litStore[reqAddrLit + LITERAL_PAGE_SIZE/16 - 1];
        fetchLine.range(511,480) = freePageAddress;
        litStore[reqAddrLit + LITERAL_PAGE_SIZE/16 - 1] = fetchLine;

        LMD_LATEST_PAGE(getLmd.compactlmd,select) = freePageAddress;
        LMD_FREE_SPACE(getLmd.compactlmd,select) = LITERAL_PAGE_SIZE-2;

        ap_uint<512> clearLastSubPage = 0;
        clearLastSubPage.range(479,448) = reqAddrLit*16;
        litStore[freePageAddress/16 + LITERAL_PAGE_SIZE/16 - 1] = clearLastSubPage;

        lmd[abs(getLit)-1] = getLmd;
    }
}


void deleteTransposedClauses(ap_uint<512> litStore[_FPGA_MAX_LITERAL_ELEMENTS/16],
    literalMetaData lmd[_FPGA_MAX_LITERALS],
    mmuStream<unsigned int,_MAX_PAGES_LIT_STORE_>& freeLitPageAddresses, const unsigned int LITERAL_PAGE_SIZE, 
    hls::stream<ap_axiu<96,0,0,0>>& clauseStoreInputStream1, hls::stream<ap_axiu<32,0,0,0>>& clauseStoreOutputStream1, hls::stream<ap_axiu<32,0,0,0>>& locationOutputStream){

    #pragma HLS inline off
    
    ap_axiu<32,0,0,0> getData = clauseStoreOutputStream1.read();
    unsigned int removeTotal = getData.data;

    REMOVE_FROM_LITSTORE: for(unsigned int a = 0; a < removeTotal; a++){
        #pragma HLS loop_tripcount min=32 max=32
        ap_axiu<32,0,0,0> getData = clauseStoreOutputStream1.read();

        unsigned int removedID = getData.data+1;
        getData = clauseStoreOutputStream1.read();
        unsigned int numElementsCls = getData.data;

        REMOVE_FROM_LITSTORE_2: for(unsigned int i = 0; i < numElementsCls; i++){
            #pragma HLS loop_tripcount min=32 max=32
            #pragma HLS dependence variable=lmd inter false
            #pragma HLS dependence variable=litStore inter false

            ap_axiu<32,0,0,0> getData = clauseStoreOutputStream1.read();
            ap_axiu<32,0,0,0> getAddr = locationOutputStream.read();

            lit literalToRemove = getData.data;
            unsigned int address = getAddr.data;

            literalMetaData getLmd = lmd[abs(literalToRemove)-1];
            ap_uint<1> selectSide = 0;
            if(literalToRemove < 0){
                selectSide = 1;
            }

            unsigned int numElements = LMD_NUM_ELE(getLmd.compactlmd,selectSide);
            unsigned int latestPage = LMD_LATEST_PAGE(getLmd.compactlmd,selectSide)/16;
            unsigned int freeSpace = LMD_FREE_SPACE(getLmd.compactlmd,selectSide);
            unsigned int reqAddrOffsetMove = LITERAL_PAGE_SIZE - freeSpace - 2 - 1;

            ap_uint<512> replaceFetch = litStore[address/16];
            ap_uint<512> moveFetch;

            unsigned int previousLatestPage = latestPage;

            bool specialCase = false;
            unsigned int previousPage = 0;
            if(freeSpace == LITERAL_PAGE_SIZE-2){
                freeLitPageAddresses.write(latestPage*16);

                moveFetch = litStore[latestPage+LITERAL_PAGE_SIZE/16-1];
                LMD_LATEST_PAGE(getLmd.compactlmd,selectSide) = moveFetch.range(479,448);
                LMD_FREE_SPACE(getLmd.compactlmd,selectSide) = 1;
                reqAddrOffsetMove = LITERAL_PAGE_SIZE - 2 - 1;
                latestPage = moveFetch.range(479,448)/16;
            }else{
                LMD_FREE_SPACE(getLmd.compactlmd,selectSide) = freeSpace + 1;
            }

            cls movedCls;
            unsigned int replaceAddr = address;
            unsigned int swapAddr = latestPage*16 + reqAddrOffsetMove;
            if(address/16 == latestPage + reqAddrOffsetMove/16){ 
                movedCls = replaceFetch.range(32*(reqAddrOffsetMove%16)+31,32*(reqAddrOffsetMove%16));
                replaceFetch.range(32*(address%16)+31,32*(address%16)) = movedCls;
                replaceFetch.range(32*(reqAddrOffsetMove%16)+31,32*(reqAddrOffsetMove%16)) = 0;

                litStore[address/16] = replaceFetch;
            }else{
                moveFetch = litStore[latestPage + reqAddrOffsetMove/16];
                movedCls = moveFetch.range(32*(reqAddrOffsetMove%16)+31,32*(reqAddrOffsetMove%16));

                replaceFetch.range(32*(address%16)+31,32*(address%16)) = movedCls;
                moveFetch.range(32*(reqAddrOffsetMove%16)+31,32*(reqAddrOffsetMove%16)) = 0;

                litStore[address/16] = replaceFetch;
                litStore[latestPage + reqAddrOffsetMove/16] = moveFetch;
            }

            ap_axiu<96,0,0,0> sendData;
            sendData.data.range(31,0) = movedCls;
            sendData.data.range(63,32) = swapAddr;
            sendData.data.range(95,64) = replaceAddr;
            clauseStoreInputStream1.write(sendData);

            LMD_NUM_ELE(getLmd.compactlmd,selectSide) = numElements-1;
            lmd[abs(literalToRemove)-1] = getLmd;
        }
        #ifdef FPGA_HW
        ap_axiu<96,0,0,0> sendClauseInputCommand;
        sendClauseInputCommand.data.range(95,64) = csh::EXIT;
        clauseStoreInputStream1.write(sendClauseInputCommand);
        #endif
    }
}