#!/bin/bash

# Vérification du paramètre mot de passe
if [ $# -ne 2 ]; then
  echo "Usage: $0 <nom_hotspot> <mot_de_passe_hotspot>"
  exit 1
fi

PASSWORD="$2"

# Nom de la connexion hotspot
CONN_NAME="$1"

# SSID du hotspot
SSID="AndroidCliauto"

# Interface Wi-Fi (à adapter selon ton système)
WIFI_IFACE="wlan0"

# Canal 5 GHz
CHANNEL=36

# Vérifie si la connexion existe
if nmcli connection show "$CONN_NAME" &> /dev/null; then
    echo "Connexion '$CONN_NAME' existe, suppression..."
    nmcli connection delete "$CONN_NAME"
fi

echo "Création de la connexion hotspot 5GHz '$CONN_NAME'..."

nmcli connection add type wifi ifname "$WIFI_IFACE" con-name "$CONN_NAME" autoconnect no ssid "$SSID"
nmcli connection modify "$CONN_NAME" 802-11-wireless.mode ap 802-11-wireless.band a 802-11-wireless.channel $CHANNEL
nmcli connection modify "$CONN_NAME" wifi-sec.key-mgmt wpa-psk
nmcli connection modify "$CONN_NAME" wifi-sec.psk "$PASSWORD"
nmcli connection modify "$CONN_NAME" 802-11-wireless-security.pmf disable
nmcli connection modify "$CONN_NAME" ipv4.method shared ipv6.method ignore
nmcli connection modify "$CONN_NAME" connection.autoconnect yes
nmcli connection up "$CONN_NAME"
echo "Connexion hotspot '$CONN_NAME' créée avec succès."
