#pragma once

#include "envoy/server/filter_config.h"

#include "source/extensions/filters/common/set_filter_state/filter_config.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SetFilterState {

class SetFilterState : public Network::ReadFilter,
                       public Network::ConnectionCallbacks,
                       Logger::Loggable<Logger::Id::filter> {
public:
  explicit SetFilterState(const Filters::Common::SetFilterState::ConfigSharedPtr config)
      : config_(config) {}

  // Network::ReadFilter
  Network::FilterStatus onData(Buffer::Instance& data, bool end_stream) override;
  Network::FilterStatus onNewConnection() override { return Network::FilterStatus::Continue; };

  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) override {
    read_callbacks_ = &callbacks;
    read_callbacks_->connection().addConnectionCallbacks(*this);
  }

  // Network::ConnectionCallbacks
  void onEvent(Network::ConnectionEvent event) override;
  void onAboveWriteBufferHighWatermark() override {}
  void onBelowWriteBufferLowWatermark() override {}

private:
  // State of this filter.
  enum class Status { NotStarted, Waiting, Complete };

  const Filters::Common::SetFilterState::ConfigSharedPtr config_;
  Network::ReadFilterCallbacks* read_callbacks_{};
  Status status_{Status::NotStarted};
};

} // namespace SetFilterState
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
