
#include <SFML/Graphics.hpp>
#include <immintrin.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define SSE

//-----------------------------------------------------------------------------

const int Window_Width  = 800;
const int Window_Height = 600;

//-----------------------------------------------------------------------------

inline void AlphaBlend( sf::Image* out_img, 
                        sf::Image* back_img, 
                        sf::Image* front_img, sf::Vector2u front_pos );

//-----------------------------------------------------------------------------

int main()
{
    sf::RenderWindow window( sf::VideoMode( Window_Width, Window_Height ), "C-a-a-a-t" );

    sf::Image cat_img;
    cat_img.loadFromFile( "img/cat.bmp" );

    sf::Image table_img;
    table_img.loadFromFile( "img/table.bmp" );

    sf::Image out_img;
    out_img.create( table_img.getSize().x, table_img.getSize().y, sf::Color::Green ); 

    sf::Texture out_texture;
    out_texture.loadFromImage( out_img );

    sf::Sprite out_sprite;
    out_sprite.setTexture( out_texture );

    sf::Clock clock;

    while( window.isOpen() )
    {
        sf::Time elapsed = clock.restart();

        sf::Event event;
        while( window.pollEvent(event) )
        {
            if( event.type == sf::Event::Closed ) window.close();
        }

        AlphaBlend( &out_img, &table_img, &cat_img, {100, 100} );
        out_texture.update( out_img );
        
        printf ( "FPS %.3f\n", 1 / elapsed.asSeconds() );

        window.clear( sf::Color::Red );
        window.draw ( out_sprite );
        window.display();
    }

    return 0;
}

//-----------------------------------------------------------------------------

inline void AlphaBlend( sf::Image* out_img, 
                        sf::Image* back_img, 
                        sf::Image* front_img, sf::Vector2u front_pos )
{
    assert( out_img   != NULL );
    assert( back_img  != NULL );
    assert( front_img != NULL );

    sf::Vector2u front_img_size = front_img->getSize();

    for( unsigned int cur_y = 0; cur_y < front_img_size.y; cur_y++ )
    {
        for( unsigned int cur_x = 0; cur_x < front_img_size.x; cur_x++ )
        {
            sf::Color back_clr  = back_img ->getPixel( cur_x + front_pos.x, 
                                                       cur_y + front_pos.y );
            sf::Color front_clr = front_img->getPixel( cur_x,   
                                                       cur_y );   

            sf::Color new_clr( sf::Uint8( ( front_clr.r * front_clr.a + back_clr.r * ( 255 - front_clr.a ) ) >> 8 ),
                               sf::Uint8( ( front_clr.g * front_clr.a + back_clr.g * ( 255 - front_clr.a ) ) >> 8 ),
                               sf::Uint8( ( front_clr.b * front_clr.a + back_clr.b * ( 255 - front_clr.a ) ) >> 8 ),
                               sf::Uint8( ( front_clr.a * 255         + back_clr.a * ( 255 - front_clr.a ) ) >> 8 ) );

            out_img->setPixel( cur_x + front_pos.x, 
                               cur_y + front_pos.y, 
                               new_clr );   
        }
    }
}

//-----------------------------------------------------------------------------