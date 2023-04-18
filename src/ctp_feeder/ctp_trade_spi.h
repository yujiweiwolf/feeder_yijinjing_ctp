#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <memory>

#include <x/x.h>
// #include "coral/coral.h"
#include "ctp.h"
#include "ctp_support.h"
#include "define.h"
#include "config.h"
#include  "instrument_field.h"

using namespace std;
using namespace x;

namespace co {

    class CTPTradeSpi : public CThostFtdcTraderSpi {
    public:
        CTPTradeSpi(CThostFtdcTraderApi* api);
        virtual ~CTPTradeSpi();

        virtual void OnFrontConnected();

        virtual void OnFrontDisconnected(int nReason);

        virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        void Wait();

    protected:
        int next_id();
        void ReqAuthenticate();
        void ReqUserLogin();
        void ReqQryInstrument();

    private:
        bool over_ = false;
        int ctp_request_id_ = 0;
        string cur_req_type_;
        int64_t date_ = 0;
        CThostFtdcTraderApi* api_ = nullptr;
        std::set<string> need_product_;
    };
}
