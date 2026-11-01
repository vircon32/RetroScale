// *********************************************************
    // include standard C/C++ headers
    #include <fstream>      // [ C++ STL ] File streams
    #include <sstream>      // [ C++ STL ] String streams
    #include <iostream>     // [ C++ STL ] I/O streams
    #include <stdexcept>    // [ C++ STL ] Standard exceptions
    #include <cstdio>       // [ ANSI C ] Standard I/O
    #include <cmath>        // [ ANSI C ] Mathematics
    
    // include external headers
    #include <png.h>        // [ libpng ] Main header
    
    // include project headers
    #include "PNGFileIO.hpp"
    
    // declare used namespaces
    using namespace std;
// *********************************************************


// ========================================================
//      LOADING AND SAVING PNG FILES
// ========================================================


void LoadPNGFile( const string& PNGFilePath, RGBColorMatrix& ImageColors )
{
    // open file in binary mode
    FILE* PNGFile = fopen( PNGFilePath.c_str(), "rb" );
    
    if( !PNGFile )
      throw runtime_error( "Could not open input PNG file: " + PNGFilePath );
    
    // check PNG signature in the first 8 bytes
    uint8_t PNGHeader[ 8 ];
    
    if( fread( PNGHeader, 1, 8, PNGFile ) != 8 || png_sig_cmp( PNGHeader, 0, 8 ) )
    {
        fclose( PNGFile );
        throw runtime_error( "The input PNG file is not valid" );
    }
    
    // create the needed libpng data structures
    png_structp PNGReadStruct = png_create_read_struct( PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr );
    
    if( !PNGReadStruct )
    {
        fclose( PNGFile );
        throw runtime_error( "Could not create the read data structure to load PNG file" );
    }
    
    png_infop PNGInfoStruct = png_create_info_struct( PNGReadStruct );
    
    if( !PNGInfoStruct )
    {
        png_destroy_read_struct( &PNGReadStruct, nullptr, nullptr );
        fclose( PNGFile );
        throw runtime_error( "Could not create the info data structure to load PNG file" );
    }
    
    // check for libpng internal errors
    if( setjmp( png_jmpbuf( PNGReadStruct ) ) )
    {
        png_destroy_read_struct( &PNGReadStruct, &PNGInfoStruct, nullptr );
        fclose( PNGFile );
        throw runtime_error( "There were libpng internal errors reading the input PNG file" );
    }
    
    // configure libpng I/O to start reading after the signature's initial 8 bytes
    png_init_io( PNGReadStruct, PNGFile );
    png_set_sig_bytes( PNGReadStruct, 8 );
    
    // read PNG file header and get its data
    png_read_info( PNGReadStruct, PNGInfoStruct );
    
    png_uint_32 Width   = png_get_image_width ( PNGReadStruct, PNGInfoStruct );
    png_uint_32 Height  = png_get_image_height( PNGReadStruct, PNGInfoStruct );
    png_byte ColorType  = png_get_color_type  ( PNGReadStruct, PNGInfoStruct );
    png_byte BitDepth   = png_get_bit_depth   ( PNGReadStruct, PNGInfoStruct );
    
    // for images with a palette we convert them to RGB
    if( ColorType == PNG_COLOR_TYPE_PALETTE )
      png_set_palette_to_rgb( PNGReadStruct );
    
    // for grayscale images with less than 8 bits, first convert them to 8 bits
    if( ColorType == PNG_COLOR_TYPE_GRAY && BitDepth < 8 )
      png_set_expand_gray_1_2_4_to_8( PNGReadStruct );
    
    // now, for 8-bit grayscale images convert them to RGB
    if( ColorType == PNG_COLOR_TYPE_GRAY || ColorType == PNG_COLOR_TYPE_GRAY_ALPHA )
      png_set_gray_to_rgb( PNGReadStruct );
    
    // if there is a transparency channel, expand it to alpha
    if( png_get_valid( PNGReadStruct, PNGInfoStruct, PNG_INFO_tRNS ) )
      png_set_tRNS_to_alpha( PNGReadStruct );
    
    // reduce 16-bit color channels to 8 bits per channel
    if( BitDepth == 16 )
    {
        // this seems to have changed in more recent libpng versions
        #if PNG_LIBPNG_VER >= 10400
          png_set_scale_16( PNGReadStruct );
        #else
          png_set_strip_16( PNGReadStruct );
        #endif
    }
    
    // discard alpha channel if present to provide only 24-bit RGB colors
    if( ColorType & PNG_COLOR_MASK_ALPHA )
      png_set_strip_alpha( PNGReadStruct );
    
    // apply these changes in the PNG read data structure
    png_read_update_info( PNGReadStruct, PNGInfoStruct );
    
    // verify that after the transformations we have 3 channels with 8 bits each
    size_t RowBytes = png_get_rowbytes( PNGReadStruct, PNGInfoStruct );
    
    if( RowBytes != Width * 3 )
    {
        png_destroy_read_struct( &PNGReadStruct, &PNGInfoStruct, nullptr );
        fclose( PNGFile );
        throw runtime_error( "Incorrect output format found when loading PNG file" );
    }
    
    // allocate memory for the row pointers libpng provides when reading
    vector< png_bytep > RowPointers( Height );
    vector< uint8_t > raw_image_data( RowBytes * Height );
    
    for( png_uint_32 i = 0; i < Height; ++i )
      RowPointers[ i ] = &raw_image_data[ i * RowBytes ];
    
    // read the actual image pixels into those pointers
    png_read_image( PNGReadStruct, RowPointers.data() );
    
    // clean up libpng resources and close the file
    png_read_end( PNGReadStruct, nullptr );
    png_destroy_read_struct( &PNGReadStruct, &PNGInfoStruct, nullptr );
    fclose( PNGFile );
    
    // copy the image data into the output RGB color matrix
    ImageColors.resize( Height );
    
    for( png_uint_32 y = 0; y < Height; ++y )
    {
        ImageColors[ y ].resize( Width );
        png_bytep pixel_row = RowPointers[ y ];
        
        for( png_uint_32 x = 0; x < Width; ++x )
        {
            ImageColors[ y ][ x ].red   = pixel_row[ x * 3 + 0 ];
            ImageColors[ y ][ x ].green = pixel_row[ x * 3 + 1 ];
            ImageColors[ y ][ x ].blue  = pixel_row[ x * 3 + 2 ];
        }
    }
}

// -----------------------------------------------------------------------------

void SaveIndexedPNGFile
(
    const string& PNGFilePath,
    const RGBColorVector& Palette,
    const PaletteIndexMatrix& ImageColorIndices
)
{
    // check and read image dimensions
    if( ImageColorIndices.empty() || ImageColorIndices[ 0 ].empty() )
      throw runtime_error( "Cannot save PNG file from an empty color index matrix" );
    
    size_t Height = ImageColorIndices.size();
    size_t Width = ImageColorIndices[ 0 ].size();
    
    // check number of colors in the palette
    if( Palette.empty() )
      throw runtime_error( "Cannot save palettized PNG file from an empty color palette" );
    
    if( static_cast< int >( Palette.size() ) > 256 )
      throw runtime_error( "Color palettes with more than 256 colors are not supported" );
    
    // open destination file in binary mode
    FILE* PNGFile = fopen( PNGFilePath.c_str(), "wb" );
    
    if( !PNGFile )
      throw runtime_error( "Could not open output PNG file: " + PNGFilePath );
    
    // create the needed libpng data structures
    png_structp PNGWriteStruct = png_create_write_struct( PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr );
    
    if( !PNGWriteStruct )
    {
        fclose( PNGFile );
        throw runtime_error( "Could not create the write data structure to save PNG file" );
    }
    
    png_infop PNGInfoStruct = png_create_info_struct( PNGWriteStruct );
    
    if( !PNGInfoStruct )
    {
        png_destroy_write_struct( &PNGWriteStruct, nullptr );
        fclose( PNGFile );
        throw runtime_error( "Could not create the info data structure to save PNG file" );
    }
    
    // check for libpng internal errors
    if( setjmp( png_jmpbuf( PNGWriteStruct ) ) )
    {
        png_destroy_write_struct( &PNGWriteStruct, &PNGInfoStruct );
        fclose( PNGFile );
        throw runtime_error( "There were libpng internal errors writing the output PNG file" );
    }
    
    // configure libpng I/O to start writing
    png_init_io( PNGWriteStruct, PNGFile );
    
    // configure basic image properties (indexed color with 8 bits per pixel)
    png_set_IHDR
    (
        PNGWriteStruct,
        PNGInfoStruct,
        static_cast< png_uint_32 >( Width ),
        static_cast< png_uint_32 >( Height ),
        8, // bits per pixel
        PNG_COLOR_TYPE_PALETTE,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
    );
    
    // convert our palette colors to libpng's native format
    vector< png_color > PNGPalette( Palette.size() );
    
    for( size_t i = 0; i < Palette.size(); i++ )
    {
        PNGPalette[ i ].red   = Palette[ i ].red;
        PNGPalette[ i ].green = Palette[ i ].green;
        PNGPalette[ i ].blue  = Palette[ i ].blue;
    }
    
    // map the color palette to PNG metadata
    png_set_PLTE
    (
        PNGWriteStruct,
        PNGInfoStruct,
        PNGPalette.data(),
        static_cast< int >( PNGPalette.size() )
    );
    
    // write PNG file header
    png_write_info( PNGWriteStruct, PNGInfoStruct );
    
    // allocate memory for the row pointers libpng uses when writing
    vector< png_bytep > RowPointers( Height );
    
    for( size_t y = 0; y < Height; y++ )
      RowPointers[ y ] = const_cast< png_bytep >( ImageColorIndices[ y ].data() );
    
    // write the actual image color indices from those pointers
    png_write_image( PNGWriteStruct, RowPointers.data() );
    
    // finish file creation
    png_write_end( PNGWriteStruct, nullptr );
    
    // clean up libpng resources and close the file
    png_destroy_write_struct( &PNGWriteStruct, &PNGInfoStruct );
    fclose( PNGFile );
}
