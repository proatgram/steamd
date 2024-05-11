#include "Steam.hpp"

auto main(int argc, char **argv) -> int {
    sdbus::ServiceName serviceName{"io.github.proatgram.steamd"};
    std::unique_ptr<sdbus::IConnection> connection = sdbus::createBusConnection(serviceName);

    sdbus::ObjectPath objectPath{"/io/github/proatgram/steamd"};
    steamd::Steamd steamdInstance{*connection, std::move(objectPath)};

    connection->enterEventLoop();

    return 0;
}
