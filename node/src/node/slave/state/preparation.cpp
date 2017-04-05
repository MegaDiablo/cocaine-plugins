#include "cocaine/detail/service/node/slave/state/preparation.hpp"

#include <boost/algorithm/string/join.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <blackhole/logger.hpp>

#include <cocaine/repository.hpp>
#include <cocaine/repository/isolate.hpp>
#include <cocaine/rpc/actor.hpp>
#include <cocaine/trace/trace.hpp>
#include <cocaine/detail/service/node/slave/spawn_handle.hpp>

#include "cocaine/api/isolate.hpp"
#include "cocaine/service/node/slave/id.hpp"
#include "cocaine/service/node/slave/error.hpp"

#include "cocaine/detail/service/node/slave/machine.hpp"
#include "cocaine/detail/service/node/slave/control.hpp"
#include "cocaine/detail/service/node/slave/spawn_handle.hpp"
#include "cocaine/detail/service/node/slave/state/spawn.hpp"

namespace cocaine {
namespace detail {
namespace service {
namespace node {
namespace slave {
namespace state {

namespace ph = std::placeholders;

using asio::ip::tcp;

preparation_t::preparation_t(std::shared_ptr<machine_t> slave_) :
    slave(std::move(slave_))
{}

auto preparation_t::name() const noexcept -> const char* {
    return "preparation";
}

auto preparation_t::terminate(const std::error_code& ec) -> void {
    slave->shutdown(ec);
}

auto preparation_t::start(std::chrono::milliseconds) -> void {
    try {
        COCAINE_LOG_DEBUG(slave->log, "preparation start");
        slave->auth->token([=](api::authentication_t::token_t token, const std::error_code& ec) {
            COCAINE_LOG_DEBUG(slave->log, "preparation got token: {}", ec);
            slave->loop.post([=] {
                on_refresh(token, ec);
            });
        });
    } catch (const std::exception& err) {
        COCAINE_LOG_ERROR(slave->log, "failed to start preparation state: {}", err.what());
        slave->loop.post([=] {
            terminate(make_error_code(error::unknown_activate_error));
        });
    }
}

auto preparation_t::on_refresh(api::authentication_t::token_t token, const std::error_code& ec) -> void {
    if (ec) {
        terminate(ec);
        return;
    }

    auto spawning = std::make_shared<spawn_t>(slave);
    slave->migrate(spawning);
    spawning->spawn(token, slave->profile.timeout.spawn);
}

}  // namespace state
}  // namespace slave
}  // namespace node
}  // namespace service
}  // namespace detail
}  // namespace cocaine
