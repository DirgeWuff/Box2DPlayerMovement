A simple, single file player movement prototype using Box2D and Raylib, written in C++. Includes use of foot a sensor to track whether the player is on the ground. This is a branch of the original, using a capsule shape for the player's collision body, rather than a box. This can be helpful in situations where tiled maps are used, as there's no edge for the player to get hung up on if they encounter imperfect terrain geometry. 

This branch includes modified debug functions to work with capsule shapes. Can draw the outine of b2_polygonShape and b2_capsuleShape to aid in debugging, drawing of the center of the a body, a live readout of player position, and a status displaying whether the player's foot sensor is in contact with an object.

Movement keys are W and A to move left and right, and space to jump.

![alt text](https://github.com/DirgeWuff/Box2DPlayerMovement/blob/CapsuleBody/Images/Screenshot%202025-06-27%20at%204.01.38%E2%80%AFAM.png "Box2DPlayerMovement (capsule body) in action")
