#!/bin/sh
# OpenKey Linux Post-Installation Script
# Designed for Debian and RPM packages

echo "OpenKey Linux installed successfully."

# Function to execute a command as the active user on the primary display
execute_as_user() {
    CMD=$1
    # Find the user running the current graphical session
    ACTIVE_USER=$(who | grep -E '\(:[0-9]+(\.[0-9]+)?\)' | awk '{print $1}' | head -n 1)
    
    if [ -n "$ACTIVE_USER" ]; then
        echo "Executing '$CMD' as active user: $ACTIVE_USER"
        # Export necessary display variables and run the command
        su - "$ACTIVE_USER" -c "export DISPLAY=:0; export WAYLAND_DISPLAY=wayland-0; $CMD"
    else
        echo "No active graphical user found. Skipping '$CMD'."
    fi
}

# The true Zero-Configuration mandate: Automatically reload Fcitx5 so OpenKey appears instantly.
execute_as_user "fcitx5 -r -d" || true

exit 0
