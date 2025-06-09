omf_data = configuration_data()
omf_data.set('GTKAM_C_IDENTIFIER', 'file:/@0@/gnome/help/gtkam/C/gtkam.xml'.format(get_option('prefix') / get_option('datadir')))

configure_file(
  input: 'gtkam-C.omf.in',
  output: '@BASENAME@',
  configuration: omf_data,
  install: true,
  install_dir: get_option('prefix') / get_option('datadir') / 'omf' / 'gtkam',
)