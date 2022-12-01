#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <thread>
#include <x/x.h>
#include "ctp.h"
#include "config.h"
#include "ctp_trade_spi.h"
#include "ctp_market_spi.h"

namespace co {
    class CTPServer {
    public:
        CTPServer() = default;
        ~CTPServer() = default;

        void Run();
        void Stop();

    protected:
        void QueryContracts();
        void SubQuotation();

    private:
        bool running_ = false;
        CThostFtdcMdApi* qApi_ = nullptr;
        CTPMarketSpi* qSpi_ = nullptr;
    };
}
