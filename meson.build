project('gtkam', 'c', version: '1.0')

i18n = import('i18n')

cc = meson.get_compiler('c')
add_project_arguments('-DHAVE_CC="@0@"'.format(cc.get_id()), language: 'c')

imagesdir = get_option('prefix') / get_option('datadir') / 'images' / 'gtkam'
icondir = get_option('prefix') / get_option('datadir') / 'pixmaps'

libgphoto2_dep        = dependency('libgphoto2', version: '>=2.5.0')
gtk_dep               = dependency('gtk+-2.0')
bonobo_activation_dep = dependency('bonobo-activation-2.0', required: get_option('bonobo'))
libbonoboui_dep       = dependency('libbonoboui-2.0', required: get_option('bonobo'))
libgnomeui_dep        = dependency('libgnomeui-2.0', required: get_option('gnomeui'))
gimpui_dep            = dependency('gimpui-2.0', required: get_option('gimpui'))
libexif_dep           = dependency('libexif', version: '>=0.3.2', required: false)
libexif_gtk_dep       = dependency('libexif-gtk', required: false)
intl_dep              = dependency('intl', required: false)

if bonobo_activation_dep.found() and libbonoboui_dep.found()
  add_project_arguments('-DHAVE_BONOBO=1', language: 'c')
endif

if libgnomeui_dep.found()
  add_project_arguments('-DHAVE_GNOME=1', language: 'c')
endif

if libexif_dep.found()
  add_project_arguments('-DHAVE_LIBEXIF=1', language: 'c')
endif

if libexif_gtk_dep.found()
  add_project_arguments('-DHAVE_LIBEXIF_GTK=1', language: 'c')
endif

if cc.has_function('gp_port_info_get_name', dependencies: libgphoto2_dep, prefix: '#include <gphoto2/gphoto2.h>')
  add_project_arguments('-DHAVE_GP_PORT_INFO_GET_NAME=1', language: 'c')
endif

if cc.has_function('gp_camera_set_timeout_funcs', dependencies: libgphoto2_dep, prefix: '#include <gphoto2/gphoto2.h>')
  add_project_arguments('-DHAVE_GP_CAMERA_SET_TIMEOUT_FUNCS=1', language: 'c')
endif

if gimpui_dep.found()
  gimp_version = '2.0'
  add_project_arguments('-DHAVE_GIMP=1', language: 'c')
endif

add_project_arguments('-DLOCALEDIR="@0@"'.format (get_option('prefix') / get_option('localedir')), language: 'c')
if intl_dep.found()
  add_project_arguments('-DENABLE_NLS=1', language: 'c')
endif

config_data = configuration_data()
config_data.set_quoted('PACKAGE', meson.project_name())
config_data.set_quoted('VERSION', meson.project_version())
config_data.set_quoted('IMAGE_DIR', imagesdir)
config_h = configure_file(
  output: 'config.h',
  configuration: config_data,
)

config_dep = declare_dependency(
  include_directories: include_directories('.'),
  sources: config_h
)

i18n.merge_file(
    input: 'gtkam.desktop.in',
    output: 'gtkam.desktop',
    install: true,
    install_dir: get_option('prefix') / get_option('datadir') / 'applications',
    po_dir: meson.project_source_root() / 'po',
    type: 'desktop'
)

install_data(
  'gtkam.png',
  install_dir: icondir,
)

subdir('doc')
subdir('src')
subdir('po')
subdir('art')
subdir('pixmaps')
subdir('test')
subdir('omf-install')
subdir('help')