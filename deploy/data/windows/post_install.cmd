sc stop AmneziaWGTunnel$DPConnect
sc delete AmneziaWGTunnel$DPConnect
taskkill /IM "DPConnect-service.exe" /F
taskkill /IM "DPConnect.exe" /F
exit /b 0
