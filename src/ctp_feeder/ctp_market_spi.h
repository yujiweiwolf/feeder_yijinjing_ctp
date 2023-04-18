#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <boost/algorithm/string.hpp>

#include <x/x.h>
#include "ctp.h"
#include "ctp_support.h"
#include "define.h"
#include "config.h"

#include "journal/Timer.h"
#include "journal/JournalWriter.h"
#include "common/datastruct.h"


using namespace std;
using namespace x;
using namespace yijinjing;

namespace co {

    class CTPMarketSpi : public CThostFtdcMdSpi {
    public:
        CTPMarketSpi(CThostFtdcMdApi* qApi);
        virtual ~CTPMarketSpi();
        void Init();

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

        ///��¼������Ӧ
        virtual void OnRspUserLogin(CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///��������Ӧ��
        virtual void OnRspSubMarketData(CThostFtdcSpecificInstrumentField *pSpecificInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

        ///�����ѯ�鲥��Լ��Ӧ
        // virtual void OnRspQryMulticastInstrument(CThostFtdcMulticastInstrumentField* pMulticastInstrument, CThostFtdcRspInfoField* pRspInfo, int nRequestID, bool bIsLast);

        ///�������֪ͨ
        virtual void OnRtnDepthMarketData(CThostFtdcDepthMarketDataField *pDepthMarketData);

        ///����Ӧ��
        virtual void OnRspError(CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast);

    protected:
        int next_id();
        void ReqUserLogin();
        void ReqSubMarketData();

    private:
        int ctp_request_id_ = 0;
        CThostFtdcMdApi* api_ = nullptr;

        map<string, int> sub_status_;
        int64_t local_date_ = 0;
        int64_t local_next_date_ = 0;
        JournalWriterPtr feeder_writer_;
        std::map<string, QTickT> all_ticks_;
    };
}
