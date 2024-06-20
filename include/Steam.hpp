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
            auto get_two_factor_details() -> void final;
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
            auto get_user_config_folder() -> std::tuple<bool, std::string> final;
            auto get_account_name() -> std::tuple<bool, std::string> final;
            auto get_account_name(const sdbus::Struct<uint32_t, uint32_t, int32_t, int32_t>& steamId) -> std::tuple<bool, std::string> final;
            auto get_player_steam_level() -> int32_t final;
            auto set_account_limited(const bool &limited) -> void final;
            auto get_account_limited() -> bool final;
            auto set_limited_account_can_invite_friends(const bool &can_invite_friends) -> void final;
            auto get_limited_account_can_invite_friends() -> bool final;
            auto is_support_user() -> bool final;
            auto needs_ssa_next_logon() -> bool final;
            auto clear_needs_ssa_next_logon() -> void final;
            auto get_wallet_balance() -> std::tuple<bool, bool, sdbus::Struct<int32_t, int32_t>, sdbus::Struct<int32_t, int32_t>> final;
            // Needs further testing to determine if it should be a signal or no
            auto prompt_to_verify_email() -> bool final;
            // Needs further testing to determine if it should be a signal or no
            auto prompt_to_change_password() -> bool final;
            auto account_has_extra_security() -> bool final;
            auto account_should_show_lock_ui() -> bool final;
            auto get_number_authed_computers() -> int32_t final;
            auto get_steam_guard_enabled_time() -> uint32_t final;
            auto get_steam_guard_history(const uint32_t &numberEntries) -> std::vector<sdbus::Struct<bool, uint32_t, uint32_t, std::vector<uint8_t>, bool, std::string, std::string>> final;
            auto set_steam_guard_new_machine_dialog_response(const bool &isApproved, const bool &isWizardComplete) -> void final;
            auto get_steam_guard_details() -> void final;
            auto set_phone_is_verified(const bool &isVerified) -> void final;
            auto is_phone_verified() -> bool final;
            auto set_phone_is_identifying(const bool &isIdentifying) -> void final;
            auto is_phone_identifying() -> bool final;
            auto set_is_phone_requiring_verification(const bool &phoneRequiresVerification) -> void final;
            auto is_phone_requiring_verification() -> bool final;
            auto change_two_factor_authentication_options(const int32_t &option) -> void final;
            auto set_user_machine_name(const std::string &machineName) -> void final;
            auto get_user_machine_name() -> std::tuple<bool, std::string> final;
            auto get_email_domain_from_logon_failure() -> std::tuple<bool, std::string> final;
            auto request_notifications() -> void final;
            auto get_recovery_email() -> std::tuple<bool, std::string> final;
            auto can_logon_offline_mode() -> bool final;
            auto log_on_offline_mode() -> int32_t final;
            auto get_offline_logon_ticket(const std::string &username) -> std::tuple<bool, sdbus::Struct<uint32_t, uint32_t>> final;
            auto validate_offline_logon_ticket(const std::string &unknown) -> int32_t final;
            auto is_other_session_playing() -> std::tuple<bool, uint32_t> final;
            auto kick_other_playing_session() -> bool final;
            auto signal_apps_to_shut_down(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> bool final;
            auto terminate_app_multi_step(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId, const uint32_t& unknown) -> bool final;
            auto is_game_running(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> bool final;
            auto is_any_game_running() -> bool final;
            auto get_number_games_running() -> int32_t final;
            auto get_running_game_id(const int32_t &game) -> sdbus::Struct<uint32_t, uint32_t, uint32_t> final;
            auto get_running_game_pid(const int32_t &game) -> int32_t final;
            auto is_game_window_ready(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> bool final;
            auto raise_window_for_game(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> int32_t final;
            auto is_app_overlay_enabled(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> bool final;
            auto overlay_ignore_child_processes(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> bool final;

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
