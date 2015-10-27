## Install debug pacakges
```
sudo apt-get install gdb deepin-mutter-dbg libx11-6-dbg \
     libglib2.0-0-dbg libgtk-3-0-dbg libclutter-1.0-dbg libcogl20-dbg \
     xserver-xorg-core-dbg libgl1-mesa-glx-dbg libegl1-mesa-dbg \
     libpixman-1-0-dbg libcogl20-dbg libgl1-mesa-dri-dbg
```

## Debug in source directory
```
./autogen.sh --prefix=/usr \
             --libexecdir=/usr/lib/deepin-mutter \
             --sysconfdir=/etc \
             --enable-gtk-doc \
             --disable-schemas-compile \
             --enable-wayland \
             --enable-native-backend \
             --enable-compile-warnings=minimum
make
gdb src/deepin-mutter
```

## Merge upstream code

Be careful, Mutter owns multiple release branches, and they always
will not merge back to `master`. So we must follow all the related
branches to avoid merge faild.

For example, if need to sync to the latest Mutter `3.16.3` code, follow
the steps:

1. Add Mutter git repository locally
```
git remote add upstream https://github.com/GNOME/mutter
```

2. Merge tag `3.16.1` which located in `upstream/master` branch
```
git checkout master
git merge 3.16.1
# fix conflicts if need
```

3. Create new branch `release/3.16`
```
git checkout -b release/3.16
```

4. Merge tag `3.16.3` which located in `upstream/gnome-3-16` branch
```
git checkout release/3.16
git merge 3.16.3
# fix conflicts if need
```

5. Give a tag and push to originally repository
```
git checkout master
git push origin master
git checkout release/3.16
git tag -a -m 'bump version to 3.16.3.0' 3.16.3.0
git push origin release/3.16
```
