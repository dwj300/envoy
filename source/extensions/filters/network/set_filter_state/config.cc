#include "source/extensions/filters/network/set_filter_state/config.h"

#include <string>

#include "envoy/buffer/buffer.h"
#include "envoy/extensions/filters/network/set_filter_state/v3/set_filter_state.pb.h"
#include "envoy/extensions/filters/network/set_filter_state/v3/set_filter_state.pb.validate.h"
#include "envoy/formatter/substitution_formatter.h"
#include "envoy/registry/registry.h"

#include "source/common/protobuf/utility.h"
#include "source/extensions/filters/network/common/factory_base.h"
#include "source/server/generic_factory_context.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace SetFilterState {

Network::FilterStatus SetFilterState::onData(Buffer::Instance&, bool) {
  return Network::FilterStatus::Continue;
}

Network::FilterStatus SetFilterState::onNewConnection() {
  // If this is not an SSL connection, do no further checking.
  if (!read_callbacks_->connection().ssl()) {
    ENVOY_LOG(debug, "Connection is not SSL, continuing.");
    return Network::FilterStatus::Continue;
  }

  // Otherwise we need to wait for handshake to be complete before proceeding.
  ENVOY_LOG(debug, "Connection is SSL, pausing.");
  return Network::FilterStatus::StopIteration;
}

void SetFilterState::onEvent(Network::ConnectionEvent event) {
  if (event != Network::ConnectionEvent::Connected) {
    return;
  }

  ENVOY_LOG(debug, "Got connected event, updating filter state and continuing.");
  config_->updateFilterState({}, read_callbacks_->connection().streamInfo());
  read_callbacks_->continueReading();
}

/**
 * Config registration for the filter. @see NamedNetworkFilterConfigFactory.
 */
class SetFilterStateConfigFactory
    : public Common::FactoryBase<
          envoy::extensions::filters::network::set_filter_state::v3::Config> {
public:
  SetFilterStateConfigFactory() : FactoryBase("envoy.filters.network.set_filter_state") {}

private:
  Network::FilterFactoryCb createFilterFactoryFromProtoTyped(
      const envoy::extensions::filters::network::set_filter_state::v3::Config& proto_config,
      Server::Configuration::FactoryContext& context) override {
    auto filter_config(std::make_shared<Filters::Common::SetFilterState::Config>(
        proto_config.on_new_connection(), StreamInfo::FilterState::LifeSpan::Connection, context));
    return [filter_config](Network::FilterManager& filter_manager) -> void {
      filter_manager.addReadFilter(std::make_shared<SetFilterState>(filter_config));
    };
  }

  bool isTerminalFilterByProtoTyped(
      const envoy::extensions::filters::network::set_filter_state::v3::Config&,
      Server::Configuration::ServerFactoryContext&) override {
    return false;
  }
};

REGISTER_FACTORY(SetFilterStateConfigFactory,
                 Server::Configuration::NamedNetworkFilterConfigFactory);

} // namespace SetFilterState
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy
