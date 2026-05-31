#!/usr/bin/env bash
set -euo pipefail

BROKER_IP="${1:-192.168.1.10}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../.." && pwd)"
CERT_DIR="$SCRIPT_DIR/certs"
APP_CA="$PROJECT_DIR/components/app_net/certs/ca.crt"
NOT_BEFORE="$(date -u -d '1 day ago' +%Y%m%d%H%M%SZ)"
NOT_AFTER="$(date -u -d '10 years' +%Y%m%d%H%M%SZ)"

mkdir -p "$CERT_DIR" "$(dirname "$APP_CA")"

openssl genrsa -out "$CERT_DIR/demo-ca.key" 2048
openssl req -x509 -new -nodes \
    -key "$CERT_DIR/demo-ca.key" \
    -sha256 \
    -not_before "$NOT_BEFORE" \
    -not_after "$NOT_AFTER" \
    -out "$CERT_DIR/ca.crt" \
    -subj "/CN=IoT25-Demo-CA"

openssl genrsa -out "$CERT_DIR/server.key" 2048
openssl req -new \
    -key "$CERT_DIR/server.key" \
    -out "$CERT_DIR/server.csr" \
    -subj "/CN=mosquitto" \
    -addext "subjectAltName=DNS:mosquitto,DNS:localhost,IP:127.0.0.1,IP:${BROKER_IP}"

openssl x509 -req \
    -in "$CERT_DIR/server.csr" \
    -CA "$CERT_DIR/ca.crt" \
    -CAkey "$CERT_DIR/demo-ca.key" \
    -CAcreateserial \
    -out "$CERT_DIR/server.crt" \
    -not_before "$NOT_BEFORE" \
    -not_after "$NOT_AFTER" \
    -sha256 \
    -copy_extensions copy

cp "$CERT_DIR/ca.crt" "$APP_CA"
chmod 644 "$CERT_DIR"/*.crt "$CERT_DIR/server.key" "$APP_CA"
chmod 600 "$CERT_DIR/demo-ca.key"

echo "Generated demo MQTTS certificates for broker IP ${BROKER_IP}."
echo "Rebuild the firmware after regenerating certificates."
