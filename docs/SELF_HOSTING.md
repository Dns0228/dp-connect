# DP Connect self-hosting

DP Connect can install and manage DP WG on a Linux VPS over SSH.

## Requirements

- A supported Linux VPS with a public IP address
- Root access, or a user that can run `sudo` without an interactive password
- SSH access with a password or supported private key
- Enough free memory and disk space for Docker and DP Connect services

## Installation

1. In DP Connect, choose **Self-hosted VPN**.
2. Enter the VPS address, SSH port, user and authentication details.
3. Review the readiness check. It only reads server information and does not change server settings.
4. Continue with installation and keep DP WG selected as the primary protocol.

DP Connect connects over SSH, installs the required container runtime when needed, downloads the DP Connect server images and creates the VPN configuration. Passwords and private keys are not published to GitHub.

If the readiness check fails, see [DP Connect error help](ERROR_CODES.md).
