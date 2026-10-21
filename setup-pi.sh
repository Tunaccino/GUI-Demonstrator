#!/bin/bash
#
# Richtet einen frischen Raspberry Pi fuer den GUI-Demonstrator mit vier
# KY-040-Drehgebern ein:
#   1. Pakete installieren (git, Compiler, CMake, Qt6)
#   2. Code von GitHub holen
#   3. Drehgeber in /boot/firmware/config.txt eintragen
#   4. Programm bauen
#
# Aufruf:
#   curl -fsSL https://raw.githubusercontent.com/Tunaccino/GUI-Demonstrator/vier-drehgeber/setup-pi.sh -o setup-pi.sh
#   bash setup-pi.sh
#
# Das Skript kann gefahrlos mehrmals laufen, die config.txt wird nur einmal
# ergaenzt. Danach einmal neu starten (sudo reboot).

set -euo pipefail

BRANCH="vier-drehgeber"
REPO="https://github.com/Tunaccino/GUI-Demonstrator.git"
DIR="${DIR:-$HOME/GUI-Demonstrator}"

if [ -z "${CONFIG:-}" ]; then
    CONFIG=/boot/firmware/config.txt
    [ -f "$CONFIG" ] || CONFIG=/boot/config.txt   # aeltere Raspberry Pi OS
fi

MARKER="# KY-040 Drehgeber (GUI-Demonstrator)"

step() { printf '\n==> %s\n' "$*"; }

# ---------------------------------------------------------------------------
step "Update-Dienst anhalten (blockiert sonst die Paketverwaltung)"
sudo systemctl stop packagekit 2>/dev/null || true

# apt wartet bis zu 10 Minuten, falls die Paketverwaltung doch noch belegt ist.
APT_WAIT=(-o DPkg::Lock::Timeout=600)

step "Pakete installieren"
sudo apt-get "${APT_WAIT[@]}" update
sudo apt-get "${APT_WAIT[@]}" install -y git cmake build-essential qt6-base-dev

# ---------------------------------------------------------------------------
step "Code holen ($BRANCH)"
if [ -d "$DIR/.git" ]; then
    git -C "$DIR" fetch origin
    git -C "$DIR" checkout "$BRANCH"
    git -C "$DIR" pull --ff-only
else
    git clone -b "$BRANCH" "$REPO" "$DIR"
fi

# ---------------------------------------------------------------------------
step "Drehgeber in $CONFIG eintragen"
if grep -qF "$MARKER" "$CONFIG" || grep -qF "rotary-encoder,pin_a=21,pin_b=20" "$CONFIG"; then
    echo "Schon eingetragen, nichts zu tun."
else
    # Sicherungskopie, falls etwas schiefgeht.
    sudo cp -n "$CONFIG" "$CONFIG.vor-drehgeber"

    # Belegung je Knopf (CLK / DT / SW):
    #   1 = P:    21 / 20 / 16
    #   2 = I:    26 / 19 / 13
    #   3 = D:     6 /  5 /  0
    #   4 = Goal: 11 /  9 / 10
    # Aenderungen hier muessen zu setupKnobs() in controller.cpp passen.
    sudo tee -a "$CONFIG" >/dev/null <<EOF

[all]
$MARKER
dtoverlay=rotary-encoder,pin_a=21,pin_b=20,relative_axis=1,linux_axis=9
dtoverlay=rotary-encoder,pin_a=26,pin_b=19,relative_axis=1,linux_axis=9
dtoverlay=rotary-encoder,pin_a=6,pin_b=5,relative_axis=1,linux_axis=9
dtoverlay=rotary-encoder,pin_a=11,pin_b=9,relative_axis=1,linux_axis=9
dtoverlay=gpio-key,gpio=16,keycode=148,label="KNOB1",gpio_pull=2
dtoverlay=gpio-key,gpio=13,keycode=149,label="KNOB2",gpio_pull=2
dtoverlay=gpio-key,gpio=0,keycode=202,label="KNOB3",gpio_pull=2
dtoverlay=gpio-key,gpio=10,keycode=203,label="KNOB4",gpio_pull=2
EOF
    echo "Eingetragen. Sicherung liegt unter $CONFIG.vor-drehgeber"
fi

# GPIO 9, 10 und 11 sind die SPI-Pins. Ist SPI an, belegt es die Pins vor uns.
if grep -qE '^[[:space:]]*dtparam=spi=on' "$CONFIG"; then
    sudo sed -i -E 's/^([[:space:]]*dtparam=spi=on)/#\1/' "$CONFIG"
    echo "SPI ausgeschaltet (wird fuer Knopf 4 gebraucht)."
fi

# ---------------------------------------------------------------------------
step "Programm bauen"
cd "$DIR/GUI-Demonstrator"
rm -rf build
cmake -S . -B build
cmake --build build -j"$(nproc)"

# ---------------------------------------------------------------------------
cat <<EOF

Fertig!

Jetzt einmal neu starten:
    sudo reboot

Danach die GUI starten mit:
    $DIR/GUI-Demonstrator/build/Controller
EOF
