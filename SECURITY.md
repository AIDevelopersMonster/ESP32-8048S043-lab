# Security

## Supported baseline

Security guidance in this repository applies to the current KONTAKTS Platform 0.4.0 line unless a newer release explicitly replaces it.

## Current boundary

Platform 0.4.0 is a laboratory/local-network product baseline.

Current local control paths include:

- HTTP Web UI;
- BLE command transport;
- UART0/P1 service interface;
- UART1/RS485 field bus.

The current HTTP and BLE paths are not claimed to provide production-grade authentication, authorization or public-Internet exposure safety.

## Deployment guidance

- Keep the board on a trusted LAN.
- Do not expose the HTTP service directly to the public Internet.
- Do not treat BLE proximity alone as strong authentication.
- Do not connect untrusted masters to the field RS485 bus.
- Use independent hardware interlocks/watchdogs for safety-critical actuation.
- A failed RS485 link can prevent software from confirming or forcing a physical relay OFF state.

## Secrets

Do not commit:

- Wi-Fi credentials;
- private keys;
- API tokens;
- local secrets;
- private certificates;
- device credentials intended to remain confidential.

Use local configuration/NVS and repository secrets as appropriate.

## Reporting

For a security-sensitive issue, avoid publishing credentials, exploit-ready secrets or private device data in a public issue. Contact the repository owner privately through the GitHub account associated with this project and provide the minimum reproducible technical detail needed to investigate.

## Non-claims

The project is not certified for industrial functional safety, medical, automotive, aviation or other regulated safety-critical use.
