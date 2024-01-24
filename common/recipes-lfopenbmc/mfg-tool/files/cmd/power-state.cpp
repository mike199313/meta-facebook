#include "utils/dbus.hpp"
#include "utils/json.hpp"
#include "utils/mapper.hpp"
#include "utils/register.hpp"

#include <phosphor-logging/lg2.hpp>
#include <sdbusplus/async.hpp>

namespace mfgtool::cmds::power_state
{
PHOSPHOR_LOG2_USING;

struct command
{
    void init(CLI::App& app)
    {
        auto cmd = app.add_subcommand("power-state", "Get state of device");
        init_callback(cmd, *this);
    }

    auto run(sdbusplus::async::context& ctx) -> sdbusplus::async::task<>
    {
        using namespace dbuspath;
        using utils::mapper::subtree_services;

        debug("Finding chasses.");
        auto chasses = co_await subtree_services(ctx, chassis::ns_path,
                                                 chassis::Proxy::interface);

        debug("Finding hosts.");
        auto hosts = co_await subtree_services(ctx, host::ns_path,
                                               host::Proxy::interface);

        auto result = R"({})"_json;

        debug("Iterating over chasses.");
        for (auto& [chassis, services] : chasses)
        {
            if (services.size() == 0)
            {
                error("No services found for {PATH}.", "PATH", chassis);
                continue;
            }
            if (services.size() > 1)
            {
                warning("Multiple services ({COUNT}) provide {PATH}.", "PATH",
                        chassis, "COUNT", services.size());
                for (auto& s : services)
                {
                    warning("Service available at {SERVICE}.", "SERVICE", s);
                };
            }

            static auto pathPrefix = chassis::path_prefix();
            if (chassis.str.size() <= pathPrefix.size())
            {
                error("Unexpected chassis path found: {PATH}", "PATH", chassis);
                continue;
            }

            debug("Getting power state for {PATH}.", "PATH", chassis);
            auto proxy =
                chassis::Proxy(ctx).service(services[0]).path(chassis.str);
            auto state = (chassis::Proxy::PowerState::Off ==
                          co_await proxy.current_power_state())
                             ? "off"
                             : "on";

            info("State for {PATH}: {STATE}", "PATH", chassis, "STATE", state);
            auto id = chassis.str.substr(pathPrefix.size());
            result[id]["standby"] = state;
        }

        debug("Iterating over hosts.");
        for (auto& [host, services] : hosts)
        {
            if (services.size() == 0)
            {
                error("No services found for {PATH}.", "PATH", host);
                continue;
            }
            if (services.size() > 1)
            {
                warning("Multiple services ({COUNT}) provide {PATH}.", "PATH",
                        host, "COUNT", services.size());
                for (auto& s : services)
                {
                    warning("Service available at {SERVICE}.", "SERVICE", s);
                };
            }

            static auto pathPrefix = host::path_prefix();
            if (host.str.size() <= pathPrefix.size())
            {
                error("Unexpected chassis path found: {PATH}", "PATH", host);
                continue;
            }

            debug("Getting power state for {PATH}.", "PATH", host);
            auto proxy = host::Proxy(ctx).service(services[0]).path(host.str);
            auto state = (host::Proxy::HostState::Off ==
                          co_await proxy.current_host_state())
                             ? "off"
                             : "on";

            info("State for {PATH}: {STATE}", "PATH", host, "STATE", state);
            auto id = host.str.substr(pathPrefix.size());
            result[id]["runtime"] = state;
        }

        json::display(result);

        co_return;
    }
};
MFGTOOL_REGISTER(command);

} // namespace mfgtool::cmds::power_state
