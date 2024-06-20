#include "Steamworks.h"
#include "steamd-server-glue.hpp"

namespace steamd {
    class Steamd final : public sdbus::AdaptorInterfaces<io::github::proatgram::steamd::ClientUser_adaptor> {
        public:
            Steamd(sdbus::IConnection & dbusConnection, sdbus::ObjectPath objectPath);
            ~Steamd();

        protected:
            auto log_on_with_steamid(const sdbus::Struct<uint32_t, uint32_t, int32_t, int32_t>& steamId) -> void final;
            auto log_on_with_credentials(const std::string& username, const std::string& password, const bool &rememberInfo) -> void final;
            auto try_cache_log_on(const std::string &username) -> void final;
            auto log_on_with_two_factor(const std::string& username, const std::string& password, const bool &rememberInfo, const std::string& twoFactorCode) -> void final;
            auto log_on_with_two_factor(const std::string& two_factor_code) -> void final;
            auto log_on_with_steam_guard_code(const std::string& username, const std::string& password, const bool &rememberInfo, const bool &rememberComputer, const std::string& steamGuardCode) -> void final;
            auto log_on_with_steam_guard_code(const bool &rememberComputer, const std::string& steamGuardCode) -> void final;
            auto send_validation_email() -> void final;
            auto log_off() -> void final;
            auto get_log_on_state() -> int32_t final;
            auto has_two_factor() -> bool final;
            auto is_logged_on() -> bool final;
            auto is_connected() -> bool final;
            auto is_trying_to_log_on() -> bool final;
            auto has_cached_credentials(const std::string &username) -> bool final;
            auto destroy_cached_credentials(const std::string &username) -> int32_t final;
            auto is_password_remembered() -> bool final;
            auto get_account_security_policy_flags() -> uint32_t final;
            auto get_client_instance() -> uint32_t final;
            auto is_VAC_banned(const uint32_t &appId) -> bool final;
            auto is_behind_nat() -> bool final;
            auto get_email() -> std::tuple<bool, bool, std::string> final;
            auto set_email(const std::string &email) -> bool final;
            auto get_user_base_folder() -> std::string final;
            auto get_user_data_folder(const sdbus::Struct<uint32_t, uint32_t, uint32_t>& gameId) -> std::tuple<bool, std::string> final;

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
