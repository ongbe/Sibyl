
#include "Reshaper_ELW.h"

namespace sibyl
{

Reshaper_ELW::Reshaper_ELW(unsigned long maxGTck_,
                           TradeDataSet *pTradeDataSet_,
                           std::vector<std::string> *pFileList_,
                           const unsigned long (*ReadRawFile_)(std::vector<FLOAT>&, const std::string&, TradeDataSet*))
                           : Reshaper(maxGTck_, pTradeDataSet_, pFileList_, ReadRawFile_)
{
    inputDim  = 45;
}

void Reshaper_ELW::State2VecIn(FLOAT *vec, const ItemState &state)
{
    const long interval = 10; // seconds
    const long T = (const long)(std::ceil((6 * 3600 - 10 * 60)/interval) - 1);
    
    auto iItems = items.find(state.code);
    if (iItems == std::end(items))
    {
        auto it_bool = items.insert(std::make_pair(state.code, ItemMem_ELW()));
        verify(it_bool.second == true);
        iItems = it_bool.first;
        iItems->second.initPr       = state.pr;
        iItems->second.initKospi200 = state.kospi200;
        iItems->second.initThp      = state.thr[0];
    }
    auto &i = iItems->second; // reference to current ItemMem
    
    unsigned long idxInput = 0;
    
    // t
    vec[idxInput++] = (FLOAT) state.time / (interval * T);
    
    // pr
    vec[idxInput++] = ReshapePrice(state.pr) - ReshapePrice(i.initPr);
    
    // qr
    vec[idxInput++] = ReshapeQuant((INT) state.qr);
    
    // ps1 - pb1
    vec[idxInput++] = ReshapePrice(state.tbr[idxPs1].p) - ReshapePrice(state.tbr[idxPb1].p);
    
    // // tbpr(1:20)
    // for (std::size_t idx = 0; idx < (std::size_t)szTb; idx++)
    //     vec[idxInput++] = ReshapePrice(state.tbr[idx].p) - ReshapePrice(i.initPr);
    
    // tbqr(1:20) - removed 2 ticks from each end
    for (std::size_t idx = 2; idx < (std::size_t)(szTb - 2); idx++)
        vec[idxInput++] = ReshapeQuant(state.tbr[idx].q);
    
    // delta_tbqr(1:20) - removed 2 ticks from each end
    for (std::size_t idx = 2; idx < (std::size_t)(szTb - 2); idx++)
    {
        INT delta = state.tbr[idx].q;
        for (std::size_t idxL = 0; idxL < (std::size_t)szTb; idxL++)
        {
            if (state.tbr[idx].p == i.lastTb[idxL].p)
            {
                if ( (idx <= idxPs1 && idxL <= idxPs1) || 
                     (idx >= idxPb1 && idxL >= idxPb1) )
                    delta = state.tbr[idx].q - i.lastTb[idxL].q;
                else
                    delta = state.tbr[idx].q + i.lastTb[idxL].q;
                break;
            }
        }
        vec[idxInput++] = ReshapeQuant(delta);
    }
    i.lastTb = state.tbr;
    
    // ELW-only data
    verify(state.isELW == true);
    
    // call/put
    vec[idxInput++] = state.iCP; // +1/-1
    
    // kospi200
    vec[idxInput++] = ReshapePrice(state.kospi200) - ReshapePrice(i.initKospi200);
    
    // theoretical price
    vec[idxInput++] = ReshapePrice(state.thr[0]) - ReshapePrice(i.initThp);
    
    // volatility
    vec[idxInput++] = state.thr[1] / (FLOAT) 10;
    
    // delta
    vec[idxInput++] = state.thr[2];
    
    // gamma
    vec[idxInput++] = state.thr[3] * (FLOAT) 10;
    
    // theta
    vec[idxInput++] = state.thr[4] / (FLOAT) 10;
    
    // vega
    vec[idxInput++] = state.thr[5] / (FLOAT) 10;
    
    // rho
    vec[idxInput++] = state.thr[6] / (FLOAT) 10;
    
    verify(inputDim == idxInput);
    
    WhitenVector(vec); // this alters vector only if matrices are initialized
}

}