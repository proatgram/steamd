#include "Steamworks.h"
#include "steamd-server-glue.hpp"

namespace steamd {
    class Steamd final : public sdbus::AdaptorInterfaces<io::github::proatgram::steamd::ClientUser_adaptor> {
        public:
            Steamd(sdbus::IConnection & dbusConnection, sdbus::ObjectPath objectPath);
            ~Steamd();

        protected:
            auto log_on_with_steamid(const uint64_t& steamId) -> void final;
            auto log_on_with_credentials(const std::string& username, const std::string& password, const bool &rememberInfo) -> void final;
            auto try_cache_log_on(const std::string &username) -> void final;
            auto log_off() -> void final;
            auto get_log_on_state() -> int32_t final;
            auto is_logged_on() -> bool final;
            auto is_connected() -> bool final;
            auto is_trying_to_log_on() -> bool final;

        private:
            STEAM_CALLBACK(Steamd, OnSteamServerConnectionFailure, SteamServerConnectFailure_t, m_cbSteamServerConnectFailure);
            STEAM_CALLBACK(Steamd, OnSteamServerConnected, SteamServersConnected_t, m_cbSteamServersConnected);
            STEAM_CALLBACK(Steamd, OnSteamServerDisconnected, SteamServersDisconnected_t, m_cbSteamServersDiconnected);

            IClientEngine *m_steamClientEngine;
            IClientUser *m_steamClientUser;
            IClientFriends *m_steamClientFriends;

            HSteamPipe m_steamPipeHandle;
            HSteamUser m_steamUserHandle;

            std::thread m_callbacksRunner;
            std::atomic_bool m_done;
            static auto RunSteamCallbacks(Steamd *context) -> void;
    };
}  // namespace steamd
