desktopfiles_in_files = \
	mutter.desktop.in
desktopfilesdir = $(datadir)/applications
desktopfiles_DATA = $(desktopfiles_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

xml_in_files = \
        50-mutter-navigation.xml.in \
        50-mutter-system.xml.in \
        50-mutter-windows.xml.in
xmldir = $(GNOME_KEYBINDINGS_KEYSDIR)
xml_DATA = $(xml_in_files:.xml.in=.xml)

gschema_in_files = \
	org.gnome.mutter.gschema.xml.in	\
	org.gnome.mutter.wayland.gschema.xml.in
gsettings_SCHEMAS = $(gschema_in_files:.xml.in=.xml)

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

convertdir = $(datadir)/GConf/gsettings
convert_DATA = mutter-schemas.convert

CLEANFILES = \
	$(desktopfiles_DATA) \
	$(gsettings_SCHEMAS) \
	$(xml_DATA)

EXTRA_DIST = \
	$(convert_DATA) \
	$(desktopfiles_in_files) \
	$(gschema_in_files) \
	$(xml_in_files)

# rename files with prefix 'deepin-'
install-data-hook:
	mv $(DESTDIR)$(desktopfilesdir)/mutter.desktop $(DESTDIR)$(desktopfilesdir)/deepin-mutter.desktop
	mv $(DESTDIR)$(xmldir)/50-mutter-navigation.xml $(DESTDIR)$(xmldir)/50-deepin-mutter-navigation.xml
	mv $(DESTDIR)$(xmldir)/50-mutter-system.xml $(DESTDIR)$(xmldir)/50-deepin-mutter-system.xml
	mv $(DESTDIR)$(xmldir)/50-mutter-windows.xml $(DESTDIR)$(xmldir)/50-deepin-mutter-windows.xml
	mv $(DESTDIR)$(gsettingsschemadir)/org.gnome.mutter.gschema.xml $(DESTDIR)$(gsettingsschemadir)/com.deepin.wrap.gnome.mutter.gschema.xml
	mv $(DESTDIR)$(gsettingsschemadir)/org.gnome.mutter.wayland.gschema.xml $(DESTDIR)$(gsettingsschemadir)/com.deepin.wrap.gnome.mutter.wayland.gschema.xml
	mv $(DESTDIR)$(convertdir)/mutter-schemas.convert $(DESTDIR)$(convertdir)/deepin-mutter-schemas.convert
