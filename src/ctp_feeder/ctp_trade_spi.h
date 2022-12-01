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

        ///���ͻ����뽻�׺�̨������ͨ������ʱ����δ��¼ǰ�����÷��������á�
        virtual void OnFrontConnected();

        ///���ͻ����뽻�׺�̨ͨ�����ӶϿ�ʱ���÷��������á���������������API���Զ��������ӣ��ͻ��˿ɲ�������
        ///@param nReason ����ԭ��
        ///        0x1001 �����ʧ��
        ///        0x1002 ����дʧ��
        ///        0x2001 ����������ʱ
        ///        0x2002 ��������ʧ��
        ///        0x2003 �յ�������
        virtual void OnFrontDisconnected(int nReason);

        /// �ͻ�����֤��Ӧ
        virtual void OnRspAuthenticate(CThostFtdcRspAuthenticateField *pRspAuthenticateField, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///��¼������Ӧ
        virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///�ǳ�������Ӧ
        virtual void OnRspUserLogout(CThostFtdcUserLogoutField *pUserLogout, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///�����ѯ��������Ӧ
        //virtual void OnRspQryExchange(CThostFtdcExchangeField *pExchange, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast) {};

        ///�����ѯ��Լ��Ӧ
        virtual void OnRspQryInstrument(CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///����Ӧ��
        virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        // �ȴ���ѯ��Լ��Ϣ����
        void Wait();

    protected:
        int next_id();
        void ReqAuthenticate(); // �ͻ�����֤����
        void ReqUserLogin();
        void ReqQryInstrument();

    private:
        bool over_ = false; // ��ѯ��Լ��Ϣ�Ƿ��ѽ���
        int ctp_request_id_ = 0;
        string cur_req_type_; // ��ǰ�������ͣ�������OnRspError�н������ԡ�
        int64_t date_ = 0; // ��ǰ�����գ���½�ɹ����ȡ
        CThostFtdcTraderApi* api_ = nullptr;

    };
}
