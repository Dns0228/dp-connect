# DP Connect error help

## Error 101: InternalError

DP Connect could not complete an operation because it received incomplete or unexpected data.

1. Restart DP Connect and repeat the operation.
2. If the error occurred during VPS setup, verify the server address, SSH port, credentials and administrator access.
3. Open the server settings and run the installed services check again.
4. If the problem continues, attach the application logs and the error number to a GitHub issue.

[Create a DP Connect issue](https://github.com/Dns0228/dp-connect/issues/new)

## Error 200: ServerCheckFailed

DP Connect connected to the VPS but could not read a complete server readiness report.

1. Verify that SSH access works with the same host, port and user.
2. Use `root`, or a user that can run `sudo` without an interactive password prompt.
3. Make sure the server has a POSIX shell and the `base64` utility.
4. Run **Check again** in DP Connect.

## Other errors

For other error numbers, retry the operation once and review the related server or connection settings. When reporting a problem, include the error number, operating system, DP Connect version and relevant log lines. Never publish VPS passwords, private keys or exported VPN profiles.
