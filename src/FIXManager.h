/*!
 *  Copyright(c)2018, Arne Gockeln. All rights reserved.
 *  http://www.arnegockeln.com
 */
#ifndef IDEFIX_FIXMANAGER_H
#define IDEFIX_FIXMANAGER_H

#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <utility>
#include <quickfix/Application.h>
#include <quickfix/FileLog.h>
#include <quickfix/FileStore.h>
#include <quickfix/fix44/CollateralInquiryAck.h>
#include <quickfix/fix44/CollateralReport.h>
#include <quickfix/fix44/ExecutionReport.h>
#include <quickfix/fix44/MarketDataRequestReject.h>
#include <quickfix/fix44/MarketDataSnapshotFullRefresh.h>
#include <quickfix/fix44/PositionReport.h>
#include <quickfix/fix44/RequestForPositionsAck.h>
#include <quickfix/fix44/SecurityList.h>
#include <quickfix/fix44/TradingSessionStatus.h>
#include <quickfix/fix44/AllocationReportAck.h>
#include <quickfix/fix44/AllocationReport.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/Session.h>
#include <quickfix/SessionID.h>
#include <quickfix/SessionSettings.h>
#include <quickfix/SocketInitiator.h>
#include <quickfix/Mutex.h>
#include "RequestId.h"
#include "Market.h"
#include "MarketOrder.h"
#include "MarketSnapshot.h"
#include "MarketDetail.h"
#include "FXCMFields.h"
#include "FIXFactory.h"
#include "Account.h"
#include "TradeMath.h"
#include <cmath>

using namespace std;
using namespace FIX;

// subscribe to a different pair than EUR/USD
// EUR/USD is subscribed per default. Set to eurusd,
// if you want to buy and sell in EUR/USD
#define SUBSCRIBE_PAIR "USD/JPY"
// change this only if the account currency is other than
// EUR or USD
#define BASE_PAIR "EUR/USD"

namespace IDEFIX {
  // FOR DEBUGGING! The following pair is un-/subscribed and traded

class FIXManager: public MessageCracker, public Application {
private:
  // for debugging
  bool m_debug_toggle_snapshot_output;

  // Synchronizing multithreading
  mutable FIX::Mutex m_mutex;

  // Pointer to SessionSettings from SessionSettingsFile
  SessionSettings *m_psettings;
  // Pointer to File Store Factory
  FileStoreFactory *m_pstore_factory;
  // Pointer to File Log Factory
  FileLogFactory *m_plog_factory;
  // Pointer to Socket
  SocketInitiator *m_pinitiator;
  // RequestID Manager
  RequestId m_reqid_manager;

  // the session id for market data, such as tick prices
  SessionID m_market_sessionID;
  // the session id for order management, such as open/closing positions
  SessionID m_order_sessionID;
  // The account
  IDEFIX::Account m_account;

  // hold all market snapshots per symbol list[symbol] = Market
  map<string, Market> m_list_market;
  // hold all open market positions list[posid] = marketOrder
  map<string, MarketOrder> m_list_marketorders;
  // hold system parameters list[key] = value
  map<string, string> m_system_params;
  // hold all market details list[symbol] = MarketDetail
  map<string, MarketDetail> m_market_details;
  
public:
  FIXManager();
  FIXManager(const string settingsFile);

  void onCreate(const SessionID& sessionID);
  void onLogon(const SessionID& sessionID);
  void onLogout(const SessionID& session_ID);
  void toAdmin(Message& message, const SessionID& session_ID);
  void toApp(Message &message, const SessionID &session_ID)
  throw( FIX::DoNotSend );
  void fromAdmin(const Message& message, const SessionID& session_ID)
  throw( FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon );
  void fromApp(const Message& message, const SessionID& session_ID)
  throw( FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::UnsupportedMessageType );

  void onMessage(const FIX44::TradingSessionStatus& tss, const SessionID& session_ID);
  void onMessage(const FIX44::CollateralInquiryAck& ack, const SessionID& session_ID);
  void onMessage(const FIX44::CollateralReport& cr, const SessionID& session_ID);
  void onMessage(const FIX44::RequestForPositionsAck& ack, const SessionID& session_ID);
  void onMessage(const FIX44::PositionReport& pr, const SessionID& session_ID);
  void onMessage(const FIX44::MarketDataRequestReject& mdr, const SessionID& session_ID);
  void onMessage(const FIX44::MarketDataSnapshotFullRefresh& mds, const SessionID& session_ID);
  void onMessage(const FIX44::ExecutionReport& er, const SessionID& session_ID);
  void onMessage(const FIX44::AllocationReportAck& ack, const SessionID& session_ID);
  void onMessage(const FIX44::AllocationReport& ar, const SessionID& session_ID);

  void startSession(const string settingsfile);
  void endSession();

  // All Message Query Methods
  void queryTradingStatus();
  void queryAccounts();
  void queryPositionReport(const FIX::PosReqType type = PosReqType_POSITIONS);
  void queryOrderMassStatus();

  void subscribeMarketData(const std::string symbol);
  void unsubscribeMarketData(const std::string symbol);
  
  void marketOrder(const MarketOrder& marketOrder, const FIXFactory::SingleOrderType orderType = FIXFactory::SingleOrderType::MARKET_ORDER);

  void closeAllPositions(const string symbol);
  void closePosition(const MarketOrder& marketOrder);
  void closeWinners(const string symbol);
  void closeLoosers(const string symbol);

  // Public Getter & Setter
  MarketSnapshot getLatestSnapshot(const string symbol);
  MarketDetail getMarketDetails(const std::string& symbol);
  Account getAccount();
  string getAccountID() const;

  void debug();
  void toggleSnapshotOutput();
  
  void showSysParamList();
  void showAvailableMarketList();
  void showMarketDetail(const string symbol);

  string formatBaseCurrency(const double value);

private:
  void onInit();
  void onExit();

  void updatePrices(const MarketSnapshot& snapshot);
  string nextRequestID();
  string nextOrderID();

  const FIX::Dictionary* getSessionSettingsPtr(const SessionID& session_ID);
  bool isMarketDataSession(const SessionID& session_ID);
  bool isOrderSession(const SessionID& session_ID);
  
  void setAccount(const Account account);

  SessionID getMarketSessionID() const;
  void setMarketSessionID(const SessionID& session_ID);

  SessionID getOrderSessionID() const;
  void setOrderSessionID(const SessionID& session_ID);

  Market getMarket(const string symbol);
  void addMarket(const Market market);
  void addMarketSnapshot(const MarketSnapshot snapshot);
  void addMarketOrder(const MarketOrder marketOrder);
  void removeMarketOrder(const string posID);
  void updateMarketOrder(const MarketOrder& marketOrder, const bool isUnsolicited = false);

  MarketOrder getMarketOrder(const string fxcm_pos_id) const;
  MarketOrder getMarketOrder(const ClOrdID clOrdID) const;

  void addMarketDetail(const MarketDetail& marketDetail);

  void addSysParam(const string key, const string value);
  string getSysParam(const string key);
}; // class fixmanager
}; // namespace idefix

#endif //IDEFIX_FIXMANAGER_H
