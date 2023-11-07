#include <stdio.h>
#include <string.h>
#include "lzf.h" // Include the correct header file for lzf functions
#include <ctype.h>
#include <stdint.h>

// Input data (char data type)
char input_data[] = "9.929577708244323730e-01,5.422534942626953125e-01,5.422534942626953125e-01,5.528169274330139160e-01,5.492957830429077148e-01,5.246478915214538574e-01,5.281690359115600586e-01,4.612676203250885010e-01,3.204225301742553711e-01,2.077464759349822998e-01,4.577464610338211060e-02,1.091549322009086609e-01,1.408450752496719360e-01,1.408450752496719360e-01,1.549295783042907715e-01,2.183098644018173218e-01,1.795774698257446289e-01,1.619718372821807861e-01,1.584507077932357788e-01,1.690140813589096069e-01,1.514084488153457642e-01,1.584507077932357788e-01,1.549295783042907715e-01,1.584507077932357788e-01,1.549295783042907715e-01,1.443661898374557495e-01,1.373239457607269287e-01,1.549295783042907715e-01,1.408450752496719360e-01,1.514084488153457642e-01,1.443661898374557495e-01,1.549295783042907715e-01,1.584507077932357788e-01,1.901408433914184570e-01,1.866197139024734497e-01,2.218309789896011353e-01,2.323943674564361572e-01,2.605633735656738281e-01,2.781690061092376709e-01,3.133802711963653564e-01,3.239436745643615723e-01,3.309859037399291992e-01,3.309859037399291992e-01,3.415493071079254150e-01,3.204225301742553711e-01,3.239436745643615723e-01,2.922535240650177002e-01,3.133802711963653564e-01,2.887323796749114990e-01,2.781690061092376709e-01,2.570422589778900146e-01,2.711267471313476562e-01,2.711267471313476562e-01,2.852112650871276855e-01,2.781690061092376709e-01,2.781690061092376709e-01,2.605633735656738281e-01,2.852112650871276855e-01,2.781690061092376709e-01,2.746478915214538574e-01,2.605633735656738281e-01,2.711267471313476562e-01,2.359154969453811646e-01,2.605633735656738281e-01,2.500000000000000000e-01,2.570422589778900146e-01,2.394366264343261719e-01,2.500000000000000000e-01,2.288732379674911499e-01,2.359154969453811646e-01,2.253521084785461426e-01,2.394366264343261719e-01,2.077464759349822998e-01,2.323943674564361572e-01,2.359154969453811646e-01,2.288732379674911499e-01,2.183098644018173218e-01,2.288732379674911499e-01,2.112676054239273071e-01,2.253521084785461426e-01,2.183098644018173218e-01,2.147887349128723145e-01,2.147887349128723145e-01,2.218309789896011353e-01,2.112676054239273071e-01,2.218309789896011353e-01,2.077464759349822998e-01,2.183098644018173218e-01,2.323943674564361572e-01,2.253521084785461426e-01,2.183098644018173218e-01,2.253521084785461426e-01,2.218309789896011353e-01,2.323943674564361572e-01,2.218309789896011353e-01,2.359154969453811646e-01,2.359154969453811646e-01,2.464788705110549927e-01,2.992957830429077148e-01,5.563380122184753418e-01,1.000000000000000000e+00,5.598591566085815430e-01,5.492957830429077148e-01,5.528169274330139160e-01,5.598591566085815430e-01,5.422534942626953125e-01,5.176056623458862305e-01,4.612676203250885010e-01,3.697183132171630859e-01,2.570422589778900146e-01,4.225352033972740173e-02,0.000000000000000000e+00,6.338027864694595337e-02,8.802817016839981079e-02,9.154929220676422119e-02,1.654929518699645996e-01,1.690140813589096069e-01,1.584507077932357788e-01,1.584507077932357788e-01,1.584507077932357788e-01,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,0.000000000000000000e+00,4.000000000000000000e+00";

#define MAX_DATA_SIZE (sizeof(input_data))

int main()
{
    uint32_t input_data_size = MAX_DATA_SIZE; // Total input data size in bytes

    // Compressed data buffer
    /*The size of the `compressed_data` buffer being set to `MAX_DATA_SIZE * 2` is based on an assumption made when using the LZF compression algorithm.
    LZF is a fast compression algorithm, but it has the property that in rare cases, the compressed data can actually be larger than the original data.
    The maximum expansion ratio for LZF is theoretically around 104%.
    This expansion occurs in some rare cases because the LZF algorithm focuses on compression speed rather than optimal compression ratio.

    To ensure that the `compressed_data` buffer can always accommodate the compressed data, we allocate a buffer that is twice the size of the input data.
    This should be sufficient to handle the rare cases when LZF may expand the data during compression.
    Using `MAX_DATA_SIZE * 2` provides a reasonable safety margin to handle most cases without requiring dynamic memory allocation.

    In practice, you may use a buffer of size `MAX_DATA_SIZE * 2` to avoid any issues when using LZF compression, even if your specific data may not result in a large expansion.
    This is a common approach when working with LZF and similar algorithms. It ensures that your code remains robust regardless of the input data characteristics.*/

    char compressed_data[MAX_DATA_SIZE * 2]; // We assume maximum expansion is 100%
    uint32_t compressed_data_size = sizeof(compressed_data);

    // Decompressed data buffer
    char decompressed_data[MAX_DATA_SIZE];
    uint32_t decompressed_data_size = sizeof(decompressed_data);

    // Compress the data
    uint32_t compressed_size = lzf_compress(input_data, input_data_size, compressed_data, compressed_data_size);

    if (compressed_size == 0)
    {
        printf("Compression failed or buffer is too small.\n");
        return 1;
    }

    // Decompress the data
    uint32_t decompressed_size = lzf_decompress(compressed_data, compressed_size, decompressed_data, decompressed_data_size);

    if (decompressed_size == 0)
    {
        printf("Decompression failed or buffer is too small.\n");
        return 1;
    }

    // First layer of check: Compare the original data with the decompressed data
    if (decompressed_size != input_data_size || memcmp(input_data, decompressed_data, input_data_size) != 0)
    {
        printf("Decompressed data does not match the original data.\n");
        return 1;
    }

    // Second layer of check: check each value in the decompressed data with the input data
    for (int i = 0; i < MAX_DATA_SIZE; i++)
    {
        if (decompressed_data[i] != input_data[i])
        {
            printf("Decompressed data does not match the original data.\n");
            return 1;
        }
    }

    // print the decompressed data
    // for (int i = 0; i < MAX_DATA_SIZE; i++)
    // {
    //     printf("%c", decompressed_data[i]);
    // }

    // print number of input data bytes
    printf("\n\nNumber of input data bytes: %d", input_data_size);
    // print number of bytes compressed
    printf("\n\nNumber of bytes compressed: %d\n", compressed_size);
    // print number of bytes decompressed
    printf("\nNumber of bytes decompressed: %d\n", decompressed_size);
    // print compression ratio
    printf("\nCompression ratio: %f\n", (float)input_data_size / (float)compressed_size);

    printf("\nCompression and decompression successful.\n");

    return 0;
}
