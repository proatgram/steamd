#include "Steam.hpp"

using namespace steamd;

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

auto Steamd::log_on_with_steamid(const uint64_t &steam_id) -> void {
    m_steamClientUser->LogOn(steam_id);
}

auto Steamd::log_on_with_credentials(const std::string &username, const std::string &password, const bool &rememberInfo) -> void {
    m_steamClientUser->SetLoginInformation(username.c_str(), password.c_str(), rememberInfo);
    m_steamClientUser->LogOn(m_steamClientUser->GetSteamID());
}

auto Steamd::try_cache_log_on(const std::string &username) -> void {
    m_steamClientUser->SetAccountNameForCachedCredentialLogin(username.c_str(), false);
    m_steamClientUser->LogOn(m_steamClientUser->GetSteamID());
}

auto Steamd::get_log_on_state() -> int32_t {
    return static_cast<int32_t>(m_steamClientUser->GetLogonState());
}

auto Steamd::log_off() -> void {
    m_steamClientUser->LogOff();
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
