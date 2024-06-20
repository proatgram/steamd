#include "Steam.hpp"

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

auto Steamd::log_on_with_two_factor(const std::string& username, const std::string& password, const bool &rememberInfo, const std::string& twoFactorCode) -> void {
    m_steamClientUser->SetLoginInformation(username.c_str(), password.c_str(), rememberInfo);
    m_steamClientUser->SetTwoFactorCode(twoFactorCode.c_str());
    m_steamClientUser->LogOn(m_steamClientUser->GetSteamID());
}

auto Steamd::log_on_with_two_factor(const std::string &twoFactorCode) -> void {
    m_steamClientUser->SetTwoFactorCode(twoFactorCode.c_str());
    m_steamClientUser->LogOn(m_steamClientUser->GetSteamID());
}

auto Steamd::log_on_with_steam_guard_code(const std::string& username, const std::string& password, const bool &rememberInfo, const bool &rememberComputer, const std::string& steamGuardCode) -> void {
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
    std::string email = std::string(320, ' ');
    bool validated{};
    bool result = m_steamClientUser->GetEmail(email.data(), 320, &validated);
    std::size_t index = email.find_first_of(' ');
    if (index != std::string::npos) {
        email.resize(index - 1);
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

    std::string path = std::string(PATH_MAX, ' ');

    bool result = m_steamClientUser->GetUserDataFolder(game, path.data(), PATH_MAX);
    std::size_t index = path.find_first_of(' ');
    if (index != std::string::npos) {
        path.resize(index - 1);
    }

    return std::make_tuple(result, path);
}

/* Steam Callbacks */

auto Steamd::RunSteamCallbacks(Steamd *context) -> void {
    while (!context->m_done.load()) {
        Steam_RunCallbacks(context->m_steamPipeHandle, false);
    }
}

auto Steamd::OnSteamServerConnectionFailure(SteamServerConnectFailure_t *cbMsg) -> void {
    emitSteam_servers_status_update(cbMsg->k_iCallback);
}

auto Steamd::OnSteamServerConnected(SteamServersConnected_t *cbMsg) -> void {
    emitSteam_servers_status_update(EResult::k_EResultOK);
}

auto Steamd::OnSteamServerDisconnected(SteamServersDisconnected_t *cbMsg) -> void {
    emitSteam_servers_status_update(cbMsg->m_eResult);
}
