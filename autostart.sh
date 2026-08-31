#!/bin/bash

###
# Helper scripts for setting autostart methods for Dash application 
###

display_help() {
    echo "Autostart install helpers Version 0.3"
    echo "Usage: $0 [option...]" >&2
    echo
    echo "   -adi, --adddesktopicon           Add desktop icon"
    echo "   -asd, --autostartdaemon          Add autostart daemon"
    echo "   -axi, --addxinit                 Add xinit autostart"
    echo "   -awi, --addwaylandinit           Add wayland (weston) autostart"
    echo "   -h, --help                       Show help of script"
    echo
    echo "Example: Add an desktop icon"
    echo "   helpers.sh -adi"
    echo
    echo "Example: Add autostart Systemd daemon"
    echo "   helpers.sh -asd"
    echo
    echo "Example: Add autostart xinit script"
    echo "   helpers.sh -axi"
    echo
    echo "Example: Add autostart wayland (weston) script"
    echo "   helpers.sh -awi"
    echo
    exit 1
}


add_desktop_icon () {
  # Remove existing opendash desktop
  if [ -f $HOME/Desktop/dash.desktop ]; then
    echo "Removing existing shortcut"
    rm $HOME/Desktop/dash.desktop
  fi

  # Copy icon to pixmaps folder
  echo "Copying icon to system directory (requires sudo)"
  sudo cp -v assets/icons/opendash.xpm /usr/share/pixmaps/opendash.xpm

  # Create shortcut on dashboard
  echo "Creating desktop shortcut at ~/Desktop/dash.desktop"
  bash -c "echo '[Desktop Entry]
Name=Dash
Comment=Open Dash
Icon=/usr/share/pixmaps/opendash.xpm
Exec=$HOME/dash/bin/dash
Type=Application
Encoding=UTF-8
Terminal=true
Categories=None;
  ' > $HOME/Desktop/dash.desktop"
  chmod +x $HOME/Desktop/dash.desktop
}

create_autostart_daemon() {
  WorkingDirectory="$HOME/dash"
  if [[ $2 != "" ]]
  then
     WorkingDirectory="$HOME/$2/dash"
  fi
  echo ${WorkingDirectory}

  if [ -f "/etc/systemd/system/dash.service" ]; then
    # Stop and disable dash service
    echo "Stopping and removing previous service"
    sudo systemctl stop dash.service || true
    sudo systemctl disable dash.service || true
  
    # Remove existing dash service
    sudo systemctl unmask dash.service || true
  fi
  # Write dash service unit
  echo "Creating Dash service unit" 
  sudo bash -c "echo '[Unit]
Description=Dash
After=graphical.target

[Service]
Type=idle
User=$USER
StandardOutput=inherit
StandardError=inherit
Environment=DISPLAY=:0
Environment=XAUTHORITY=${HOME}/.Xauthority
WorkingDirectory=${WorkingDirectory}
ExecStart=${WorkingDirectory}/bin/dash
Restart=on-failure
RestartSec=5s
KillMode=process
TimeoutSec=infinity

[Install]
WantedBy=graphical.target
  ' > /etc/systemd/system/dash.service"

  # Activate and start dash service
  echo "Enabling and starting Dash service"
  sudo systemctl daemon-reload
  sudo systemctl enable dash.service
  sudo systemctl start dash.service
  sudo systemctl status dash.service
}

add_xinit_autostart () {
  # Install dependencies
  echo "Installing xinit and Xorg dependencies"
  sudo apt install -y xserver-xorg xinit x11-xserver-utils hsetroot picom

  # Create picom config
  echo "Creating ~/.config/picom.conf"
  mkdir -p $HOME/.config
  cat <<EOT > $HOME/.config/picom.conf
backend = "xrender";
vsync = true;
shadow = false;
fading = false;
blur-background = false;
EOT

  # Create .xinitrc
  echo "Creating ~/.xinitrc"
  cat <<EOT > $HOME/.xinitrc
#!/usr/bin/env sh
xset -dpms
xset s off
xset s noblank

hsetroot -solid "#000000"

picom --config $HOME/.config/picom.conf -b

while [ true ]; do
  sh $HOME/run_dash.sh
done
EOT

  # Create runner
  echo "Creating ~/run_dash.sh and linking to ~/dash/bin/dash"
  cat <<EOT > $HOME/run_dash.sh
#!/usr/bin/env sh
export QSG_RHI_BACKEND=opengl
export GST_GL_PLATFORM=glx
export GST_GL_WINDOW=x11
export GST_GL_API=opengl
export QT_XCB_GL_INTEGRATION=xcb_glx
export QSG_RENDER_LOOP=basic
export GST_DEBUG=qmlglsink:5,glimagesink:5,xvimagesink:5,v4l2:3,glcontext:5,glwindow:5,qml6gl:5,gldisplay:5
$HOME/dash/bin/dash >> $HOME/dash/bin/dash.log 2>&1
sleep 1
EOT

  # Append to .bashrc
  echo "Appending startx to ~/.bashrc"
  cat <<EOT >> $HOME/.bashrc

### xinit
if [ "\$(tty)" = "/dev/tty1" ]; then
  startx
fi
EOT

}

add_wayland_autostart () {
  # Install dependencies
  echo "Installing weston and Wayland/Qt6 dependencies"
  sudo apt install -y weston qt6-wayland libgl1-mesa-dri mesa-utils

  # Create weston config
  echo "Creating ~/.config/weston.ini"
  mkdir -p $HOME/.config
  cat <<EOT > $HOME/.config/weston.ini
[core]
xwayland=false

[shell]
locking=false
panel-position=none
# background-color is intentionally left unset: weston's DRM backend can
# crash (assertion \`fb' failed in drm_output_find_plane_for_view) when it
# tries to promote a solid-color background surface to a hardware overlay
# plane without a real framebuffer. Use background-image instead if you
# need a non-default background, e.g.:
#background-image=$HOME/.config/wallpaper.png
EOT

  # Create weston launcher (equivalent of .xinitrc)
  echo "Creating ~/run_weston.sh"
  cat <<EOT > $HOME/run_weston.sh
#!/usr/bin/env sh
export XDG_RUNTIME_DIR=\${XDG_RUNTIME_DIR:-/run/user/\$(id -u)}
mkdir -p "\$XDG_RUNTIME_DIR"
chmod 0700 "\$XDG_RUNTIME_DIR"

# Work around a libweston DRM-backend crash (assertion \`fb' failed in
# drm_output_find_plane_for_view) that can be triggered when a view
# without a real framebuffer (e.g. a solid-color background) gets
# considered for direct scanout on a hardware overlay plane. Forcing
# legacy (non-atomic) KMS avoids that code path.
export WESTON_DISABLE_ATOMIC=1

weston --config=$HOME/.config/weston.ini --socket=wayland-dash --idle-time=0 > $HOME/weston.log 2>&1 &
WESTON_PID=\$!

# Wait for the Wayland socket to be created before starting clients
# (a fixed socket name is used so this doesn't collide with, or get
# starved by, any other Wayland compositor already holding wayland-0,
# e.g. Raspberry Pi Connect's labwc/wayvnc session). Also bail out if
# weston dies before ever creating the socket, so we don't wait forever
# on a compositor that's already gone.
while [ ! -e "\$XDG_RUNTIME_DIR/wayland-dash" ]; do
  if ! kill -0 "\$WESTON_PID" 2>/dev/null; then
    echo "weston exited before creating its socket, see $HOME/weston.log" >&2
    exit 1
  fi
  sleep 0.2
done

# Keep restarting dash as long as weston is alive. If weston crashes
# (e.g. the DRM backend fault above), stop instead of restarting dash
# against a stale, dead socket.
while kill -0 "\$WESTON_PID" 2>/dev/null; do
  sh $HOME/run_dash_wayland.sh
done

kill \$WESTON_PID 2>/dev/null
EOT

  # Create runner
  echo "Creating ~/run_dash_wayland.sh and linking to ~/dash/bin/dash"
  cat <<EOT > $HOME/run_dash_wayland.sh
#!/usr/bin/env sh
export XDG_RUNTIME_DIR=\${XDG_RUNTIME_DIR:-/run/user/\$(id -u)}
export WAYLAND_DISPLAY=wayland-dash
export QT_QPA_PLATFORM=wayland
export QT_WAYLAND_DISABLE_WINDOWDECORATION=1
export QSG_RHI_BACKEND=opengl
export QSG_RENDER_LOOP=basic
export GST_GL_PLATFORM=egl
export GST_GL_WINDOW=wayland
export GST_GL_API=opengl
export GST_DEBUG=qmlglsink:5,glimagesink:5,xvimagesink:5,v4l2:3,glcontext:5,glwindow:5,qml6gl:5,gldisplay:5
$HOME/dash/bin/dash >> $HOME/dash/bin/dash.log 2>&1
sleep 1
EOT

  # Append to .bashrc
  echo "Appending weston autostart to ~/.bashrc"
  cat <<EOT >> $HOME/.bashrc

### wayland (weston)
if [ "\$(tty)" = "/dev/tty1" ]; then
  sh $HOME/run_weston.sh
fi
EOT

}

# Main Menu
while :
do
    case "$1" in
        -adi | --adddesktopicon)
            add_desktop_icon
            exit 0
          ;;
        -asd | --autostartdaemon)
            if [ $# -ne 0 ]; then
              create_autostart_daemon $2
              exit 0
            fi
          ;;
        -axi | --addxinit)
            if [ $# -ne 0 ]; then
              add_xinit_autostart
              exit 0
            fi
          ;;
        -awi | --addwaylandinit)
            if [ $# -ne 0 ]; then
              add_wayland_autostart
              exit 0
            fi
          ;;
        -h | --help)
            display_help  # Call your function
            exit 0
          ;;
        "")  # If $1 is blank, run display_help
            display_help
            exit 0
          ;;
        --) # End of all options
            shift
            break
          ;;
        -*)
            echo "Error: Unknown option: $1" >&2
            ## or call function display_help
            exit 1
          ;;
        *)  # No more options
            break
          ;;
    esac
done
