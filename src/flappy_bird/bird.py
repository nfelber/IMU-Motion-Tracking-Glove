import pygame
from pygame.locals import *
import random
from math import sqrt

pygame.init()

using_glove = True
glove_pipe_gesture = False
glove_stop_falling_gesture = False
glove_stop_event_gesture = False
glove_reset_gesture = False
movement_threshold = 10
glove_acceleration = 0

palm_finger_bent = False
thumb_finger_bent = False
index_finger_bent = False
middle_finger_bent = False
ring_finger_bent = False
little_finger_bent = False

glove_data = [palm_finger_bent,
thumb_finger_bent,
index_finger_bent,
middle_finger_bent,
ring_finger_bent,
little_finger_bent]

width = 1280
height = 1024
fps = 40
flying = False
pipe_gap_distance = 200
random_obstacle_height = 0
time_last_obstacle = pygame.time.get_ticks() - 1750
score = 0
floor_movement = 0
scrolling_px_per_second = 4
lost = False
glove_event = 0
image_number = 0
resetted = False

score_array = []

clock = pygame.time.Clock()

display = pygame.display.set_mode([width, height])
pygame.display.set_caption('IMU Glove Flappy Bird')

class FlappyBird(pygame.sprite.Sprite):
	def __init__(self, x, y):
		pygame.sprite.Sprite.__init__(self)
		self.sprite_group = []

		for image_number in range (1, 5):
			image = pygame.image.load(f"img/bird{image_number}.png")
			self.sprite_group.append(image)

		for image_number in range (1, 5):
			image_event = pygame.image.load(f"img/bird_event-{image_number}.png")
			self.sprite_group.append(image_event)

		self.image_number = 0
		self.image = self.sprite_group[self.image_number]
		self.rect = self.image.get_rect()
		self.currently_jumping = False
		self.rect.center = [x, y]
		self.falling_speed = 0
		self.image_time = pygame.time.get_ticks()
		
	def update(self):
		global glove_acceleration
		global image_number 
		global glove_event
		global glove_stop_falling_gesture
		global movement_threshold
		global lost
		global flying

		if self.rect.y >= 777:
			lost = True
			flying = False

		if not lost:
			if pygame.mouse.get_pressed()[0] and not self.currently_jumping and not glove_event:
				flying = True
				self.currently_jumping = True
				self.falling_speed = -10

			elif using_glove and not glove_event:
				if glove_acceleration > movement_threshold:
					self.currently_jumping = True
					self.falling_speed = -10

			if pygame.time.get_ticks() - self.image_time > 100:
				self.image_time = pygame.time.get_ticks()

				self.draw()

	def collides_with_pipes(self, flappy_sprites, pipes):
		return pygame.sprite.groupcollide(flappy_sprites, pipes, False, False)

	def draw(self):
		self.image_number += 1
		if self.image_number >= 4:
				self.image_number = 0
		if not glove_event:
			img = self.image_number
		else:
			img = self.image_number + 4

		self.image = pygame.transform.rotate(self.sprite_group[img], self.falling_speed * -2.5)

class Reset_Game(pygame.sprite.Sprite):
	def __init__(self, x, y):
		pygame.sprite.Sprite.__init__(self)
		self.image = pygame.image.load('img/restart.png')
		self.rect = self.image.get_rect()
		self.rect.center = (x, y)

	def reset(self):
		if self.rect.collidepoint(pygame.mouse.get_pos()):
			if pygame.mouse.get_pressed()[0]:
				self.kill()
				return True

		return False

class GlovePipeObstacle(pygame.sprite.Sprite):
	def __init__(self, x, y):
		pygame.sprite.Sprite.__init__(self)
		self.image = pygame.image.load("img/glove_obstacle.jpg")
		self.rect = self.image.get_rect()
		self.rect.bottomleft = [x, y]

	def update(self):
		self.rect.x -= scrolling_px_per_second

class PipeObstacle(pygame.sprite.Sprite):
	def __init__(self, x, y, from_top_or_bottom):
		global score_array
		pygame.sprite.Sprite.__init__(self)
		self.remove_me_counter = 340

		if from_top_or_bottom == "top":
			score_array.append(width - 40)
			self.image = pygame.transform.flip(pygame.image.load("img/obstacle.png"), False, True)
			self.rect = self.image.get_rect()
			self.rect.bottomleft = [x, y - pipe_gap_distance / 2]
		elif from_top_or_bottom == "bottom":
			self.image = pygame.image.load("img/obstacle.png")
			self.rect = self.image.get_rect()
			self.rect.topleft = [x, y + pipe_gap_distance / 2]

	def update(self):
		self.rect.x -= scrolling_px_per_second
		self.remove_me_counter -= 1
		if self.remove_me_counter == 0:
			self.kill()

def restart():
	global glove_event
	global score
	global lost
	global score_array
	global resetted

	resetted = True
	glove_event = False
	pipes.empty()
	glove_pipes.empty()
	flappy.rect.x = 100
	flappy.rect.y = height / 2 - 40			
	score = 0
	lost = False
	score_array = []
	pygame.transform.rotate(flappy.image, 0)

def receive_glove_data(input_data):
	global glove_data

	for index in range(5):
		glove_data[index] = input_data[index]

def receive_glove_acceleration(acceleration):
	global glove_acceleration

	x_acceleration_squared = acceleration[0] ** 2
	y_acceleration_squared = acceleration[1] ** 2
	z_acceleration_squared = acceleration[2] ** 2

	glove_acceleration = sqrt(x_acceleration_squared + y_acceleration_squared + z_acceleration_squared)

def gesture_check_falling_event(glove_data):
	global glove_stop_event_gesture

	if glove_data[0] and not glove_data[1] and glove_data[2] and glove_data[3] and not glove_data[4]:
		glove_stop_event_gesture = True
	else:
		glove_stop_event_gesture = False

def gesture_check_brick_wall(glove_data):
	global glove_pipe_gesture

	if glove_data[0] and glove_data[1] and glove_data[2] and glove_data[3] and glove_data[4]:
		glove_pipe_gesture = True
	else:
		glove_pipe_gesture = False

def gesture_check_stop_falling(glove_data):
	global glove_stop_falling_gesture

	if not glove_data[0] and not glove_data[1] and glove_data[2] and glove_data[3] and glove_data[4]:
		glove_stop_falling_gesture = True
	else:
		glove_stop_falling_gesture = False

def gesture_reset_game(glove_data):
	global glove_reset_gesture

	if not glove_data[0] and glove_data[1] and glove_data[2] and not glove_data[3] and not glove_data[4]:
		glove_reset_gesture = True
	else:
		glove_reset_gesture = False

pipes = pygame.sprite.Group()
glove_pipes = pygame.sprite.Group()
flappy_sprites = pygame.sprite.Group()

flappy = FlappyBird(100, height / 2)

flappy_sprites.add(flappy)

reset_option = Reset_Game(width / 2, height / 2)

game_enabled = True
while game_enabled:

	if glove_event:
		display.blit(pygame.image.load('img/background_event.jpg'), (0,0))
	else:
		display.blit(pygame.image.load('img/background.jpg'), (0,0))

	clock.tick(fps)

	pipes.draw(display)
	glove_pipes.draw(display)
	flappy_sprites.draw(display)
	flappy_sprites.update()
	random_obstacle_height = random.randint(-130, 130)

	ground = display.blit(pygame.image.load('img/floor.png'), (floor_movement, 824))

	if flappy.collides_with_pipes(flappy_sprites, pipes) or  flappy.collides_with_pipes(flappy_sprites, glove_pipes) or flappy.rect.top < 0:
		lost = True
		if flappy.falling_speed < 0:
			flappy.falling_speed = 2

	if lost:
		flappy.image = pygame.transform.rotate(pygame.image.load(f"img/bird_dead.png"), -11 * flappy.falling_speed)

	if using_glove:
		gesture_check_brick_wall(glove_data)
		gesture_check_falling_event(glove_data)
		gesture_check_stop_falling(glove_data)
		gesture_reset_game(glove_data)

	if not lost and len(score_array) > 0:
		for index in range(0, len(score_array)):
			score_array[index] -= scrolling_px_per_second

		if score_array[0] == 0:
			score += 1
			score_array.pop(0)

	if not lost and flying:
		floor_movement -= scrolling_px_per_second
		if abs(floor_movement) > 40:
			floor_movement = 0

		current_time = pygame.time.get_ticks()
		if current_time - time_last_obstacle > 1750:
			pipe_type = random.randint(1, 3)
			if pipe_type == 1:
				pipe = GlovePipeObstacle(width, height)
				glove_pipes.add(pipe)				
			else:
				bottom_pipe = PipeObstacle(width, height / 2 + random_obstacle_height, "bottom")
				top_pipe = PipeObstacle(width, height / 2 + random_obstacle_height, "top")
				pipes.add(bottom_pipe)
				pipes.add(top_pipe)
			time_last_obstacle = current_time

	for e in pygame.event.get():		
		if e.type == pygame.MOUSEBUTTONDOWN and not lost:
			flying = True
		if e.type == pygame.QUIT:
			game_enabled = False
			break

	keystate = pygame.key.get_pressed()
	if not lost and (keystate[pygame.K_SPACE] or glove_pipe_gesture):
		glove_pipes.empty()

	if not lost and (keystate[pygame.K_n] or glove_stop_event_gesture):
		glove_event = False

	if not pygame.mouse.get_pressed()[0]:
		flappy.currently_jumping = False

	if flying:
		flappy.falling_speed += 0.45

		if not lost and not glove_event and (keystate[pygame.K_h] or glove_stop_falling_gesture):
			flappy.falling_speed = 0

		if flappy.rect.y < 777:
			flappy.rect.y += int(flappy.falling_speed)

		if flappy.falling_speed > 10:
			flappy.falling_speed = 10

	random_event = random.randint(0, 200)
	if random_event == 0 and not lost and flying:
		glove_event = True

	display.blit(pygame.font.SysFont('arial', 40).render(f"Score: {score}", True, [255, 255, 255]), [566, 30])

	if not lost:
		pipes.update()
		glove_pipes.update()
	else:
		game_over_skull = pygame.image.load("img/game_over.png")
		display.blit(game_over_skull, (100, 150))
		display.blit(pygame.font.SysFont('arial', 120).render(f"GAME OVER", True, [random.randint(0, 160), random.randint(0, 10), random.randint(0, 10)]), [280, 140])
		display.blit(game_over_skull, (1040, 150))
		display.blit(pygame.font.SysFont('arial', 75).render(f"Press to restart", True, [0, 0, 0]), [390, 330])
		display.blit(reset_option.image, [reset_option.rect.x, reset_option.rect.y])
		
		if reset_option.reset() or glove_reset_gesture:
			restart()

	if not lost and not flying and not resetted:
		display.blit(pygame.font.SysFont('arial', 120).render(f"FLAP TO START", True, [random.randint(0, 10), random.randint(0, 10), random.randint(0, 10)]), [200, 450])

	pygame.display.update()

pygame.quit()
