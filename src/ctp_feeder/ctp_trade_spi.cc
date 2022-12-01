#include "ctp_trade_spi.h"
#include <regex>

namespace co {

    CTPTradeSpi::CTPTradeSpi(CThostFtdcTraderApi* api) {
        api_ = api;
    }

    CTPTradeSpi::~CTPTradeSpi() {

    }

    ///当客户端与交易后台建立起通信连接时（还未登录前），该方法被调用。
    void CTPTradeSpi::OnFrontConnected() {
        __info << "connect to CTP trade server ok";
        // 在登陆之前，服务端要求对客户端进行身份认证，客户端通过认证之后才能请求登录。期货公司可以关闭该功能。
        string ctp_app_id = Config::Instance()->ctp_app_id();
        if (!ctp_app_id.empty()) {
            ReqAuthenticate();
        } else {
            ReqUserLogin();
        }
    };

    ///当客户端与交易后台通信连接断开时，该方法被调用。当发生这个情况后，API会自动重新连接，客户端可不做处理。
    ///@param nReason 错误原因
    ///        0x1001 网络读失败
    ///        0x1002 网络写失败
    ///        0x2001 接收心跳超时
    ///        0x2002 发送心跳失败
    ///        0x2003 收到错误报文
    void CTPTradeSpi::OnFrontDisconnected(int nReason) {
        stringstream ss;
        ss << "ret=" << nReason << ", msg=";
        switch (nReason) {
            case 0x1001:
                ss << "read error";
                break;
            case 0x1002:
                ss << "write error";
                break;
            case 0x2001:
                ss << "recv heartbeat timeout";
                break;
            case 0x2002:
                ss << "send heartbeat timeout";
                break;
            case 0x2003:
                ss << "recv broken data";
                break;
            default:
                ss << "unknown error";
                break;
        }
        __info << "connection is broken: " << ss.str();
        x::Sleep(1000);
    };

    void CTPTradeSpi::ReqAuthenticate() {
        __info << "authenticate ...";
        string broker_id = Config::Instance()->ctp_broker_id();
        string investor_id = Config::Instance()->ctp_investor_id();
        string app_id = Config::Instance()->ctp_app_id();
        string product_info = Config::Instance()->ctp_product_info();
        string auth_code = Config::Instance()->ctp_auth_code();
        if (app_id.length() >= sizeof(TThostFtdcAppIDType)) {
            __fatal << "app_id is too long, max length is " << (sizeof(TThostFtdcAppIDType) - 1);
            xthrow() << "app_id is too long, max length is " << (sizeof(TThostFtdcAppIDType) - 1);
        }
        if (product_info.length() >= sizeof(TThostFtdcProductInfoType)) {
            __fatal << "product_info is too long, max length is " << (sizeof(TThostFtdcProductInfoType) - 1);
            xthrow() << "product_info is too long, max length is " << (sizeof(TThostFtdcProductInfoType) - 1);
        }
        if (auth_code.length() >= sizeof(TThostFtdcAuthCodeType)) {
            __fatal << "auth_code is too long, max length is " << (sizeof(TThostFtdcAuthCodeType) - 1);
            xthrow() << "auth_code is too long, max length is " << (sizeof(TThostFtdcAuthCodeType) - 1);
        }
        CThostFtdcReqAuthenticateField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, broker_id.c_str());
        strcpy(req.UserID, investor_id.c_str());
        strcpy(req.AppID, app_id.c_str());
        strcpy(req.UserProductInfo, product_info.c_str());
        strcpy(req.AuthCode, auth_code.c_str());
        int rc = 0;
        while ((rc = api_->ReqAuthenticate(&req, next_id())) != 0) {
            __warn << "ReqAuthenticate failed: " << rc << ", retring ...";
            x::Sleep(kCtpRetrySleepMs);
        }
    }

    /// 客户端认证响应
    void CTPTradeSpi::OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pRspInfo == NULL || pRspInfo->ErrorID == 0) {
            __info << "authenticate ok";
            ReqUserLogin();
        } else {
            __error << "authenticate failed: ret=" << pRspInfo->ErrorID << ", msg=" << ctp_str(pRspInfo->ErrorMsg);
        }
    }

    void CTPTradeSpi::ReqUserLogin() {
        __info << "login ...";
        cur_req_type_ = "ReqUserLogin";
        CThostFtdcReqUserLoginField req;
        memset(&req, 0, sizeof(req));
        strcpy(req.BrokerID, Config::Instance()->ctp_broker_id().c_str());
        strcpy(req.UserID, Config::Instance()->ctp_investor_id().c_str());
        strcpy(req.Password, Config::Instance()->ctp_password().c_str());
        int rc = 0;
        while ((rc = api_->ReqUserLogin(&req, next_id())) != 0) {
            __warn << "ReqUserLogin failed: ret=" << rc << ", retring ...";
            x::Sleep(kCtpRetrySleepMs);
        }
    }

    ///登录请求响应
    void CTPTradeSpi::OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pRspInfo->ErrorID == 0) {
            date_ = atoi(api_->GetTradingDay());
            __info << "trade login ok: trading_day = " << date_;
            if (date_ < 19700101 || date_ > 29991231) {
                __error << "illegal trading day: " << date_;
                over_ = true;
            } else if (date_ < x::RawDate()) {
                __error << "trading day is not today: trading_day=" << date_ << ", today=" << x::RawDate() << ", quit ...";
                over_ = true;
            } else {
                // 如果立即查询合约，可能会在OnRspError中收到报错：ret=90, msg=CTP：查询未就绪，请稍后重试
                x::Sleep(5000);
                ReqQryInstrument();
            }
        } else {
            __error << "login failed: ret=" << pRspInfo->ErrorID << ", msg=" << ctp_str(pRspInfo->ErrorMsg);
            over_ = true;
        }
    };

    void CTPTradeSpi::OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pRspInfo->ErrorID == 0) {
            __info << "logout ok";
        } else {
            __error << "logout failed: ret=" << pRspInfo->ErrorID << ", msg=" << ctp_str(pRspInfo->ErrorMsg);
        }
    }

    void CTPTradeSpi::ReqQryInstrument() {
        __info << "query contracts ...";
        cur_req_type_ = "ReqQryInstrument";
        CThostFtdcQryInstrumentField req;
        memset(&req, 0, sizeof(req));
        int rc = 0;
        while ((rc = api_->ReqQryInstrument(&req, next_id())) != 0) {
            __warn << "ReqQryInstrument failed: ret=" << rc << ", retring ...";
            x::Sleep(kCtpRetrySleepMs);
        }
    }

    ///请求查询合约响应
    void CTPTradeSpi::OnRspQryInstrument(CThostFtdcInstrumentField *p, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        if (pRspInfo == NULL || pRspInfo->ErrorID == 0) {
            Singleton<InstrumentMgr>::GetInstance()->AddInstrument(p);
            if (bIsLast) {
                std::vector<CThostFtdcInstrumentField> all_instrument = Singleton<InstrumentMgr>::GetInstance()->GetInstrument();
                vector<string> ctxs;
                for (auto it : all_instrument) {
                    ctxs.push_back(it.InstrumentID);
                }
                std::sort(ctxs.begin(), ctxs.end(), [&](const string & a, const string & b) -> bool {return a < b; });
                stringstream ss;
                ss << "query contracts ok: size=" << ctxs.size() << ", codes=";
                for (size_t i = 0; i < ctxs.size(); ++i) {
                    if (i > 0) {
                        ss << ",";
                    }
                    ss << ctxs[i];
                }
                __info << ss.str();
                over_ = true;
            }
        } else {
            __error << "query all future codes failed: ret=" << pRspInfo->ErrorID << ", msg=" << ctp_str(pRspInfo->ErrorMsg);
            over_ = true;
        }
    }

    ///错误应答
    void CTPTradeSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        __error << "OnRspError: ret=" << pRspInfo->ErrorID << ", msg=" << ctp_str(pRspInfo->ErrorMsg);
        if (pRspInfo->ErrorID == 90) { // <error id="NEED_RETRY" value="90" prompt="CTP：查询未就绪，请稍后重试" />
            x::Sleep(kCtpRetrySleepMs);
            if (cur_req_type_ == "ReqUserLogin") {
                ReqUserLogin();
            } else if (cur_req_type_ == "ReqQryInstrument") {
                ReqQryInstrument();
            }
        }
    };

    void CTPTradeSpi::Wait() {
        while (!over_) {
            x::Sleep(10); // sleep 10ms
        }
    }

    int CTPTradeSpi::next_id() {
        return ctp_request_id_++;
    }


}

