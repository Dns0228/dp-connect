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

Подробности находятся в [DP_CONNECT_CHANGES.md](DP_CONNECT_CHANGES.md), [DP_CONNECT_CRYPTO.md](DP_CONNECT_CRYPTO.md) и [DP_CONNECT_IMPROVEMENTS.md](DP_CONNECT_IMPROVEMENTS.md).

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

## Происхождение и лицензия

DP Connect основан на открытом клиенте [AmneziaVPN](https://github.com/amnezia-vpn/amnezia-client), версия `5.0.3.0`, и распространяется по GNU GPL v3. Авторские уведомления исходного проекта и сторонних компонентов сохранены в коде и в [THIRD_PARTY_LICENSES.md](THIRD_PARTY_LICENSES.md).

DP Connect является самостоятельной сборкой DP project и не относится к официальным релизам исходного проекта.
