// Copyright (c) YugaByte, Inc.

#ifndef YB_CLIENT_ASYNC_RPC_H_
#define YB_CLIENT_ASYNC_RPC_H_

#include "yb/rpc/messenger.h"
#include "yb/rpc/rpc.h"
#include "yb/tserver/tserver_service.proxy.h"
#include "yb/common/redis_protocol.pb.h"

namespace yb {
namespace client {

class YBTable;

namespace internal {

class Batcher;
struct InFlightOp;
class RemoteTablet;
class RemoteTabletServer;

// An Async RPC which is in-flight to a tablet. Initially, the RPC is sent
// to the leader replica, but it may be retried with another replica if the
// leader fails.
//
// Keeps a reference on the owning batcher while alive. It doesn't take a generic callback,
// but ProcessResponseFromTserver will update the state after getting the end response.
// This class deletes itself after Rpc returns and is processed.
class AsyncRpc : public rpc::Rpc {
 public:
  AsyncRpc(const scoped_refptr<Batcher> &batcher,
           RemoteTablet *const tablet,
           vector<InFlightOp*> ops,
           const MonoTime &deadline,
           const std::shared_ptr<rpc::Messenger> &messenger);

  virtual ~AsyncRpc();

  virtual void SendRpc() OVERRIDE;
  virtual string ToString() const OVERRIDE;

  const YBTable* table() const;
  const RemoteTablet* tablet() const { return tablet_; }
  const vector<InFlightOp*>& ops() const { return ops_; }

 protected:
  static scoped_refptr<Histogram> metric_rpc_send_;

  // Called when we finish a lookup (to find the new consensus leader). Retries
  // the rpc after a short delay.
  void LookupTabletCb(const Status& status);

  // Marks all replicas on current_ts_ as failed and retries the write on a
  // new replica.
  void FailToNewReplica(const Status& reason);

  virtual void SendRpcCb(const Status& status) OVERRIDE;

  // Called when we finish initializing a TS proxy.
  // Sends the RPC, provided there was no error.
  void InitTSProxyCb(const Status& status);

  virtual Status response_error_status() = 0;

  virtual void SendRpcToTserver() = 0;

  // This is the last step where errors and responses are collected from the response and
  // stored in batcher. If there's a callback from the user, it is done in this step.
  virtual void ProcessResponseFromTserver(Status status) = 0;

  virtual void MarkOpsAsFailed() = 0;

  virtual scoped_refptr<Histogram> metrics_entry_to_update() = 0;

  // Pointer back to the batcher. Processes the write response when it
  // completes, regardless of success or failure.
  scoped_refptr<Batcher> batcher_;

  // The trace buffer.
  scoped_refptr<Trace> trace_;

  // The tablet that should receive this write.
  RemoteTablet* const tablet_;

  // The TS receiving the write. May change if the write is retried.
  RemoteTabletServer* current_ts_;

  // TSes that refused the write because they were followers at the time.
  // Cleared when new consensus configuration information arrives from the master.
  std::set<RemoteTabletServer*> followers_;

  // Operations which were batched into this RPC.
  // These operations are in kRequestSent state.
  vector<InFlightOp*> ops_;

  MonoTime start_;
};

class WriteRpc : public AsyncRpc {
 public:
  WriteRpc(const scoped_refptr<Batcher>& batcher,
           RemoteTablet* const tablet,
           vector<InFlightOp*> ops,
           const MonoTime& deadline,
           const std::shared_ptr<rpc::Messenger>& messenger);

  virtual ~WriteRpc();

  const tserver::WriteResponsePB& resp() const { return resp_; }

  static void InitializeMetric(const scoped_refptr<MetricEntity>& entity);

 protected:
  void SendRpcToTserver() OVERRIDE;

  Status response_error_status() OVERRIDE;

  void ProcessResponseFromTserver(Status status) OVERRIDE;

  void MarkOpsAsFailed() OVERRIDE;

 private:
  scoped_refptr<Histogram> metrics_entry_to_update() override { return rpc_metric_; }

  static scoped_refptr<Histogram> rpc_metric_;

  // Request body.
  tserver::WriteRequestPB req_;

  // Response body.
  tserver::WriteResponsePB resp_;
};

class ReadRpc : public AsyncRpc {
 public:
  ReadRpc(
      const scoped_refptr<Batcher>& batcher, RemoteTablet* const tablet, vector<InFlightOp*> ops,
      const MonoTime& deadline, const std::shared_ptr<rpc::Messenger>& messenger);
  virtual ~ReadRpc();

  const tserver::ReadResponsePB& resp() const { return resp_; }

  static void InitializeMetric(const scoped_refptr<MetricEntity>& entity);

 protected:
  void SendRpcToTserver() OVERRIDE;

  void ProcessResponseFromTserver(Status status) OVERRIDE;

  Status response_error_status() OVERRIDE;

  void MarkOpsAsFailed() OVERRIDE;

 private:
  scoped_refptr<Histogram> metrics_entry_to_update() override { return rpc_metric_; }

  static scoped_refptr<Histogram> rpc_metric_;

  // Request body.
  tserver::ReadRequestPB req_;

  // Response body.
  tserver::ReadResponsePB resp_;
};

}  // namespace internal
}  // namespace client
}  // namespace yb

#endif  // YB_CLIENT_ASYNC_RPC_H_
