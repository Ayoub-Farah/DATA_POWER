import pygame

def victory():
    # Initialize Pygame
    pygame.init()

    # Load the MP3 file
    pygame.mixer.music.load("Libraries/victory.mp3")

    # Play the MP3 file
    pygame.mixer.music.play()

    # Wait for the music to finish playing
    pygame.time.wait(4200)

    # Clean up
    pygame.mixer.quit()
    pygame.quit()
