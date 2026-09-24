# DP Connect

DP Connect is a self-hosted VPN client by **DP project**. It deploys and manages a VPN on your own VPS, with **DP WG** as the recommended protocol.

[Русская версия](README_RU.md)

## Current improvements

- DP Connect identity and Graphite Mint interface.
- Public multi-architecture DP WG server image: `ghcr.io/dns0228/dp-wg-go`.
- Per-device X25519 key pair and independent 256-bit preshared key.
- VPS readiness check and staged background installation.
- Post-install container and tunnel verification.
- Device activity, traffic, rename and access revocation.
- Server status screen.
- Password-protected backups using AES-256-GCM and PBKDF2-HMAC-SHA256.
- Installable PWA with encrypted profile storage, QR codes, and offline support.
- DP Stealth beta mode prefers DP WG, verifies real HTTPS traffic, and automatically tries the next installed transport when the tunnel is blocked or fails.

The implementation notes are in [DP_CONNECT_CHANGES.md](DP_CONNECT_CHANGES.md), [DP_CONNECT_CRYPTO.md](DP_CONNECT_CRYPTO.md), [DP_CONNECT_IMPROVEMENTS.md](DP_CONNECT_IMPROVEMENTS.md), and [docs/DP_STEALTH.md](docs/DP_STEALTH.md).

## Get the source

```bash
git clone --recurse-submodules https://github.com/Dns0228/dp-connect.git
cd dp-connect
```

If the repository was cloned without submodules:

```bash
git submodule update --init --recursive
```

The project uses CMake, Qt 6.10, Conan 2, OpenSSL, and the platform toolchain. Manual GitHub Actions builds are defined in `.github/workflows/dp-connect-build.yml`.

## PWA client

[Open DP Connect PWA](https://dns0228.github.io/dp-connect/)

The web client is in [`pwa`](pwa). It imports a DP WG profile, encrypts it in the browser with AES-256-GCM, and can display a QR code or download/share the `.conf` file with a native VPN client. Core functions work offline after the first load.

Web browsers cannot create an operating-system VPN interface, so the tunnel itself is established by native DP Connect or another compatible WireGuard client.

## Origin and license

DP Connect is a modified distribution based on the open-source [AmneziaVPN client](https://github.com/amnezia-vpn/amnezia-client), version `5.0.3.0`. The project remains licensed under the GNU General Public License v3. Original copyright and third-party notices are preserved in the source and in [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).

DP Connect is an independent DP project distribution and is not an official release of the upstream project.
