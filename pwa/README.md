# DP Connect PWA

Installable offline companion for DP WG profiles.

## Features

- imports standard DP WG/WireGuard `.conf` profiles;
- encrypts every profile locally with AES-256-GCM;
- derives the encryption key using PBKDF2-HMAC-SHA256 with 600,000 iterations;
- opens a QR code, downloads a `.conf` file, or shares it with a native VPN client;
- automatically locks an opened profile after five minutes;
- works offline after the first successful load.

A web browser cannot create an operating-system VPN tunnel. The PWA securely stores and transfers a profile; DP Connect or another compatible native WireGuard client establishes the tunnel.

## Development

```sh
npm install
npm run dev
```

Production build:

```sh
npm run build
```
