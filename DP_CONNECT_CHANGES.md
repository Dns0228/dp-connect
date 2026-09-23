# DP Connect branding changes

## Product identity

- Application: **DP Connect**
- Brand line: **by DP project**
- Main protocol label: **DP WG**
- Palette: `#10171C`, `#19242B`, `#77E0C2`, `#F3F6F5`
- New application mark and Windows icon are included in `client/images`.

## Self-host

- The first-run wizard prioritizes self-hosted VPN setup.
- Store, hosting, upstream support and news entry points are hidden or removed from the main flow.
- The DP WG server wrapper uses the public DP project image from GHCR.
- The image is pinned to immutable multi-architecture digest `sha256:727a5a9b8cd0698865d2d00b1e92d93c6c48c3c81d52a5128e24fb4ef3d2cf27`.
- Client updates remain disabled until DP Connect has its own signed release channel.
- New DP WG profiles receive an independent X25519 key pair and a 256-bit preshared key generated with OpenSSL's private random generator.
- The setup wizard now checks VPS compatibility before installation without changing server state.
- Installation runs in a background task and reports real server-side stages to the progress screen.
- A successful setup now requires Docker to confirm that the deployed container is running. DP WG and WireGuard setups also verify their tunnel interface and public key.
- Device management now uses the locale-independent `show all dump` format for handshake and traffic statistics.
- Device cards show active/offline state and provide public-key copy and access revocation actions.
- Server management now includes an asynchronous overview with container state, uptime, protocol version, peer count, image and last-check time.
- Backups are protected with authenticated AES-256-GCM encryption and a password-derived key using PBKDF2-HMAC-SHA256 with 600,000 iterations.
- Restore verifies the authentication tag before importing any settings. Legacy plaintext backups remain readable for migration.
- Required TCP and UDP ports are checked for local conflicts before Docker installation or any other server mutation. The check reads the Linux socket tables directly and works on minimal VPS images.

## DP WG cryptography

- DP WG keeps the audited WireGuard Noise_IKpsk2 construction and its X25519, ChaCha20-Poly1305, BLAKE2s and HKDF primitives.
- Long-term X25519 private keys and per-device preshared keys are generated independently.
- Temporary key buffers are cleansed after use and all OpenSSL return values are checked.
- Existing profiles keep their previous PSK until they are reissued; automatic profile migration is a follow-up item.

## Compatibility identifiers

Some internal names remain unchanged because they are part of configuration formats, migration paths, protocol executables, container paths or third-party copyright notices. Changing these identifiers would break imports, existing servers or license attribution. They are not shown as product branding in the main user interface.

## Verification performed

- GitHub Actions built and smoke-tested the DP WG image for `linux/amd64` and `linux/arm64`.
- Anonymous GHCR manifest access was verified for the exact digest used by the client.
- All Qt resource manifests and 11 translation XML files parse successfully.
- Every resource referenced by a Qt resource manifest exists locally.
- No active QML product text or external link contains the old brand name.

The full desktop client was not compiled in this workspace because the extracted source archive does not include its Git submodule contents and the required Qt/CMake toolchain is not installed.
