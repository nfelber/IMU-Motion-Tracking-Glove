# FlappyBird
This directory is contains the code and image files allowing to play the Flappy Bird game.

## Launching the Flappy Bird game
To launch the game, follow these steps:

1. Install necessary pygame module by using the command ```pip install pygame```
2. Launch the game using the command ```python3 bird.py```

## Description of our implementation of the Flappy Bird game
Our game is based on the original Flappy Bird game developped by Dong Nguyen in 2013. The gameplay has several particularities that distinguish it from the initial gameplay:
1. There is a chance of 1/3 that instead of a pipe, the player will encounter a wall. The wall cannot be avoided and must thus be "destroyed" by the player making relevant interraction. 
2. In a random manner, the bird enters once every while into a state of madness because of flying without any break. During the state of madness, player loses his control over the bird. The bird will be no longer be willing to react to the player's command to flap his wings. To regain control over the bird, the player has to make relevant interraction.
3. Bird can float in the space at the same level by the player making relevant interraction. While the bird is floating, it will no longer be falling on the ground. However, when the bird enters into the madness state, it is too tired and cannot float anymore.

The goal of the game is to gain the highest score possible. The score is incremented every time that the bird flies between two pipes. The game is over when the bird falls on the ground by either hitting a pipe, a wall or the sky.

## Gameplay
Player can interract with the game's environment in the following manners:
1. By moving the glove fast enough or clicking on the mouse, the flappy bird will flap his wings and jump higher in space.
2. By making gesture 1 or pressing space, any wall that the player encounters gets destroyed.
2. By making gesture 2 or pressing n during the state of madness, the bird becomes normal once again and the player regains control over it.
3. By making gesture 3 or holding h, the bird floats at the same level in space.
4. By making gesture 4 or pressing the button on the screen, player can reset the game.

## Source of used images:
The images that our game uses come from the following sources:
- Flappy Bird texture 1: https://openclipart.org/detail/239673/flying-bird-3-frame-1
- Flappy Bird texture 2: https://openclipart.org/detail/239674/flying-bird-3-frame-2
- Flappy Bird texture 3: https://openclipart.org/detail/239675/flying-bird-3-frame-3
- Flappy Bird texture 4: https://openclipart.org/detail/239676/flying-bird-3-frame-4
- Background: http://clipart-library.com/clipart/761360.htm
- Ground: https://www.pikpng.com/pngvi/hmimmh_ground-flappy-bird-ground-scratch-clipart/
- Pipe: https://www.nicepng.com/ourpic/u2q8w7e6e6y3o0q8_flappy-bird-pipes-png-bottle/
- Brick wall obstacle: https://www.pngwing.com/en/free-png-zbvly
- Game over skull texture: https://openclipart.org/detail/248773/skull-and-cross-bones

All images that are not listed here come are images that were created by us by modifying aforementionned images.
