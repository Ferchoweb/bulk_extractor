/*
 * scan_outlook: optimistically decrypt Outlook Compressible Encryption
 * author:   Simson L. Garfinkel
 * created:  2014-01-28
 */
#include "config.h"

#include "be13_api/scanner_params.h"
#include "scan_outlook.h"
#include "utils.h"

/*
 * Below is the decoding array, i.e.:
 *
 * unsigned char plaintext_c = libpff_encryption_compressible[ ciphertext_c ];
 *
 * This comes from Herr Metz's libpff/libpff_encryption.c, under LGPL.
 *
 * Outlook also has a "high compression" scheme, that's basically a
 * salted three-rotor scheme.
 *
 * Inverse map to encrypt:
 */

/* The transposition array contains the un-encrypted (plain) values.
 * The un-encrypted value is in the position of the encrypted value.
 * i.e. the encrypted value 0x13 represents 0x02
 * encrypted:		0     1     2     3     4     5     6     7
 * un-encrypted:	8     9     a     b     c     d     e     f
 */

/* For compressible encryption
 */
static uint8_t libpff_encryption_compressible[] = {
	0x47, 0xf1, 0xb4, 0xe6, 0x0b, 0x6a, 0x72, 0x48, 0x85, 0x4e, 0x9e, 0xeb, 0xe2, 0xf8, 0x94, 0x53,
	0xe0, 0xbb, 0xa0, 0x02, 0xe8, 0x5a, 0x09, 0xab, 0xdb, 0xe3, 0xba, 0xc6, 0x7c, 0xc3, 0x10, 0xdd,
	0x39, 0x05, 0x96, 0x30, 0xf5, 0x37, 0x60, 0x82, 0x8c, 0xc9, 0x13, 0x4a, 0x6b, 0x1d, 0xf3, 0xfb,
	0x8f, 0x26, 0x97, 0xca, 0x91, 0x17, 0x01, 0xc4, 0x32, 0x2d, 0x6e, 0x31, 0x95, 0xff, 0xd9, 0x23,
	0xd1, 0x00, 0x5e, 0x79, 0xdc, 0x44, 0x3b, 0x1a, 0x28, 0xc5, 0x61, 0x57, 0x20, 0x90, 0x3d, 0x83,
	0xb9, 0x43, 0xbe, 0x67, 0xd2, 0x46, 0x42, 0x76, 0xc0, 0x6d, 0x5b, 0x7e, 0xb2, 0x0f, 0x16, 0x29,
	0x3c, 0xa9, 0x03, 0x54, 0x0d, 0xda, 0x5d, 0xdf, 0xf6, 0xb7, 0xc7, 0x62, 0xcd, 0x8d, 0x06, 0xd3,
	0x69, 0x5c, 0x86, 0xd6, 0x14, 0xf7, 0xa5, 0x66, 0x75, 0xac, 0xb1, 0xe9, 0x45, 0x21, 0x70, 0x0c,
	0x87, 0x9f, 0x74, 0xa4, 0x22, 0x4c, 0x6f, 0xbf, 0x1f, 0x56, 0xaa, 0x2e, 0xb3, 0x78, 0x33, 0x50,
	0xb0, 0xa3, 0x92, 0xbc, 0xcf, 0x19, 0x1c, 0xa7, 0x63, 0xcb, 0x1e, 0x4d, 0x3e, 0x4b, 0x1b, 0x9b,
	0x4f, 0xe7, 0xf0, 0xee, 0xad, 0x3a, 0xb5, 0x59, 0x04, 0xea, 0x40, 0x55, 0x25, 0x51, 0xe5, 0x7a,
	0x89, 0x38, 0x68, 0x52, 0x7b, 0xfc, 0x27, 0xae, 0xd7, 0xbd, 0xfa, 0x07, 0xf4, 0xcc, 0x8e, 0x5f,
	0xef, 0x35, 0x9c, 0x84, 0x2b, 0x15, 0xd5, 0x77, 0x34, 0x49, 0xb6, 0x12, 0x0a, 0x7f, 0x71, 0x88,
	0xfd, 0x9d, 0x18, 0x41, 0x7d, 0x93, 0xd8, 0x58, 0x2c, 0xce, 0xfe, 0x24, 0xaf, 0xde, 0xb8, 0x36,
	0xc8, 0xa1, 0x80, 0xa6, 0x99, 0x98, 0xa8, 0x2f, 0x0e, 0x81, 0x65, 0x73, 0xe4, 0xc2, 0xa2, 0x8a,
	0xd4, 0xe1, 0x11, 0xd0, 0x08, 0x8b, 0x2a, 0xf2, 0xed, 0x9a, 0x64, 0x3f, 0xc1, 0x6c, 0xf9, 0xec
};



extern "C"
void scan_outlook(scanner_params &sp)
{
    sp.check_version();
    if(sp.phase==scanner_params::PHASE_INIT) {
        auto info = new scanner_params::scanner_info( scan_outlook, "outlook" ); scan_outlook, "outlook" );
    //auto info = new scanner_params::scanner_info(
	//info->name  = "outlook";
	info->author = "Simson L. Garfinkel";
	info->description = "Outlook Compressible Encryption";
	info->flags = scanner_info::SCANNER_DISABLED \
            | scanner_info::SCANNER_RECURSE | scanner_info::SCANNER_DEPTH_0 ;
        sp.register_info(info);
	return;
    }
    if(sp.phase==scanner_params::PHASE_SCAN) {
	const sbuf_t &sbuf = sp.sbuf;
	const pos0_t &pos0 = sp.sbuf.pos0;

        // dodge infinite recursion by refusing to operate on an OFE'd buffer
        if(rcb.partName == pos0.lastAddedPart()) {
            return;
        }

        // managed_malloc throws an exception if allocation fails.
        managed_malloc<uint8_t>dbuf(sbuf.bufsize);
        for(size_t ii = 0; ii < sbuf.bufsize; ii++) {
            uint8_t ch = sbuf.buf[ii];
            dbuf.buf[ii] = libpff_encryption_compressible[ ch ];
        }

        const pos0_t pos0_oce = pos0 + "OUTLOOK";
        const sbuf_t child_sbuf(pos0_oce, dbuf.buf, sbuf.bufsize, sbuf.pagesize, 0, false);
        scanner_params child_params(sp, child_sbuf);
        (*rcb.callback)(child_params);    // recurse on deobfuscated buffer
    }
}
