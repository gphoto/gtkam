libgtkam_lib = static_library('gtkam-static',
  'gdk-pixbuf-hacks.c', 'gdk-pixbuf-hacks.h',
  'gtkam-camera.c',     'gtkam-camera.h',
  'gtkam-cancel.c',     'gtkam-cancel.h',
  'gtkam-chooser.c',    'gtkam-chooser.h',
  'gtkam-clock.c',      'gtkam-clock.h',
  'gtkam-close.c',      'gtkam-close.h',
  'gtkam-config.c',     'gtkam-config.h',
  'gtkam-context.c',    'gtkam-context.h',
  'gtkam-delete.c',     'gtkam-delete.h',
  'gtkam-dialog.c',     'gtkam-dialog.h',
  'gtkam-error.c',      'gtkam-error.h',
  'gtkam-exif.c',       'gtkam-exif.h',
  'gtkam-fsel.c',       'gtkam-fsel.h',
  'gtkam-info.c',       'gtkam-info.h',
  'gtkam-list.c',       'gtkam-list.h',
  'gtkam-mkdir.c',      'gtkam-mkdir.h',
  'gtkam-port.c',       'gtkam-port.h',
  'gtkam-preview.c',    'gtkam-preview.h',
  'gtkam-save.c',       'gtkam-save.h',
  'gtkam-status.c',     'gtkam-status.h',
  'gtkam-tree.c',       'gtkam-tree.h',
  'gtkam-util.c',       'gtkam-util.h',
  'gtkam-viewer.c',     'gtkam-viewer.h',
  'gtkam-version.c',    'gtkam-version.h',
  'util.c',             'util.h',
  dependencies: [
    libgphoto2_dep,
    gtk_dep,
    bonobo_activation_dep,
    libbonoboui_dep,
    libgnomeui_dep,
    gimpui_dep,
    libexif_dep,
    libexif_gtk_dep,
    intl_dep,
    config_dep,
  ]
)

libgtkam_dep = declare_dependency(
  link_whole: libgtkam_lib,
  include_directories: include_directories('.'),
  dependencies: [
    libgphoto2_dep,
    gtk_dep,
    bonobo_activation_dep,
    libbonoboui_dep,
    libgnomeui_dep,
    gimpui_dep,
    libexif_dep,
    libexif_gtk_dep,
    intl_dep,
    config_dep,
  ]
)

gtkam = executable('gtkam',
  'gtkam-debug.c', 'gtkam-debug.h',
  'gtkam-main.c', 'gtkam-main.h',
  'i18n.h', 'main.c',
  dependencies: libgtkam_dep,
  install: true
)

if gimpui_dep.found()
  executable('gtkam-gimp',
    'gtkam-gimp.c',
    install: true,
    install_dir: get_option('prefix') / get_option('libdir') / 'gimp' / gimp_version / 'plug-ins',
    dependencies: libgtkam_dep,
  )
endif