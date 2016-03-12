#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <inttypes.h>
#include <errno.h>

/*
 * http://read.pudn.com/downloads110/ebook/456020/E-EDID%20Standard.pdf
 */

#define CASCII_TO_ASCII(ca) (('A' - 1 + ((ca))))

int main()
{
	uint8_t buf[0x80];
	FILE *fp;

	int reti;
	ssize_t retss;

	fp = fopen("ed", "rb");
	if (fp == NULL) {
		fprintf(stderr, "fopen: %s\n", strerror(errno));
		goto exit_program;
	}

	retss = fread(buf, 1, 0x80, fp);
	if (retss != 0x80) {
		fprintf(stderr, "error: short read\n");
		goto close_file;
	}


	/* 3.3: Header */
	if (
		   buf[0] != 0x00
		|| buf[1] != 0xff
		|| buf[2] != 0xff
		|| buf[3] != 0xff
		|| buf[4] != 0xff
		|| buf[5] != 0xff
		|| buf[6] != 0xff
		|| buf[7] != 0x00
	) {
		fprintf(stderr, "error: invalid header pattern\n");
		goto close_file;
	}
	printf("Header is OK\n");


	/* Checksum */
	/* TODO: Check the validity of the checksum, not the sum. */
	{
		int i;
		uint8_t sum = 0;

		for (i = 0; i < 0x80; i ++)
			sum = (sum + buf[i]) & 0xff;

		if (sum != 0) {
			fprintf(stderr, "error: invalid checksum\n");
			goto close_file;
		}
	}
	printf("Checksum is OK\n");


	/* 3.4: Vendor / Product Identification */
	if (buf[0x08] & 0x80) {
		fprintf(stderr, "error: invalid bit in ID Manufacturer Name\n");
		goto close_file;
	}
	{
		uint8_t c1, c2, c3;

		c1 = CASCII_TO_ASCII((buf[0x08] & 0x7c) >> 2);
		c2 = CASCII_TO_ASCII(((buf[0x08] & 0x03) << 3) | ((buf[0x09] & 0xe0) >> 5));
		c3 = CASCII_TO_ASCII(buf[0x09] & 0x1f);

		if (!isupper(c1)) {
			fprintf(stderr, "error: ID Manufacturer Name Character 1 is invalid\n");
			goto close_file;
		}
		if (!isupper(c2)) {
			fprintf(stderr, "error: ID Manufacturer Name Character 2 is invalid\n");
			goto close_file;
		}
		if (!isupper(c3)) {
			fprintf(stderr, "error: ID Manufacturer Name Character 3 is invalid\n");
			goto close_file;
		}

		printf("ID Manufacturer Name: %c%c%c\n", c1, c2, c3);
	}

	printf("ID Product Code: 0x%04" PRIx16 "\n",
		  buf[0x0a]
		| (buf[0x0b] << 8)
	);

	printf("ID Serial Number: 0x%08" PRIx32 "\n",
		buf[0x0c]
		| (buf[0x0d] << 8)
		| (buf[0x0e] << 16)
		| (buf[0x0f] << 24)
	);

	if (buf[0x10] > 53) {
		fprintf(stderr, "error: Week of Manufacture (%d) is out of range\n", buf[0x10]);
		goto close_file;
	}
	printf("Week of Manufacture: %d\n", buf[0x10]);

	printf("Year of Manufacture: %d (year:%d)\n", buf[0x11], 1990 + buf[0x11]);


	/* 3.5: EDID Structure Version / Revision */
	if ((buf[0x12] != 1) || (buf[0x13] != 3)) {
		fprintf(stderr, "error: invalid EDID Structure Version / Revision (%" PRId8 " must be 1 and %" PRId8 " must be 3)\n", buf[0x12], buf[0x13]);
		goto close_file;
	}
	printf("Version no.: %" PRId8 "\n", buf[0x12]);
	printf("Revision no.: %" PRId8 "\n", buf[0x13]);

close_file:
	reti = fclose(fp);
	if (reti == EOF) {
		fprintf(stderr, "fclose: %s\n", strerror(errno));
		goto exit_program;
	}

exit_program:
	return 0;
}
