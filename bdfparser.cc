// bdfparser.cc
//
// Generates C++ code for SSD1306 OLED screen from 8x16 monospace BDF font file
//

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

using std::cout;
using std::dec;
using std::endl;
using std::hex;
using std::ifstream;
using std::setfill;
using std::setw;
using std::string;

constexpr const char*   FILE_NAME = "spleen-8x16.bdf";

/**************************************************************************************************************/
int
main()
{
    ifstream    input( FILE_NAME );

    if ( !input )
    {
        return EXIT_FAILURE;
    }

    string  line;

    cout <<
        "// font.cc\n"
        "//\n"
        "// The font based on spleen-8x16.bdf from project https://github.com/fcambus/spleen/\n"
        "//\n"
        "\n"
        "#include <cstdint>"
        "\n"
        "\n"
        "extern const uint8_t    FONT[][ 16 ] =\n"
        "{\n";


    while ( input.good() )
    {
        getline( input, line );

        if ( line.find("ENCODING ") != 0 )
        {
            continue;
        }

        line.erase( 0, 9 );

        int     encoding = stoi( line );

        if ( encoding > 127 )
        {
            continue;
        }

        getline( input, line );
        getline( input, line );
        getline( input, line );
        getline( input, line );

        int     bytes[ 16 ];

        memset( bytes, 0, sizeof(bytes) );

        for ( int i = 0; i < 16; ++i )
        {
            getline( input, line );

            int     byte = stoi( line, nullptr, 16 );
            int     shift = i & 0x07;

            if ( i < 8 )
            {
                bytes[ 7 ] |= ( byte & 1 ) << shift;
                bytes[ 6 ] |= ( (byte >> 1) & 1 ) << shift;
                bytes[ 5 ] |= ( (byte >> 2) & 1 ) << shift;
                bytes[ 4 ] |= ( (byte >> 3) & 1 ) << shift;
                bytes[ 3 ] |= ( (byte >> 4) & 1 ) << shift;
                bytes[ 2 ] |= ( (byte >> 5) & 1 ) << shift;
                bytes[ 1 ] |= ( (byte >> 6) & 1 ) << shift;
                bytes[ 0 ] |= ( (byte >> 7) & 1 ) << shift;
            }
            else
            {
                bytes[ 15 ] |= ( byte & 1 ) << shift;
                bytes[ 14 ] |= ( (byte >> 1) & 1 ) << shift;
                bytes[ 13 ] |= ( (byte >> 2) & 1 ) << shift;
                bytes[ 12 ] |= ( (byte >> 3) & 1 ) << shift;
                bytes[ 11 ] |= ( (byte >> 4) & 1 ) << shift;
                bytes[ 10 ] |= ( (byte >> 5) & 1 ) << shift;
                bytes[ 9 ] |= ( (byte >> 6) & 1 ) << shift;
                bytes[ 8 ] |= ( (byte >> 7) & 1 ) << shift;
            }
        }

        cout << "    { " << hex;

        for ( int i = 0; i < 15; ++i )
        {
            cout << "0x" << setw(2) << setfill('0') << bytes[ i ] << ", ";
        }

        cout << "0x" << setw(2) << setfill('0') << bytes[ 15 ] << dec << " }";
        
        if ( encoding != 127 )
        {
            cout << ",\n";
        }
    }

    cout << "\n};" << endl;
    return EXIT_SUCCESS;
}
