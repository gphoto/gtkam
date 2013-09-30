pixmapdir = "$(datadir)/gtkam/pixmaps"

pixmap_DATA = camera.xpm configure.xpm delete_images.xpm exit.xpm folder.xpm save_current_image.xpm

EXTRA_DIST = camera.xpm close_image.xpm configure.xpm delete_images.xpm exit.xpm fliph.xpm flipv.xpm folder.xpm get_index_empty.xpm get_index.xpm get_selected_images.xpm no_thumbnail.xpm open_image.xpm print_image.xpm refresh.xpm resize.xpm rotc.xpm save_current_image.xpm

uninstall-hook:
	cd "$(DESTDIR)$(pixmapdir)" && rm -f $(pixmap_DATA)
