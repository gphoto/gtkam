#include <config.h>

#include <stdio.h>

#include <libjpeg/jpeg-data.h>

int
main (int argc, char **argv)
{
	JPEGData *data;
	ExifData *edata;
	FILE *f;

	if (argc <= 1) {
		printf ("You need to specify a file!\n");
		return (1);
	}

	data = jpeg_data_new_from_file (argv[1]);
	if (!data) {
		printf ("Could not load '%s'!\n", argv[1]);
		return (1);
	}
	jpeg_data_dump (data);
	edata = jpeg_data_get_exif_data (data);
	if (edata && edata->data) {
		f = fopen ("test.jpeg", "w");
		if (f) {
			fwrite (edata->data, 1, edata->size, f);
			fclose (f);
		}
	}

	jpeg_data_free (data);

	return (0);
}
