#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <x/x.h>

#include "ctp.h"
#include "define.h"

#ifdef _WIN32
#pragma comment(lib, "thostmduserapi_se.lib")
#pragma comment(lib, "thosttraderapi_se.lib")
#endif

namespace co {

    bool is_flow_control(int result);
    string ctp_str(const char* data);

    inline double ctp_price(const double& v) {
        return v > 0 && v <= kCtpMaxPrice ? v : 0;
    }

    int64_t ctp_market2std(TThostFtdcExchangeIDType v);

    string market_suffix(int64_t market);

    string get_product(const string& code);
}