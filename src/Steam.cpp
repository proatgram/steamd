#include "Steam.hpp"
#include "Imported/SteamIPAddress.hpp"
#include "Types/CGameID.h"

#include <climits>

using namespace steamd;

/*
 * All DBus structs are documented in the DBus XML stub
 */

Steamd::Steamd(sdbus::IConnection &dbusConnection, sdbus::ObjectPath objectPath) :
    AdaptorInterfaces(dbusConnection, std::move(objectPath)),
    m_cbSteamServerConnectFailure(this, &Steamd::OnSteamServerConnectionFailure),
    m_cbSteamServersConnected(this, &Steamd::OnSteamServerConnected),
    m_cbSteamServersDiconnected(this, &Steamd::OnSteamServerDisconnected),
    m_done(false)
{
    // Initialize Steam Client
    if (!OpenAPI_LoadLibrary()) {
        throw std::runtime_error("Unable to load steamclient library.");
    }

    m_steamClientEngine = static_cast<IClientEngine*>(SteamInternal_CreateInterface(CLIENTENGINE_INTERFACE_VERSION));
    m_steamUserHandle = m_steamClientEngine->CreateLocalUser(&m_steamPipeHandle, k_EAccountTypeIndividual);
    m_steamClientUser = static_cast<IClientUser*>(m_steamClientEngine->GetIClientUser(m_steamUserHandle, m_steamPipeHandle));
    m_steamClientFriends = static_cast<IClientFriends*>(m_steamClientEngine->GetIClientFriends(m_steamUserHandle, m_steamPipeHandle));

    m_callbacksRunner = std::thread(Steamd::RunSteamCallbacks, this);

    registerAdaptor();
}

Steamd::~Steamd() {
    unregisterAdaptor();

    m_done.store(true);

    m_callbacksRunner.join();
}

auto Steamd::log_on_with_steamid(const sdbus::Struct<uint32_t, uint32_t, int32_t, int32_t>& steamId) -> void {
    CSteamID id = CSteamID(steamId.get<0>(), steamId.get<1>(), static_cast<EUniverse>(steamId.get<3>()), static_cast<EAccountType>(steamId.get<2>()));
    m_steamClientUser->LogOn(id);
}

auto Steamd::log_on_with_credentials(const std::string &username, const std::string &password, const bool &rememberInfo) -> void {
    m_steamClientUser->SetLoginInformation(username.c_str(), password.c_str(), rememberInfo);
    m_steamClientUser->LogOn(m_steamClientUser->GetSteamID());
}

auto Steamd::log_on_with_two_factor_with_credentials(const std::string& username, const std::string& password, const bool &rememberInfo, const std::string& twoFactorCode) -> void {
    m_steamClientUser->SetLoginInformation(username.c_str(), password.c_str(), rememberInfo);
    m_steamClientUser->SetTwoFactorCode(twoFactorCode.c_str());
    m_steamClientUser->LogOn(m_steamClientUser->GetSteamID());
}

auto Steamd::log_on_with_two_factor(const std::string &twoFactorCode) -> void {
    m_steamClientUser->SetTwoFactorCode(twoFactorCode.c_str());
    m_steamClientUser->LogOn(m_steamClientUser->GetSteamID());
}

auto Steamd::log_on_with_steam_guard_code_with_credentials(const std::string& username, const std::string& password, const bool &rememberInfo, const bool &rememberComputer, const std::string& steamGuardCode) -> void {
    m_steamClientUser->SetLoginInformation(username.c_str(), password.c_str(), rememberInfo);
    m_steamClientUser->Set2ndFactorAuthCode(steamGuardCode.c_str(), !rememberComputer);
    m_steamClientUser->LogOn(m_steamClientUser->GetSteamID());
}

auto Steamd::log_on_with_steam_guard_code(const bool &rememberComputer, const std::string &steamGuardCode) -> void {
    m_steamClientUser->Set2ndFactorAuthCode(steamGuardCode.c_str(), !rememberComputer);
    m_steamClientUser->LogOn(m_steamClientUser->GetSteamID());
}

auto Steamd::send_validation_email() -> void {
    m_steamClientUser->SendValidationEmail();
}

auto Steamd::try_cache_log_on(const std::string &username) -> void {
    m_steamClientUser->SetAccountNameForCachedCredentialLogin(username.c_str(), false);
    m_steamClientUser->LogOn(m_steamClientUser->GetSteamID());
}

auto Steamd::log_off() -> void {
    m_steamClientUser->LogOff();
}

auto Steamd::get_log_on_state() -> int32_t {
    return static_cast<int32_t>(m_steamClientUser->GetLogonState());
}

auto Steamd::has_two_factor() -> bool {
    return m_steamClientUser->BHasTwoFactor();
}

auto Steamd::get_two_factor_details() -> void {
    m_steamClientUser->GetTwoFactorDetails();
}

auto Steamd::is_logged_on() -> bool {
    return m_steamClientUser->BLoggedOn();
}

auto Steamd::is_connected() -> bool {
    return m_steamClientUser->BConnected();
}

auto Steamd::is_trying_to_log_on() -> bool {
    return m_steamClientUser->BTryingToLogin();
}

auto Steamd::has_cached_credentials(const std::string &username) -> bool {
    return m_steamClientUser->BHasCachedCredentials(username.c_str());
}

auto Steamd::destroy_cached_credentials(const std::string &username) -> int32_t {
    return m_steamClientUser->DestroyCachedCredentials(username.c_str());
}

auto Steamd::is_password_remembered() -> bool {
    return m_steamClientUser->IsPasswordRemembered();
}

auto Steamd::get_account_security_policy_flags() -> uint32_t {
    return m_steamClientUser->GetAccountSecurityPolicyFlags();
}

auto Steamd::get_client_instance() -> uint32_t {
    return m_steamClientUser->GetClientInstanceID();
}

auto Steamd::is_VAC_banned(const uint32_t &appId) -> bool {
    return m_steamClientUser->IsVACBanned(appId);
}

auto Steamd::is_behind_nat() -> bool {
    return m_steamClientUser->BIsBehindNAT();
}

auto Steamd::get_email() -> std::tuple<bool, bool, std::string> {
    bool validated{};
    std::string email(320, '\0');

    bool result = m_steamClientUser->GetEmail(email.data(), 320, &validated);

    std::size_t index = email.find('\0');
    if (index != std::string::npos) {
        email.resize(index);
    }

    return std::make_tuple(result, validated, email);
}

auto Steamd::set_email(const std::string &email) -> bool {
    return m_steamClientUser->SetEmail(email.c_str());
}

auto Steamd::get_user_base_folder() -> std::string {
    return m_steamClientUser->GetUserBaseFolder();
}

auto Steamd::get_user_data_folder(const sdbus::Struct<uint32_t, uint32_t, uint32_t>& gameId) -> std::tuple<bool, std::string> {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    std::string path = std::string(PATH_MAX, '\0');

    bool result = m_steamClientUser->GetUserDataFolder(game, path.data(), PATH_MAX);
    std::size_t index = path.find('\0');
    if (index != std::string::npos) {
        path.resize(index);
    }

    return std::make_tuple(result, path);
}

auto Steamd::get_user_config_folder() -> std::tuple<bool, std::string> {
    std::string path = std::string(PATH_MAX, '\0');

    bool result = m_steamClientUser->GetUserConfigFolder(path.data(), PATH_MAX);
    
    std::size_t index = path.find('\0');
    if (index != std::string::npos) {
        path.resize(index);
    }

    return std::make_tuple(result, path);
}

auto Steamd::get_account_name() -> std::tuple<bool, std::string> {
    std::string accountName = std::string(32, '\0');
    
    bool result = m_steamClientUser->GetAccountName(accountName.data(), 128);
    
    std::size_t index = accountName.find('\0');
    if (index != std::string::npos) {
        accountName.resize(index);
    }

    return std::make_tuple(result, accountName);
}
auto Steamd::get_account_name_by_steam_id(const sdbus::Struct<uint32_t, uint32_t, int32_t, int32_t>& steamId) -> std::tuple<bool, std::string> {
    CSteamID id = CSteamID(steamId.get<0>(), steamId.get<1>(), static_cast<EUniverse>(steamId.get<3>()), static_cast<EAccountType>(steamId.get<2>()));
    std::string accountName = std::string(32, '\0'); // I don't think spaces are allowed in Steam usernames?

    bool result = m_steamClientUser->GetAccountName(id, accountName.data(), 32);
    
    std::size_t index = accountName.find('\0');
    if (index != std::string::npos) {
        accountName.resize(index);
    }

    return std::make_tuple(result, accountName);
}
auto Steamd::get_player_steam_level() -> int32_t {
    return m_steamClientUser->GetPlayerSteamLevel();
}

auto Steamd::set_account_limited(const bool &limited) -> void {
    m_steamClientUser->SetAccountLimited(limited);
}

auto Steamd::get_account_limited() -> bool {
    return m_steamClientUser->BIsAccountLimited();
}

auto Steamd::set_limited_account_can_invite_friends(const bool &can_invite_friends) -> void {
    m_steamClientUser->SetLimitedAccountCanInviteFriends(can_invite_friends);
}

auto Steamd::get_limited_account_can_invite_friends() -> bool {
    return m_steamClientUser->BLimitedAccountCanInviteFriends();
}

auto Steamd::is_support_user() -> bool {
    return m_steamClientUser->BSupportUser();
}

auto Steamd::needs_ssa_next_logon() -> bool {
    return m_steamClientUser->BNeedsSSANextSteamLogon();
}

auto Steamd::clear_needs_ssa_next_logon() -> void {
    m_steamClientUser->ClearNeedsSSANextSteamLogon();
}

auto Steamd::get_wallet_balance() -> std::tuple<bool, bool, sdbus::Struct<int32_t, int32_t>, sdbus::Struct<int32_t, int32_t>> {
    CAmount balance{};
    CAmount pending{};
    bool hasWallet{};

    bool result = m_steamClientUser->BGetWalletBalance(&hasWallet, &balance, &pending);
    return std::make_tuple(result, hasWallet, sdbus::Struct<int32_t, int32_t>(balance.m_nAmount, balance.m_eCurrencyCode), sdbus::Struct<int32_t, int32_t>(pending.m_nAmount, pending.m_eCurrencyCode));
}

auto Steamd::prompt_to_verify_email() -> bool {
    return m_steamClientUser->BPromptToVerifyEmail();
}

auto Steamd::prompt_to_change_password() -> bool {
    return m_steamClientUser->BPromptToChangePassword();
}

auto Steamd::account_has_extra_security() -> bool {
    return m_steamClientUser->BAccountExtraSecurity();
}

auto Steamd::account_should_show_lock_ui() -> bool {
    return m_steamClientUser->BAccountShouldShowLockUI();
}

auto Steamd::get_number_authed_computers() -> int32_t {
    return m_steamClientUser->GetCountAuthedComputers();
}

auto Steamd::get_steam_guard_enabled_time() -> uint32_t {
    return m_steamClientUser->GetSteamGuardEnabledTime();
}

auto Steamd::get_steam_guard_history(const uint32_t &numberEntries) -> std::vector<sdbus::Struct<bool, uint32_t, uint32_t, std::vector<uint8_t>, bool, std::string, std::string>> {
    std::vector<sdbus::Struct<bool, uint32_t, uint32_t, std::vector<uint8_t>, bool, std::string, std::string>> entries;
    for (uint32_t i = 0; i < numberEntries; i++) {
        RTime32 timestamp{};
        SteamIPAddress_t ipAddress{};
        bool isRemembered{};
        std::string geolocationInfo(512, '\0');
        std::string unknown(512, '\0');

        bool result = m_steamClientUser->GetSteamGuardHistoryEntry(i, &timestamp, &ipAddress, &isRemembered, geolocationInfo.data(), 512, unknown.data(), 512);

        if (!result) {
            break;
        }

        std::size_t index = geolocationInfo.find('\0');
        if (index != std::string::npos) {
            geolocationInfo.resize(index);
        }

        index = unknown.find('\0');
        if (index != std::string::npos) {
            unknown.resize(index);
        }
        
        if (ipAddress.m_eType == ESteamIPType::k_ESteamIPTypeIPv4) {
            entries.push_back(sdbus::Struct<bool, uint32_t, uint32_t, std::vector<uint8_t>, bool, std::string, std::string>(result, timestamp, ipAddress.m_unIPv4, {}, isRemembered, geolocationInfo, unknown));
        }
        else {
            std::vector<uint8_t> ipv6{};
            for (std::size_t i = 0; i < 16; i++) {
                ipv6.push_back(ipAddress.m_rgubIPv6[i]);
            }

            entries.push_back(sdbus::Struct<bool, uint32_t, uint32_t, std::vector<uint8_t>, bool, std::string, std::string>(result, timestamp, 0, ipv6, isRemembered, geolocationInfo, unknown));
        }
    }

    return entries;
}

auto Steamd::set_steam_guard_new_machine_dialog_response(const bool& isApproved, const bool &isWizardComplete) -> void {
    m_steamClientUser->SetSteamGuardNewMachineDialogResponse(isApproved, isWizardComplete);
}

auto Steamd::get_steam_guard_details() -> void {
    m_steamClientUser->GetSteamGuardDetails();
}

auto Steamd::set_phone_is_verified(const bool &isVerified) -> void {
    m_steamClientUser->SetPhoneIsVerified(isVerified);
}

auto Steamd::is_phone_verified() -> bool {
    return m_steamClientUser->BIsPhoneVerified();
}

auto Steamd::set_phone_is_identifying(const bool &isIdentifying) -> void {
    m_steamClientUser->SetPhoneIsIdentifying(isIdentifying);
}

auto Steamd::is_phone_identifying() -> bool {
    return m_steamClientUser->BIsPhoneIdentifying();
}

auto Steamd::set_is_phone_requiring_verification(const bool &phoneRequiresVerification) -> void {
    m_steamClientUser->SetPhoneIsRequiringVerification(phoneRequiresVerification);
}

auto Steamd::is_phone_requiring_verification() -> bool {
    return m_steamClientUser->BIsPhoneRequiringVerification();
}

auto Steamd::change_two_factor_authentication_options(const int32_t &option) -> void {
    m_steamClientUser->ChangeTwoFactorAuthOptions(option);
}

auto Steamd::set_user_machine_name(const std::string &machineName) -> void {
    m_steamClientUser->SetUserMachineName(machineName.c_str());
}

auto Steamd::get_user_machine_name() -> std::tuple<bool, std::string> {
    std::string machineName(32, '\0');

    bool result = m_steamClientUser->GetUserMachineName(machineName.data(), 32);

    std::size_t index = machineName.find('\0');
    if (index != std::string::npos) {
        machineName.resize(index);
    }

    return std::make_tuple(result, machineName);
}

auto Steamd::get_email_domain_from_logon_failure() -> std::tuple<bool, std::string> {
    // Domains (After the @) can be a maximum of 255
    std::string domain(255, '\0');

    bool result = m_steamClientUser->GetEmailDomainFromLogonFailure(domain.data(), 255);

    std::size_t index = domain.find('\0');
    if (index != std::string::npos) {
        domain.resize(index);
    }

    return std::make_tuple(result, domain);
}

auto Steamd::request_notifications() -> void {
    m_steamClientUser->RequestNotifications();
}

auto Steamd::get_recovery_email() -> std::tuple<bool, std::string> {
    // Emails can have a maximum of 320 characters
    std::string recoveryEmail(320, '\0');

    bool result = m_steamClientUser->BGetRecoveryEmail(recoveryEmail.data(), 320);

    std::size_t index = recoveryEmail.find('\0');
    if (index != std::string::npos) {
        recoveryEmail.resize(index);
    }
    return std::make_tuple(result, recoveryEmail);
}

auto Steamd::can_logon_offline_mode() -> bool {
    return m_steamClientUser->BCanLogonOfflineMode();
}

auto Steamd::log_on_offline_mode() -> int32_t {
    return m_steamClientUser->LogOnOfflineMode();
}

auto Steamd::get_offline_logon_ticket(const std::string &username) -> std::tuple<bool, sdbus::Struct<uint32_t, uint32_t>> {
    // TODO: Use Protobuf for this function

    return std::make_tuple(false, sdbus::Struct<uint32_t, uint32_t>(0, 0));
}

auto Steamd::validate_offline_logon_ticket(const std::string &unknown) -> int32_t {
    return m_steamClientUser->ValidateOfflineLogonTicket(unknown.c_str());
}

auto Steamd::is_other_session_playing() -> std::tuple<bool, uint32_t> {
    uint32_t other{};
    bool result = m_steamClientUser->BIsOtherSessionPlaying(&other);

    return std::make_tuple(result, other);
}

auto Steamd::kick_other_playing_session() -> bool {
    return m_steamClientUser->BKickOtherPlayingSession();
}

auto Steamd::signal_apps_to_shut_down(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> bool {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    return m_steamClientUser->SignalAppsToShutDown(game);
}

auto Steamd::terminate_app_multi_step(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId, const uint32_t& unknown) -> bool {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    return m_steamClientUser->TerminateAppMultiStep(game, unknown);
}

auto Steamd::is_game_running(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> bool {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    return m_steamClientUser->BIsGameRunning(game);
}

auto Steamd::is_any_game_running() -> bool {
    return m_steamClientUser->BIsAnyGameRunning();
}

auto Steamd::get_number_games_running() -> int32_t {
    return m_steamClientUser->NumGamesRunning();
}

auto Steamd::get_running_game_id(const int32_t &game) -> sdbus::Struct<uint32_t, uint32_t, uint32_t> {
    CGameID id = m_steamClientUser->GetRunningGameID(game);

    return sdbus::Struct<uint32_t, uint32_t, uint32_t>(id.AppID(), (id.IsSteamApp() ? 0 : (id.IsMod() ? 1 : (id.IsShortcut() ? 2 : 3))), id.ModID());
}

auto Steamd::get_running_game_pid(const int32_t &game) -> int32_t {
    return m_steamClientUser->GetRunningGamePID(game);
}

auto Steamd::is_game_window_ready(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> bool {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    return m_steamClientUser->BIsGameWindowReady(game);
}

auto Steamd::raise_window_for_game(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> int32_t {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    return m_steamClientUser->RaiseWindowForGame(game);
}

auto Steamd::is_app_overlay_enabled(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> bool {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    return m_steamClientUser->BIsAppOverlayEnabled(game);
}

auto Steamd::overlay_ignore_child_processes(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> bool {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    return m_steamClientUser->BOverlayIgnoreChildProcesses(game);
}

auto Steamd::request_custom_binary(const std::string& absolutePath, const uint32_t& appId, const bool& forceUpdate, const bool& appLaunchRequest) -> bool {
    return m_steamClientUser->RequestCustomBinary(absolutePath.c_str(), appId, forceUpdate, appLaunchRequest);
}

auto Steamd::run_install_script(const uint32_t &appId, const bool &uninstall, const std::string &unknown) -> bool {
    return m_steamClientUser->RunInstallScript(appId, unknown.c_str(), uninstall);
}

auto Steamd::is_install_script_running() -> uint32_t {
    return m_steamClientUser->IsInstallScriptRunning();
}

auto Steamd::get_install_script_state() -> std::tuple<std::string, uint32_t, uint32_t, bool> {
    std::string description(512, '\0');
    uint32_t steps{};
    uint32_t currentStep{};
    
    bool result = m_steamClientUser->GetInstallScriptState(description.data(), 512, &steps, &currentStep);

    std::size_t index = description.find('\0');
    if (index != std::string::npos) {
        description.resize(index);
    }

    return std::make_tuple(description, steps, currentStep, result);
}

auto Steamd::spawn_process(const std::string& applicationName, const std::string& commandLine, const std::string& currentDirectory, const sdbus::Struct<uint32_t, uint32_t, uint32_t>& gameId, const std::string& gameName, const uint32_t& unknown1, const uint32_t& unknown2, const uint32_t& unknown3) -> bool {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    return m_steamClientUser->SpawnProcess(applicationName.c_str(), commandLine.c_str(), currentDirectory.c_str(), game, gameName.c_str(), unknown1, unknown2, unknown3);
}

auto Steamd::get_marketing_messages() -> std::vector<sdbus::Struct<int64_t, std::string, int32_t>> {
    std::vector<sdbus::Struct<int64_t, std::string, int32_t>> marketingMessages{};
    int32_t marketingMessageCount = m_steamClientUser->GetMarketingMessageCount();

    for (std::size_t i = 0; i < marketingMessageCount; i++) {
        // Kind of a long pre-initialized string
        // Have to make sure we are able to fit an
        // entire URL, which can be a max of 2048 characters
        std::string messageUrl(2048, '\0');
        GID_t messageGid{};
        EMarketingMessageFlags messageFlags{};

        bool result = m_steamClientUser->GetMarketingMessage(i, &messageGid, messageUrl.data(), 2048, &messageFlags);

        std::size_t index = messageUrl.find('\0');
        if (index != std::string::npos) {
            messageUrl.resize(index);
        }
        
        if (result) {
            marketingMessages.push_back(sdbus::Struct<int64_t, std::string, int32_t>(messageGid, messageUrl, messageFlags));
        }
    }
    
    return marketingMessages;
}

auto Steamd::get_game_badge_level(const int32_t &series, const bool &foil) -> int32_t {
    return m_steamClientUser->GetGameBadgeLevel(series, foil);
}

auto Steamd::get_app_minutes_played(const uint32_t &appId) -> std::tuple<bool, int32_t, int32_t> {
    int32_t playtimeForever{};
    int32_t playtimeLastTwoWeeks{};

    bool result = m_steamClientUser->BGetAppMinutesPlayed(appId, &playtimeForever, &playtimeLastTwoWeeks);

    return std::make_tuple(result, playtimeForever, playtimeLastTwoWeeks);
}

auto Steamd::get_app_last_played_time(const uint32_t &appId) -> uint32_t {
    return m_steamClientUser->GetAppLastPlayedTime(appId);
}

auto Steamd::get_app_update_disabled_seconds_remaining(const uint32_t &appId) -> uint32_t {
    return m_steamClientUser->GetAppUpdateDisabledSecondsRemaining(appId);
}

auto Steamd::get_guide_url(const uint32_t &appId) -> std::tuple<bool, std::string> {
    // Kind of a long pre-initialized string
    // Have to make sure we are able to fit an
    // entire URL, which can be a max of 2048 characters
    std::string guideUrl(2048, '\0');

    bool result = m_steamClientUser->BGetGuideURL(appId, guideUrl.data(), 2048);

    std::size_t index = guideUrl.find('\0');
    if (index != std::string::npos) {
        guideUrl.resize(index);
    }

    return std::make_tuple(result, guideUrl);
}

auto Steamd::is_subscribed_to_app(const uint32_t &appId) -> bool {
    return m_steamClientUser->BIsSubscribedApp(appId);
}

auto Steamd::get_subscribed_apps() -> std::tuple<uint32_t, std::vector<uint32_t>> {
    // Temporary implementation based on a whim of guesses...
    // Assuming the return of the steam call is how many total are available
    // Assuming the first parameter is a c style array
    // Assuming the second parameter is the size of the first
    // Not sure about the third...

    uint32_t numSubscribedApps = m_steamClientUser->GetSubscribedApps(nullptr, 0, true);
    std::vector<uint32_t> subscribedApps(numSubscribedApps);
    m_steamClientUser->GetSubscribedApps(subscribedApps.data(), numSubscribedApps, true);
    
    return std::make_tuple(numSubscribedApps, subscribedApps);
}

auto Steamd::clear_and_set_app_tags(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId, const std::vector<std::string> &appTags) -> void {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    SteamParamStringArray_t params;
    params.m_nNumStrings = appTags.size();

    // Workaround to include a C++ vector of C++ strings
    // in a Steam Array of C strings (char**)
    // Without copying data
    const char** array = new const char*[appTags.size()];
    for (std::size_t i = 0; i < appTags.size(); i++) {
        array[i] = appTags.at(i).data();
    }

    params.m_ppStrings = array;

    m_steamClientUser->ClearAndSetAppTags(game, &params);

    params.m_nNumStrings = 0;
    params.m_ppStrings = nullptr;

    // Single delete since the underlying pointers are already managed
    delete array; // NOLINT
}

auto Steamd::remove_app_tag(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId, const std::string &appTag) -> void {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }
    
    m_steamClientUser->RemoveAppTag(game, appTag.c_str());
}

auto Steamd::add_app_tag(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId, const std::string &appTag) -> void {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    m_steamClientUser->AddAppTag(game, appTag.c_str());
}

auto Steamd::clear_app_tags(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId) -> void {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    m_steamClientUser->ClearAppTags(game);
}

auto Steamd::set_app_hidden(const sdbus::Struct<uint32_t, uint32_t, uint32_t> &gameId, const bool &hidden) -> void {
    CGameID game = CGameID(gameId.get<0>());

    if(gameId.get<1>() != 0) {
        game = CGameID(gameId.get<0>(), gameId.get<2>());
    }

    m_steamClientUser->SetAppHidden(game, hidden);
}

/* Steam Callbacks */

auto Steamd::RunSteamCallbacks(Steamd *context) -> void {
    while (!context->m_done.load()) {
        Steam_RunCallbacks(context->m_steamPipeHandle, false);
    }
}

auto Steamd::OnSteamServerConnectionFailure(SteamServerConnectFailure_t *cbMsg) -> void {
    std::cout << "Connect fail" << std::endl;
    emitSteam_servers_status_update(cbMsg->k_iCallback);
}

auto Steamd::OnSteamServerConnected(SteamServersConnected_t *cbMsg) -> void {
    std::cout << "Connected" << std::endl;
    emitSteam_servers_status_update(EResult::k_EResultOK);
}

auto Steamd::OnSteamServerDisconnected(SteamServersDisconnected_t *cbMsg) -> void {
    std::cout << "Disconnected" << std::endl;
    emitSteam_servers_status_update(cbMsg->m_eResult);
}
