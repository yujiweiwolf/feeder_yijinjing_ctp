#include "ctp_support.h"
#include "coral/define.h"

namespace co {


    // �����ж�
    bool is_flow_control(int result) {
        return ((result == -2) || (result == -3));
    }

    string ctp_str(const char* data) {
        string str = data;
        try {
            str = x::GBKToUTF8(str);
        } catch (...) {

        }
        return str;
    }

    int64_t ctp_market2std(TThostFtdcExchangeIDType v) {
        /////////////////////////////////////////////////////////////////////////
        ///TFtdcExchangeIDType��һ����������������
        /////////////////////////////////////////////////////////////////////////
        //typedef char TThostFtdcExchangeIDType[9];
        int64_t std_market = 0;
        string s = "." + string(v);
        if (s == co::kSuffixCFFEX) {
            std_market = co::kMarketCFFEX;
        } else if (s == co::kSuffixSHFE) {
            std_market = co::kMarketSHFE;
        } else if (s == co::kSuffixDCE) {
            std_market = co::kMarketDCE;
        } else if (s == co::kSuffixCZCE) {
            std_market = co::kMarketCZCE;
        } else if (s == co::kSuffixINE) {
            std_market = co::kMarketINE;
        } else {
			__error << "unknown ctp_market: " << v;
            xthrow() << "unknown ctp_market: " << v;
        }
        return std_market;
    }

    string market_suffix(int64_t market) {
        string suffix = "";
        switch (market) {
            case co::kMarketCFFEX:
                suffix = co::kSuffixCFFEX;
                break;
            case co::kMarketSHFE:
                suffix = co::kSuffixSHFE;
                break;
            case co::kMarketDCE:
                suffix = co::kSuffixDCE;
                break;
            case co::kMarketCZCE:
                suffix = co::kSuffixCZCE;
                break;
            case co::kMarketINE:
                suffix = co::kSuffixINE;
                break;
            default:
                break;
        }
        return suffix;
    }
}

