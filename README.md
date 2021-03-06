# fastdecode-utf-8
SIMD-accelerated UTF-8 to UTF-32 conversion


This is a UTF-8 decoder (to UTF-32) using Intel SIMD instructions (so far SSE only).  It's based on an idea I had for doing rapid selection in variable-width data.

Given a  packed sequence of variable-length, byte-encoded values where the length is encoded in the first byte, selection of the k-th value is remarkably easy despite the variability.
A mapping P can be formed in-place where each initial byte points to the next, which can be manipulated by the SIMD shuffle instruction.

Example:  A sequence of initial (shown as lengths) and continuation (shown as ".") bytes:

    S = 2,.,1,3,.,.,2,.,1,1,1

is transformed by simple arithmetic to

    P = 2,.,3,6,.,.,8,.,9,10,11
    
where, starting with position 0, each entry points to the next in sequence (a linked list, but with 4-bit pointers).

The pshufb instruction dereferences such a list of indexes to the values at those indexes in a source register, so applying P above will shift the initial byte of the second value to the first byte (shifting left by one), and we can compose P^2, P^4, etc. to shift by other amounts. 
That composability is the basis of selecting the k-th value in O(log k) time despite the variability in its position; we simply exponentiate P^k, and the first byte will contain the offset of the k-th element.  It's the power of permutation.

These shuffles, once constructed, allow gathering bytes from multiple characters at a time into fixed positions, or generalized suffix-summing, where each position aggregates information from its following k consecutive values.  They're quite fun.

One approach shown here is to aggregate, for each UTF-8 character, the lengths of it and its 3 following characters into a code byte which is used to look up a decoding shuffle from a table.

The aggregation process is similar to the brute-force SIMD prefix sum algorithm:  aggregate adjacent elements into pairs (using P), and then aggregate pairs into quadruples (using P^2).  We then iterate through quadruples by building P^4 and storing it to a byte array.

Another approach is to shift the first, eg, 4 characters into fixed positions by adding 3 fixed-size dummy elements to the beginning of the array and building P^3 from that.  Shifting everything left by 3 puts the offsets of the first 4 non-dummy characters into bytes 0-3, and they are used to build a decoding shuffle into the 4xuint32 output.  This approach seems slower vs. the first approach, but it looks like it will scale better for wider registers (eg AVX512).  The code-LUT approach is limited by the fact that only 4 2-bit lengths will fit in a code byte.

However prepending dummy positions means truncating bytes from the right, so some care is needed.

(Code for the dummy-positions approach is not in the initial checkin; I will remove this sentence when it is.)

## BENCHMARKS

From the "benchmark" target.  On clang-4.0, Skylake, input is 65536 bytes of UTF-8 data.  Cycles are reported per input byte.

| Input        | Cycles/Byte | output chars | 
|--------------|-------------|--------------|
| ASCII        | 2.618       | 65536        |
| UTF-8 3-byte | 2.218       | 21846        | 
| Random 1/3B  | 3.39        | 32926        | 

ASCII data is randomly-generated bytes in the ASCII range (0-128).

3-byte data is the letter 'ㄱ' repeated 21845 times, followed by 'a' as a filler.

Random is generated as simple 50/50-distributed 1 or 3-byte sequences, but it's enough to show that branch prediction is likely biasing the simpler benchmarks.

The benchmark framework is due to Daniel Lemire.

### Performance Notes

The code has a number of branch points and variable-length loops, based on traversing the data 4 unicode characters at a time, where width can vary from 4 to 16 (yielding 1-4 iterations per SSE register), and handling ASCII-only as a special case (4 bytes are converted directly to uint32_t's, without expensive masking and bitfield-compression steps).  So far experiments with unrolling the traversal and dispatching multiple convert/shuffle-compress operations at a time (see the "unroll" branch) have been slower than the branchy code.

## RELATED WORK

This project had its inspiration in work we did for validation (https://github.com/lemire/fastvalidate-utf-8), and we still have an issue open on that project deciding whether we should do decoding as well, but the two techniques are not integrated as of yet.  So it should be noted that this code only expects valid UTF-8.

## BUILDING

    $ make test
    $ make benchmark

"test" and "benchmark" are the only targets right now.
