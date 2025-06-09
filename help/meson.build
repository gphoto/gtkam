helpdir = get_option('prefix') / get_option('datadir') / 'gnome' / 'help'

langs = ['C', 'ru']
foreach lang : langs
  install_data(lang / 'gtkam.xml', lang / 'legal.xml',
    install_dir: helpdir / lang,
  )
  install_subdir('@0@/figures'.format(lang),
    install_dir: helpdir / lang,
  )
endforeach