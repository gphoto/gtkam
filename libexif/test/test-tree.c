#include <config.h>

#include <stdio.h>

#include <libjpeg/jpeg-data.h>

int
main (int argc, char **argv)
{
	JPEGData *data;

	if (argc <= 1) {
		printf ("You need to specify a file!");
		return (1);
	}

	data = jpeg_data_new_from_file (argv[1]);
	jpeg_data_dump (data);
	jpeg_data_free (data);

	return (0);
}
