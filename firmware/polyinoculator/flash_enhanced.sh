#!/bin/bash
# Enhanced Polyinoculator Flash Script - Linux/macOS
# Flashes the enhanced firmware with persistent configuration support

clear
echo "==============================================="
echo "    Enhanced Polyinoculator Flash Utility"
echo "==============================================="
echo
echo "Features in this firmware:"
echo "- Persistent configuration storage (NVRAM)"
echo "- Web-based configuration interface"
echo "- Individual prop addressing"
echo "- Device label customization"
echo "- SACN universe/address management"
echo
echo "Hardware Configuration:"
echo "- LED Strip 1: D5 (GPIO5) - 7 LEDs"
echo "- LED Strip 2: D6 (GPIO16) - 4 LEDs"
echo "- LED Strip 3: D8 (GPIO20) - 4 LEDs"
echo "- Total: 15 LEDs (45 DMX channels)"
echo

# Check if device is connected
echo "Checking for connected ESP32-C3 device..."
if ! pio device list | grep -qi "tty\|usb"; then
    echo
    echo "ERROR: No ESP32-C3 device detected!"
    echo
    echo "Please check:"
    echo "1. Device is connected via USB-C cable"
    echo "2. Drivers are installed"
    echo "3. Device permissions (may need sudo)"
    echo "4. Device is in bootloader mode if needed"
    echo
    read -p "Press Enter to continue..."
    exit 1
fi

echo "Device detected! Proceeding with flash..."
echo

# Change to script directory
cd "$(dirname "$0")"

# Clean build to ensure fresh compilation
echo "Cleaning previous build..."
if ! pio run --target clean; then
    echo
    echo "ERROR: Failed to clean build directory!"
    read -p "Press Enter to continue..."
    exit 1
fi

echo
echo "Building enhanced firmware..."
if ! pio run; then
    echo
    echo "ERROR: Firmware compilation failed!"
    echo "Check the code for syntax errors."
    read -p "Press Enter to continue..."
    exit 1
fi

echo
echo "Flashing firmware to device..."
if ! pio run --target upload; then
    echo
    echo "ERROR: Firmware upload failed!"
    echo
    echo "Troubleshooting:"
    echo "1. Try holding BOOT button while connecting USB"
    echo "2. Check device permissions (ls -l /dev/tty*)"
    echo "3. Try different USB cable/port"
    echo "4. Run with sudo if needed"
    echo
    read -p "Press Enter to continue..."
    exit 1
fi

echo
echo "==============================================="
echo "           FLASH SUCCESSFUL!"
echo "==============================================="
echo
echo "The enhanced polyinoculator firmware has been flashed."
echo
echo "Next Steps:"
echo "1. Device will restart automatically"
echo "2. Connect to serial monitor to see boot messages"
echo "3. Device will connect to WiFi: \"Rigging Electric\""
echo "4. Access web interface at device IP address"
echo "5. Configure device label and SACN addressing"
echo
echo "Default Configuration:"
echo "- Device Label: Polyinoculator_XXXX (random)"
echo "- SACN Universe: 1"
echo "- DMX Start Address: 1"
echo "- Brightness: 128/255"
echo

# Start serial monitor automatically
echo -n "Start serial monitor to see device boot? (y/n): "
read choice
if [[ "$choice" =~ ^[Yy]$ ]]; then
    echo
    echo "Starting serial monitor... (Press Ctrl+C to exit)"
    sleep 2
    pio device monitor
fi

echo
echo "Flash process complete!"
read -p "Press Enter to continue..."
