#include "ctp_trade_spi.h"
#include <regex>

namespace co {

    CTPTradeSpi::CTPTradeSpi(CThostFtdcTraderApi* api) {
        api_ = api;
    }

    CTPTradeSpi::~CTPTradeSpi() {

    }

    ///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
    void CTPTradeSpi::OnFrontConnected() {
        __info << "connect to CTP trade server ok";
        string sub_instrument = Config::Instance()->sub_instrument();
        if (!sub_instrument.empty()) {
            vector<string> products;
            x::Split(&products, sub_instrument, ";");
            for (auto& it: products) {
                need_product_.insert(it);
            }
        }
        // �ڵ�½֮ǰ�������Ҫ��Կͻ��˽��������֤���ͻ���ͨ����֤֮����������¼���ڻ���˾���Թرոù��ܡ�
        string ctp_app_id = Config::Instance()->ctp_app_id();
        if (!ctp_app_id.empty()) {
            ReqAuthenticate();
        } else {
            ReqUserLogin();
        }
    };

    ///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
    ///@param nReason ����ԭ��
    ///        0x1001 �����ʧ��
    ///        0x1002 ����дʧ��
    ///        0x2001 ����������ʱ
    ///        0x2002 ��������ʧ��
    ///        0x2003 �յ�������
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

    /// �ͻ�����֤��Ӧ
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

    ///��¼������Ӧ
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
                // ���������ѯ��Լ�����ܻ���OnRspError���յ�����ret=90, msg=CTP����ѯδ���������Ժ�����
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

    ///�����ѯ��Լ��Ӧ
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

    ///����Ӧ��
    void CTPTradeSpi::OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {
        __error << "OnRspError: ret=" << pRspInfo->ErrorID << ", msg=" << ctp_str(pRspInfo->ErrorMsg);
        if (pRspInfo->ErrorID == 90) { // <error id="NEED_RETRY" value="90" prompt="CTP����ѯδ���������Ժ�����" />
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
