/**************************************************************************
 * PinNode 400 bulk mode: files between the node (XIAO) and the RP's SD card, on the link's SPI
 * wires, outside games. The owner's own sounds (SPEC-sound-upload) go laptop -> node over the LAN,
 * then node -> RP here. The same layout is in pinnode-node (pn_bulk.h).
 *
 * The node asks by putting BULK_MAGIC first in its next frame; the RP, if the game allows it (attract
 * or the operator menus) and a file handler is linked, then exchanges BULK_LEN-byte frames at
 * BULK_SPI_HZ every loop pass instead of the 64-byte frames, until END or BULK_IDLE_MS of nothing.
 *
 * Frame (both ways): [0] BULK_MAGIC [1] version [2] seq [3] op (node) / status (RP)
 *   [4..7] arg0 [8..11] arg1 [12..13] data length [14..15] 0 [16..63] name (NUL-terminated)
 *   A reply carries, at [16] and [20..23], the op and arg0 of the command it answers; the node checks them
 *   (0 there: an RP from before the echo), and at [24..27] a CRC-32 of its data alone (0: none). WRITE
 *   carries its data's CRC-32 in arg1. Both are checked where the data is used.
 *   [64..64+BULK_DATA) data [BULK_DATA+64..+4) CRC-32 (IEEE) of bytes 0..BULK_DATA+63, then padding.
 * Multi-byte fields little-endian. The node sends command `seq` until a reply carries the same seq;
 * the RP runs each new seq once and repeats its reply.
 **************************************************************************/
#ifndef PINNODE_BULK_H
#define PINNODE_BULK_H

#define BULK_MAGIC      0x5A
#define BULK_VERSION    2   // 2: 8 KB data (1: 4 KB)
#define BULK_DATA       8192
#define BULK_HDR        64
#define BULK_LEN        (BULK_HDR + BULK_DATA + 8)      // CRC + padding to a multiple of 8
#define BULK_SPI_HZ     16000000UL
#define BULK_IDLE_MS    3000
#define BULK_NAME_MAX   48

// ops (node -> RP)
#define BULK_OP_NOP     1   // keep the session open
#define BULK_OP_INFO    2   // -> arg0 1 = card ok
#define BULK_OP_LIST    3   // name = folder ("" root), arg0 = entries to skip -> data [size u32][flags u8: 1 folder]
                            // [n u8][name n]..., arg0 = count, arg1 = 1 more. Hidden (.) entries are left out
#define BULK_OP_OPEN    4   // name (a .wav path), arg0 = size: start writing (to a temporary file)
#define BULK_OP_WRITE   5   // arg0 = offset, arg1 = CRC-32 of the data (0: unchecked), data
#define BULK_OP_CLOSE   6   // name, arg0 = size: the file is whole, replace `name` with it
#define BULK_OP_DELETE  7   // name: a file, or an empty folder
#define BULK_OP_END     8   // back to the 64-byte link
#define BULK_OP_MKDIR  10   // name
#define BULK_OP_MOVE   11   // name -> the path in data (a rename on the card: instant)
#define BULK_OP_COPY   12   // name -> the path in data: starts a copy -> arg1 = size
#define BULK_OP_COPY_STEP 13 // copies up to 64 KB -> arg0 = done, arg1 = size; finished when equal
#define BULK_OP_READ   14   // name, arg0 = offset -> data (up to BULK_DATA), arg0 = bytes, arg1 = size
// Paths: folders joined by '/', no leading '/', < BULK_NAME_MAX; segments of letters, digits, space
// . _ - ( ), none starting with '.'. Only the root's NNN_name.wav files play; uploads, moves and
// copies of .wav files only.

// status (RP -> node)
#define BULK_OK          0
#define BULK_E_NO_CARD   2
#define BULK_E_BAD_NAME  3
#define BULK_E_IO        4
#define BULK_E_NO_FILE   5
#define BULK_E_OFFSET    6
#define BULK_E_FULL      7
#define BULK_E_REFUSED   8   // not now (a game, or no file handler)
#define BULK_E_OP        9
#define BULK_E_EXISTS   10   // the destination is there already
#define BULK_E_NOT_EMPTY 11  // a folder with things in it

#ifdef __cplusplus
extern "C" {
#endif
// Provided by the game's sound player (RPU Rev 400's RPU400Audio); weak in the library. Runs one op;
// returns a status. out (BULK_DATA bytes) and *outLen / *r0 / *r1 carry a reply's data.
int PinNode400_FileOp(unsigned char op, const char *name, unsigned long arg0, unsigned long arg1,
                      const unsigned char *data, unsigned int len, unsigned char *out, unsigned int *outLen,
                      unsigned long *r0, unsigned long *r1);
#ifdef __cplusplus
}
#endif

#endif
