# DP Connect

DP Connect — клиент для собственного VPN от **DP project**. Приложение разворачивает и обслуживает VPN на вашем VPS; рекомендуемый протокол — **DP WG**.

[English](README.md)

## Что улучшено

- фирменный стиль DP Connect и палитра Graphite Mint;
- публичный образ DP WG для amd64 и arm64: `ghcr.io/dns0228/dp-wg-go`;
- отдельная пара X25519 и независимый 256-битный preshared key для каждого устройства;
- проверка готовности VPS и фоновая установка с реальными этапами;
- проверка контейнера и туннеля после установки;
- список устройств с активностью, трафиком, переименованием и отзывом доступа;
- экран состояния сервера;
- резервные копии с паролем, AES-256-GCM и PBKDF2-HMAC-SHA256.
- устанавливаемый PWA-клиент с зашифрованным хранилищем профилей, QR-кодом и офлайн-режимом.
- DP Stealth (бета): приоритет DP WG, проверка реального HTTPS-трафика и автоматический переход на следующий транспорт, если туннель заблокирован или не работает.

Подробности находятся в [DP_CONNECT_CHANGES.md](DP_CONNECT_CHANGES.md), [DP_CONNECT_CRYPTO.md](DP_CONNECT_CRYPTO.md), [DP_CONNECT_IMPROVEMENTS.md](DP_CONNECT_IMPROVEMENTS.md) и [docs/DP_STEALTH.md](docs/DP_STEALTH.md).

## Получение исходного кода

```bash
git clone --recurse-submodules https://github.com/Dns0228/dp-connect.git
cd dp-connect
```

Если репозиторий уже скачан без подмодулей:

```bash
git submodule update --init --recursive
```

Для сборки используются CMake, Qt 6.10, Conan 2, OpenSSL и инструменты выбранной платформы. Ручные сборки GitHub Actions описаны в `.github/workflows/dp-connect-build.yml`.

## PWA-клиент

[Открыть DP Connect PWA](https://dns0228.github.io/dp-connect/)

Исходники веб-клиента находятся в каталоге [`pwa`](pwa). PWA импортирует профиль DP WG, шифрует его в браузере с помощью AES-256-GCM и позволяет открыть QR-код, скачать или передать `.conf` в системный VPN-клиент. После первого открытия основные функции доступны офлайн.

Браузер не имеет доступа к системному VPN-интерфейсу, поэтому непосредственно туннель запускает нативный DP Connect либо совместимый WireGuard-клиент.

## Происхождение и лицензия

DP Connect основан на открытом клиенте [AmneziaVPN](https://github.com/amnezia-vpn/amnezia-client), версия `5.0.3.0`, и распространяется по GNU GPL v3. Авторские уведомления исходного проекта и сторонних компонентов сохранены в коде и в [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).

DP Connect является самостоятельной сборкой DP project и не относится к официальным релизам исходного проекта.
