#ifndef BH_RIFF_H
#define BH_RIFF_H
/* Complete 0047b830 optional-output/early-exit scanner with explicit input size.
   Outputs are borrowed pointers; input must outlive them. All provided outputs
   are zeroed initially, and partial discoveries survive failure as originally.
   Only requested fmt is required to have >=14 bytes. Unknown/unused chunks are
   skipped, and successful requested outputs stop scanning immediately. The
   original empty-data/duplicate-chunk rules depend on which outputs are asked
   for; no PCM codec validation or complete-tail validation is added here.
   Bounded container/chunk spans reject unsafe original out-of-bounds accesses.
   No allocation and no original-code execution. */
int __cdecl bh_riff_scan(const void *bytes, unsigned long size, const unsigned char **format,
                         const unsigned char **samples, unsigned long *sample_bytes);
#endif
