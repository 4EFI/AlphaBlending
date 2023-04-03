
#include <SFML/Graphics.hpp>
#include <immintrin.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SSE

//-----------------------------------------------------------------------------

const int Window_Width  = 1024;
const int Window_Height = 1024;

//-----------------------------------------------------------------------------

int main()
{
    sf::RenderWindow window( sf::VideoMode( Window_Width, Window_Height ), "C-a-a-a-t" );

    sf::Image img;
    img.create( Window_Width, Window_Height, sf::Color::White );

    sf::Texture texture;
    texture.loadFromImage( img );

    sf::Sprite sprite;
    sprite.setTexture( texture );

    sf::Clock clock;

    while( window.isOpen() )
    {
        sf::Time elapsed = clock.restart();

        sf::Event event;
        while( window.pollEvent(event) )
        {
            if( event.type == sf::Event::Closed ) window.close();
        }

        // texture.update( img );

        printf ( "FPS %.3f\n", 1 / elapsed.asSeconds() );

        window.clear( sf::Color::Red );
        // window.draw ( sprite );
        window.display();
    }

    return 0;
}

//-----------------------------------------------------------------------------
