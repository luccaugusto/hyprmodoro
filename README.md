# Hyprmodoro

A Pomodoro timer plugin for [hyprland](https://github.com/hyprwm/hyprland).
Fork from https://github.com/0xFMD/hyprmodoro with sound and notifications support.

![preview](https://github.com/user-attachments/assets/61799c82-f1c5-4d98-b861-3cda51b76134)

## Installation

### hyprpm

```bash
hyprpm add https://github.com/0xFMD/hyprmodoro
hyprpm enable hyprmodoro
```

### Manual
> [!NOTE]
> [Hyprland headers must be installed first.](https://wiki.hypr.land/Plugins/Using-Plugins/#manual)

```bash
git clone https://github.com/0xFMD/hyprmodoro
cd hyprmodoro
make all
hyprctl plugin load $(pwd)/hyprmodoro.so
```

## Configuration

Add to your `hyprland.conf`:

```ini
# Enable the plugin
plugin:hyprmodoro:enabled                     # (default: true)

# Timer durations in minutes
plugin:hyprmodoro:work_duration               # Work session length (default: 25)
plugin:hyprmodoro:rest_duration               # Rest session length (default: 5)

# Auto-transition between sessions
plugin:hyprmodoro:auto_transition             # Automatically transition between work/rest (default: true)
                                              # Note: Forced to true if title:enabled is false

# Title configurations
plugin:hyprmodoro:title:enabled               # Show timer on window title bars (default: true)
plugin:hyprmodoro:title:all_windows           # Show title on all windows or only the active window (default: false)
plugin:hyprmodoro:title:reserve_space_all     # Reserve space for title bar for all windows (default: false)
plugin:hyprmodoro:title:floating_window       # Show title on floating windows (default: false)
plugin:hyprmodoro:title:margin                # Margin around title elements (default: 15)
plugin:hyprmodoro:title:spacing               # Spacing between elements (default: 8)

# Progress border configurations
plugin:hyprmodoro:border:enabled              # Show progress border (default: true)
plugin:hyprmodoro:border:all_windows          # Show progress border on all windows or only the active window (default: false)
plugin:hyprmodoro:border:floating_window      # Show progress border on floating windows (default: false)
plugin:hyprmodoro:border:color                # Progress border color (default: rgba(33333388))

# Text configurations
plugin:hyprmodoro:text:color                  # Timer text color (default: rgba(ffffffff))
plugin:hyprmodoro:text:font                   # Font family (default: Sans)
plugin:hyprmodoro:text:size                   # Font size (default: 17)
plugin:hyprmodoro:text:work_prefix            # Prefix for work sessions (default: 🍅)
plugin:hyprmodoro:text:rest_prefix            # Prefix for rest sessions (default: ☕)
plugin:hyprmodoro:text:skip_on_click          # Allow skipping a session by clicking the timer (default: true)

# Sound configurations
# Note: Sounds will ONLY play if player, work_end, and rest_end are all configured
plugin:hyprmodoro:sound:player                # Audio player command (default: pw-play)
plugin:hyprmodoro:sound:work_end              # Absolute path to sound file for work session end (default: empty)
plugin:hyprmodoro:sound:rest_end              # Absolute path to sound file for rest session end (default: empty)

# Notification configurations
plugin:hyprmodoro:notification:enabled              # Enable desktop notifications (default: true)
plugin:hyprmodoro:notification:use_system_notifications  # Use system notifications (notify-send) instead of Hyprland notifications (default: false)
plugin:hyprmodoro:notification:work_end             # Notification message for work end (default: "Work session complete")
plugin:hyprmodoro:notification:rest_end             # Notification message for rest end (default: "Break is over")

# Buttons configurations
plugin:hyprmodoro:buttons:size                # Button size (default: 17)
plugin:hyprmodoro:buttons:color:foreground    # Button text color (default: rgba(ffffffff))
plugin:hyprmodoro:buttons:color:background    # Button background color (default: rgba(ffffff44))

# Hover behavior
plugin:hyprmodoro:hover:text                  # Show text on hover (default: false)
plugin:hyprmodoro:hover:buttons               # Show buttons on hover (default: true)
plugin:hyprmodoro:hover:height                # Percentage of window height for hover area (default: 10.0)
plugin:hyprmodoro:hover:width                 # Percentage of window width for hover area (default: 20.0)

# Window configurations
plugin:hyprmodoro:window:min_width            # Minimum window width to show timer (default: 300)
plugin:hyprmodoro:window:padding              # Window padding (default: 0)
```


## Dispatchers

```ini
# Start timer
hyprctl dispatch hyprmodoro:start

# Stop timer
hyprctl dispatch hyprmodoro:stop

# Pause/resume timer
hyprctl dispatch hyprmodoro:pause

# Skip current session
hyprctl dispatch hyprmodoro:skip

# Set custom work duration (in minutes)
hyprctl dispatch hyprmodoro:set 25

# Set custom rest duration (in minutes)
hyprctl dispatch hyprmodoro:setRest 5
```

## HyprCtl Commands

```bash
# Get current progress
hyprctl hyprmodoro:getProgress

# Get current state
hyprctl hyprmodoro:getState

# Get remaining time
hyprctl hyprmodoro:getTime
```
