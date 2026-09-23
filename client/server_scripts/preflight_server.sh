#!/bin/sh

# Stable, machine-readable VPS capability report. Values never contain newlines.
emit() {
    printf '%s=%s\n' "$1" "$2"
}

OS_ID=unknown
OS_VERSION=unknown
if [ -r /etc/os-release ]; then
    . /etc/os-release
    OS_ID=${ID:-unknown}
    OS_VERSION=${VERSION_ID:-unknown}
fi

ARCH=$(uname -m 2>/dev/null || printf unknown)
KERNEL=$(uname -r 2>/dev/null || printf unknown)
MEMORY_MB=$(awk '/^MemTotal:/ { printf "%d", $2 / 1024 }' /proc/meminfo 2>/dev/null)
DISK_FREE_MB=$(df -Pk / 2>/dev/null | awk 'NR == 2 { printf "%d", $4 / 1024 }')

PACKAGE_MANAGER=unknown
for candidate in apt-get dnf yum zypper pacman; do
    if command -v "$candidate" >/dev/null 2>&1; then
        PACKAGE_MANAGER=$candidate
        break
    fi
done

SUDO_READY=false
if [ "$(id -u 2>/dev/null)" = "0" ]; then
    SUDO_READY=true
elif command -v sudo >/dev/null 2>&1 && sudo -n true >/dev/null 2>&1; then
    SUDO_READY=true
fi

DOCKER_INSTALLED=false
DOCKER_ACTIVE=false
DOCKER_VERSION=none
if command -v docker >/dev/null 2>&1; then
    DOCKER_INSTALLED=true
    DOCKER_VERSION=$(docker --version 2>/dev/null | tr '\n=' '  ')
    if docker info >/dev/null 2>&1 || sudo -n docker info >/dev/null 2>&1; then
        DOCKER_ACTIVE=true
    fi
fi

emit os_id "$OS_ID"
emit os_version "$OS_VERSION"
emit architecture "$ARCH"
emit kernel "$KERNEL"
emit memory_mb "${MEMORY_MB:-0}"
emit disk_free_mb "${DISK_FREE_MB:-0}"
emit package_manager "$PACKAGE_MANAGER"
emit sudo_ready "$SUDO_READY"
emit docker_installed "$DOCKER_INSTALLED"
emit docker_active "$DOCKER_ACTIVE"
emit docker_version "$DOCKER_VERSION"
