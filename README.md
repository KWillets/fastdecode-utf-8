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
That composability is the basis of selecting the k-th value in O(log k) time despite the variability in its position; we simply exponentiate P^k, and the first byte will contain the offset of the k-th element.

These shuffles, once constructed, allow gathering bytes from multiple characters at a time into fixed positions, or generalized suffix-summing, where each position aggregates information from its following k consecutive values.  They're quite fun.

One approach shown here is to aggregate, for each UTF-8 character, the lengths of it and its 3 following characters into a code byte which is used to look up a decoding shuffle from a table.

The aggregation process is similar to the brute-force SIMD prefix sum algorithm:  aggregate adjacent elements into pairs (using P), and then aggregate pairs into quadruples (using P^2).  We then iterate through quadruples by building P^4 and storing it to a byte array.

Another approach is to shift the first, eg, 4 characters into fixed positions by adding 3 fixed-size dummy elements to the beginning of the array and building P^3 from that.  Shifting everything left by 3 puts the offsets of the first 4 non-dummy characters into bytes 0-3, and they are used to build a decoding shuffle into the 4xuint32 output.  This approach seems slower vs. the first approach, but it looks like it will scale better for wider registers (eg AVX512).  The code-LUT approach is limited by the fact that only 4 2-bit lengths will fit in a code byte.

However prepending dummy positions means truncating bytes from the right, so some care is needed.

(Code for the dummy-positions approach is not in the initial checkin; I will remove this sentence when it is.)


## BUILDING

    $ make

"test" is the only target right now.