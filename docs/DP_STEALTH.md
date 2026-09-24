# DP Stealth (beta)

DP Stealth is an adaptive connection mode for DP Connect. It uses the VPN transports that are already installed on a server and keeps one Connect button for the user.

## Current behavior

When the mode is enabled, DP Connect tries configured transports in this order:

1. DP WG 3.1;
2. DP WG Legacy;
3. XRay/REALITY;
4. OpenVPN;
5. WireGuard;
6. IKEv2.

Only transports supported by the current platform and containing a client configuration are considered. After a local tunnel reports that it is connected, DP Connect verifies that HTTPS traffic can pass through it using two independent connectivity endpoints. If both checks fail, or if the transport reports an error, the client suppresses the intermediate failure and tries the next candidate. The active candidate is displayed under the Connect button.

For useful fallback behavior, install both XRay/REALITY and DP WG on the same self-hosted server. With only one installed VPN transport, DP Stealth uses that transport without changing its configuration.

## Scope of the beta

New DP WG 3.1 installations receive diversified junk counts, packet padding, message headers, content padding and pre-handshake random packets. The WireGuard cryptographic core remains unchanged. Existing servers keep their current synchronized parameters until they are reinstalled or explicitly updated.

This version does not yet encapsulate DP WG in MASQUE/HTTP3 and does not claim permanent resistance to traffic analysis. A later transport layer can be added without changing the profile and fallback model introduced here.

The setting is available under **Settings → Connection → DP Stealth (beta)** and is disabled by default.
