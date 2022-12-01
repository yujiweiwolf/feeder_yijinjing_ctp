#include "ctp_server.h"

namespace co {

    void CTPServer::Run() {
        running_ = true;
        Singleton<InstrumentMgr>::Instance();
        QueryContracts();

        SubQuotation();
        while (true) {
            x::Sleep(1000);
        }
        Stop();
    };

    void CTPServer::QueryContracts() {
        __info << "connecting to ctp trade server ...";
        CThostFtdcTraderApi* tApi = CThostFtdcTraderApi::CreateFtdcTraderApi("");
        CTPTradeSpi* tSpi = new CTPTradeSpi(tApi);
        tApi->RegisterSpi(tSpi);
        string trade_addr = Config::Instance()->ctp_trade_front();
        tApi->RegisterFront((char*)trade_addr.c_str());
        tApi->SubscribePublicTopic(THOST_TERT_RESTART);
        tApi->SubscribePrivateTopic(THOST_TERT_RESUME);
        tApi->Init();
        tSpi->Wait(); // �ȴ���ѯ����
        tApi->RegisterSpi(nullptr);
        tApi->Release();
        tApi = nullptr;
        delete tSpi;
        tSpi = nullptr;
    }

    void CTPServer::SubQuotation() {
        __info << "connecting to ctp market server ...";
        qApi_ = CThostFtdcMdApi::CreateFtdcMdApi("");
        qSpi_ = new CTPMarketSpi(qApi_);
        qSpi_->Init();
        qApi_->RegisterSpi(qSpi_);
        string market_addr = Config::Instance()->ctp_market_front();
        qApi_->RegisterFront((char*)market_addr.c_str());
        qApi_->Init();
    }

    void CTPServer::Stop() {
        running_ = false;
        if (qApi_) {
            qApi_->RegisterSpi(nullptr);
            qApi_->Release();
            qApi_ = nullptr;
        }
        if (qSpi_) {
            delete qSpi_;
            qSpi_ = nullptr;
        }
    }

}

