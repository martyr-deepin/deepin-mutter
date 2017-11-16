# Deepin Mutter

**Description**: Base window manager component for Deepin.

Mutter is a window manager for the X Window System, and is in the
process of becoming a Wayland compositor; it became the default window
manager in GNOME 3, replacing Metacity which uses GTK+ for rendering,
while Mutter uses a graphics library called Clutter, and through it
also supports OpenGL.

Mutter is extensible with plugins and supports numerous visual
effects. For example,
[deepin-wm](https://github.com/linuxdeepin/deepin-wm) is written as a
plugin to Mutter.

This project is a fork of Mutter to avoid DDE depending on GNOME with
some special hacking work, include applying some patches, using the
gsettings under `com.deepin.wrap.gnome` instead of `org.gnome`,
etc. And Deepin Mutter just could coexists with Mutter to make the
porting work easier.

## Dependencies

### Build dependencies

- debhelper (>= 9)
- intltool (>= 0.41)
- gtk-doc-tools (>= 1.15)
- gobject-introspection (>= 1.41.3)
- gsettings-desktop-schemas (>= 3.15.92)

### Runtime dependencies

- cairo (>= 1.10.0)
- clutter-1.0 (>= 1.21.3)
- clutter-wayland-1.0
- clutter-wayland-compositor-1.0
- cogl-1.0 (>= 1.17.1)
- [deepin-desktop-schemas](https://github.com/linuxdeepin/deepin-desktop-schemas)
- gbm (>= 10.3)
- glib2.0 (>= 2.35.1)
- gnome-desktop-3.0 (>= 3.10)
- gnome-themes-standard
- gobject-introspection-1.0 (>= 0.9.12)
- gtk+-3.0 (>= 3.9.11)
- gudev-1.0
- ice
- json-glib-1.0 (>= 0.13.2)
- libcanberra-gtk3
- libinput
- libstartup-notification-1.0 (>= 0.7)
- libsystemd (>= 212)
- pango (>= 1.2.0)
- sm
- upower-glib (>= 0.99.0)
- wayland-server (>= 1.6.90)
- x11
- x11-xcb
- xcb-randr
- xcomposite (>= 1:0.2)
- xcursor
- xdamage
- xext
- xfixes
- xi (>= 2:1.6.0)
- xinerama
- xkbcommon (>= 0.4.3)
- xkbcommon-x11
- xkbfile
- xrandr
- xrender
- xt
- zenity

## Installation

### Debian 8.0 (jessie)

Install prerequisites
```
$ sudo apt-get install \
               cdbs \
               debhelper \
               gnome-pkg-tools \
               dh-autoreconf \
               intltool \
               gtk-doc-tools \
               gobject-introspection \
               gsettings-desktop-schemas-dev \
               deepin-desktop-schemas \
               deepin-desktop-schemas \
               gnome-themes-standard \
               libcairo2-dev \
               libcanberra-gtk3-dev \
               libclutter-1.0-dev \
               libcogl-dev \
               libgbm-dev \
               libgirepository1.0-dev \
               libglib2.0-dev \
               libgnome-desktop-3-dev \
               libgtk-3-dev \
               libgudev-1.0-dev \
               libice-dev \
               libinput-dev \
               libjson-glib-dev \
               libpam0g-dev \
               libpango1.0-dev \
               libsm-dev \
               libstartup-notification0-dev \
               libsystemd-dev \
               libupower-glib-dev \
               libwayland-dev \
               libx11-dev \
               libx11-xcb-dev \
               libxcb-randr0-dev \
               libxcomposite-dev \
               libxcursor-dev \
               libxdamage-dev \
               libxext-dev \
               libxfixes-dev \
               libxi-dev \
               libxinerama-dev \
               libxkbcommon-dev \
               libxkbcommon-x11-dev \
               libxkbfile-dev \
               libxrandr-dev \
               libxrender-dev \
               libxt-dev \
               xkb-data \
               zenity
```

Build
```
$ ./autogen.sh --prefix=/usr \
               --libexecdir=/usr/lib/deepin-mutter \
               --sysconfdir=/etc \
               --enable-gtk-doc \
               --disable-schemas-compile \
               --enable-wayland \
               --enable-native-backend \
               --enable-compile-warnings=minimum
$ make
```

If you have isolated testing build environment (say a docker container), you can install it directly
```
$ sudo make install
```

Or, generate package files and install Deepin Mutter with it
```
$ debuild -uc -us ...
$ sudo dpkg -i ../deepin-mutter-*deb
```

## Usage

Run Deepin Mutter to replace current window manager with the command below
```
$ deepin-mutter --replace &
```

## Getting help

Any usage issues can ask for help via

* [Gitter](https://gitter.im/orgs/linuxdeepin/rooms)
* [IRC channel](https://webchat.freenode.net/?channels=deepin)
* [Forum](https://bbs.deepin.org)
* [WiKi](https://wiki.deepin.org/)

## Getting involved

We encourage you to report issues and contribute changes

* [Contribution guide for developers](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers-en). (English)
* [开发者代码贡献指南](https://github.com/linuxdeepin/developer-center/wiki/Contribution-Guidelines-for-Developers) (中文)

## License

Deepin Mutter is licensed under [GPLv3](LICENSE).
