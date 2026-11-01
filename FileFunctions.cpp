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
    #include "FileFunctions.hpp"
    
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


// ========================================================
//      LOADING COLOR PALETTE FILES
// ========================================================


static bool ReadPaletteTextLine( istream& InputFile, string& TextLine )
{
    if( !getline( InputFile, TextLine ) )
      return false;

    // also accept Windows line endings when running on other platforms
    if( !TextLine.empty() && TextLine.back() == '\r' )
      TextLine.pop_back();

    return true;
}

// -----------------------------------------------------------------------------

static void LoadJASCPaletteFile( const string& PaletteFilePath, RGBColorVector& PaletteColors )
{
    // open input palette file in text mode
    ifstream InputFile( PaletteFilePath );
    
    if( !InputFile.is_open() )
      throw runtime_error( "Could not open palette file: " + PaletteFilePath );
    
    string TextLine;
    
    // validate format signature in the first line
    if( !ReadPaletteTextLine( InputFile, TextLine ) || TextLine != "JASC-PAL" )
      throw runtime_error( "The palette file does not have a valid JASC-PAL format" );
    
    // read version number in second line, typically "0100"
    if( !ReadPaletteTextLine( InputFile, TextLine ) )
      throw runtime_error( "Unexpected end of file reading palette file version" );
    
    // read number of colors in the third line
    if( !ReadPaletteTextLine( InputFile, TextLine ) )
      throw runtime_error( "Unexpected end of file reading palette file color number" );
    
    stringstream CountStream( TextLine );
    int NumberOfColors = 0;
    
    if( !(CountStream >> NumberOfColors) || NumberOfColors < 1 || NumberOfColors > 256 )
      throw runtime_error( "Stated number of colors in palette file is not valid: " + TextLine );
    
    // prepare the output vector to store all colors in order
    PaletteColors.clear();
    PaletteColors.reserve( NumberOfColors );
    
    // read every color in order (an RGB triple per line)
    for( int i = 0; i < NumberOfColors; i++ )
    {
        if( !ReadPaletteTextLine( InputFile, TextLine ) )
          throw runtime_error( "Palette file contains less colors than it states. Contains: " + to_string( i ) + ", expected: " + to_string( NumberOfColors ) );
        
        // ignore empty lines within the data block
        if( TextLine.empty() )
        {
            --i;
            continue;
        }
        
        // read the 3 color components as integers
        stringstream ColorStream( TextLine );
        int r, g, b;
        
        if( !(ColorStream >> r >> g >> b) )
          throw runtime_error( "RGB format error in palette file, in line number " + to_string( i + 4 ) + ": " + TextLine );
        
        // check range for each of the colors
        if( r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255 )
          throw runtime_error( "Color components out of range (0-255) in palette file, in line number " + to_string( i + 4 ) );
        
        // build the output RGB color for this index
        RGBColor Color;
        Color.red   = static_cast< uint8_t >( r );
        Color.green = static_cast< uint8_t >( g );
        Color.blue  = static_cast< uint8_t >( b );
        
        PaletteColors.push_back( Color );
    }
}

// -----------------------------------------------------------------------------

static bool IsHEXColorLine( const string& TextLine )
{
    if( TextLine.size() != 6 )
      return false;

    for( char Character : TextLine )
    {
        if( !(Character >= '0' && Character <= '9') &&
            !(Character >= 'a' && Character <= 'f') &&
            !(Character >= 'A' && Character <= 'F') )
          return false;
    }

    return true;
}

// -----------------------------------------------------------------------------

static void LoadHEXPaletteFile( const string& FileData, RGBColorVector& PaletteColors )
{
    istringstream InputFile( FileData );
    string TextLine;
    int LineNumber = 0;

    // each nonempty line contains one color as six hexadecimal digits
    while( ReadPaletteTextLine( InputFile, TextLine ) )
    {
        LineNumber++;

        if( TextLine.empty() )
          continue;

        if( !IsHEXColorLine( TextLine ) )
          throw runtime_error( "HEX format error in palette file, in line number " + to_string( LineNumber ) );

        if( PaletteColors.size() == 256 )
          throw runtime_error( "Only palettes up to 256 colors are supported" );

        unsigned int ColorValue = 0;
        stringstream ColorStream( TextLine );
        ColorStream >> hex >> ColorValue;

        RGBColor Color;
        Color.red   = static_cast< uint8_t >( ColorValue >> 16 );
        Color.green = static_cast< uint8_t >( ColorValue >> 8 );
        Color.blue  = static_cast< uint8_t >( ColorValue );
        PaletteColors.push_back( Color );
    }
}

// -----------------------------------------------------------------------------

static void LoadACTPaletteFile( const string& FileData, RGBColorVector& PaletteColors )
{
    // ACT has 256 RGB triples, optionally followed by two big-endian words
    if( FileData.size() != 768 && FileData.size() != 772 )
      throw runtime_error( "The ACT palette file must contain 768 or 772 bytes" );

    size_t NumberOfColors = 256;

    if( FileData.size() == 772 )
    {
        NumberOfColors = static_cast< uint8_t >( FileData[ 768 ] ) * 256u
                       + static_cast< uint8_t >( FileData[ 769 ] );

        // a zero count denotes the full 256-color table
        if( NumberOfColors == 0 )
          NumberOfColors = 256;

        if( NumberOfColors > 256 )
          throw runtime_error( "Stated number of colors in ACT palette file is not valid" );

        // the last word is a transparency index; this program only uses RGB
    }

    PaletteColors.reserve( NumberOfColors );

    for( size_t i = 0; i < NumberOfColors; i++ )
    {
        RGBColor Color;
        Color.red   = static_cast< uint8_t >( FileData[ i * 3 + 0 ] );
        Color.green = static_cast< uint8_t >( FileData[ i * 3 + 1 ] );
        Color.blue  = static_cast< uint8_t >( FileData[ i * 3 + 2 ] );
        PaletteColors.push_back( Color );
    }
}

// -----------------------------------------------------------------------------

static uint32_t ReadPaletteLittleEndian( const string& FileData, size_t Offset, size_t NumberOfBytes )
{
    uint32_t Value = 0;

    // callers validate the field's bounds before reading it
    for( size_t i = 0; i < NumberOfBytes; i++ )
      Value |= static_cast< uint32_t >( static_cast< uint8_t >( FileData[ Offset + i ] ) ) << ( i * 8 );

    return Value;
}

// -----------------------------------------------------------------------------

static void LoadRIFFPaletteFile( const string& FileData, RGBColorVector& PaletteColors )
{
    // RIFF PAL contains a little-endian LOGPALETTE structure in its data chunk
    if( FileData.size() < 12 || FileData.compare( 0, 4, "RIFF" ) != 0 ||
        FileData.compare( 8, 4, "PAL " ) != 0 )
      throw runtime_error( "The palette file does not have a valid RIFF PAL format" );

    if( ReadPaletteLittleEndian( FileData, 4, 4 ) != FileData.size() - 8 )
      throw runtime_error( "The RIFF palette file size does not match its header" );

    size_t Offset = 12;
    bool FoundColors = false;

    while( Offset < FileData.size() )
    {
        if( FileData.size() - Offset < 8 )
          throw runtime_error( "Incomplete chunk header in RIFF palette file" );

        bool IsDataChunk = FileData.compare( Offset, 4, "data" ) == 0;
        size_t ChunkSize = ReadPaletteLittleEndian( FileData, Offset + 4, 4 );
        Offset += 8;

        if( ChunkSize > FileData.size() - Offset )
          throw runtime_error( "Incomplete chunk data in RIFF palette file" );

        if( IsDataChunk )
        {
            if( FoundColors || ChunkSize < 4 )
              throw runtime_error( "Invalid data chunk in RIFF palette file" );

            if( ReadPaletteLittleEndian( FileData, Offset, 2 ) != 0x0300 )
              throw runtime_error( "Unsupported version in RIFF palette file" );

            size_t NumberOfColors = ReadPaletteLittleEndian( FileData, Offset + 2, 2 );

            if( NumberOfColors < 1 || NumberOfColors > 256 || ChunkSize != 4 + NumberOfColors * 4 )
              throw runtime_error( "Invalid color count or data size in RIFF palette file" );

            PaletteColors.reserve( NumberOfColors );

            for( size_t i = 0; i < NumberOfColors; i++ )
            {
                // the fourth byte of each entry contains flags, not alpha
                RGBColor Color;
                Color.red   = static_cast< uint8_t >( FileData[ Offset + 4 + i * 4 + 0 ] );
                Color.green = static_cast< uint8_t >( FileData[ Offset + 4 + i * 4 + 1 ] );
                Color.blue  = static_cast< uint8_t >( FileData[ Offset + 4 + i * 4 + 2 ] );
                PaletteColors.push_back( Color );
            }

            FoundColors = true;
        }

        // skip unknown chunks and the padding byte following odd-sized chunks
        Offset += ChunkSize;

        if( ChunkSize % 2 != 0 )
        {
            if( Offset == FileData.size() )
              throw runtime_error( "Missing chunk padding in RIFF palette file" );

            Offset++;
        }
    }

    if( !FoundColors )
      throw runtime_error( "The RIFF palette file does not contain a data chunk" );
}

// -----------------------------------------------------------------------------

void LoadPaletteFile( const string& PaletteFilePath, RGBColorVector& PaletteColors )
{
    // read bytes without newline translation to detect the format by content
    ifstream InputFile( PaletteFilePath, ios::binary | ios::ate );

    if( !InputFile.is_open() )
      throw runtime_error( "Could not open palette file: " + PaletteFilePath );

    streamsize FileSize = InputFile.tellg();

    if( FileSize < 0 )
      throw runtime_error( "Could not determine palette file size: " + PaletteFilePath );

    string FileData( static_cast< size_t >( FileSize ), '\0' );
    InputFile.seekg( 0 );

    if( FileSize > 0 && !InputFile.read( &FileData[ 0 ], FileSize ) )
      throw runtime_error( "Could not read palette file: " + PaletteFilePath );

    istringstream TextStream( FileData );
    string FirstLine;

    // HEX files may contain blank lines before the first color
    while( ReadPaletteTextLine( TextStream, FirstLine ) && FirstLine.empty() ) {}

    // load into a temporary vector so failed loads leave the output unchanged
    RGBColorVector LoadedColors;

    if( FileData.compare( 0, 8, "JASC-PAL" ) == 0 )
      LoadJASCPaletteFile( PaletteFilePath, LoadedColors );
    else if( FileData.compare( 0, 4, "RIFF" ) == 0 )
      LoadRIFFPaletteFile( FileData, LoadedColors );
    else if( IsHEXColorLine( FirstLine ) )
      LoadHEXPaletteFile( FileData, LoadedColors );
    // ACT has no signature; its standard file sizes are the final fallback
    else if( FileSize == 768 || FileSize == 772 )
      LoadACTPaletteFile( FileData, LoadedColors );
    else
      throw runtime_error( "Unrecognized palette file format: " + PaletteFilePath );

    PaletteColors.swap( LoadedColors );
}
