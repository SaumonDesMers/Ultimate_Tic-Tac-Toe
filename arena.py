import subprocess
import select
import time

#   0 1 2   3 4 5   6 7 8
# 0 . . . | . . . | . . .
# 1 . 0 . | . 1 . | . 2 .
# 2 . . . | . . . | . . .
#   ------|-------|------
# 3 . . . | . . . | . . .
# 4 . 3 . | . 4 . | . 5 .
# 5 . . . | . . . | . . .
#   ------|-------|------
# 6 . . . | . . . | . . .
# 7 . 6 . | . 7 . | . 8 .
# 8 . . . | . . . | . . .

class Game:
	def __init__(self):
		# Initialize the game state
		self.board = [[0] * 9 for _ in range(9)]
		self.big_board = [0] * 9
		self.current_player = 1
		self.last_move = (-1, -1)
		self.possible_moves = []
		# Initialize the possible moves
		for i in range(9):
			for j in range(9):
				if self.big_board[i] == 0:
					self.possible_moves.append((i, j))

	def update(self, move_str: str):
		# Update the game state based on the move
		move = move_str.split()

		if len(move) != 2:
			print(f"Invalid move format: {move}")
			return False

		pos_y, pos_x = int(move[0]), int(move[1])
		small_board_index = (pos_y // 3) * 3 + (pos_x // 3)
		cell_index = (pos_y % 3) * 3 + (pos_x % 3)

		if pos_y < 0 or pos_y >= 9 or pos_x < 0 or pos_x >= 9:
			raise ValueError(f"Invalid position out of bounds: ({pos_y}, {pos_x})")

		if self.board[small_board_index][cell_index] != 0:
			print(f"Invalid move: Cell already occupied at ({pos_y}, {pos_x})")
			return False
		
		if (pos_y, pos_x) not in self.possible_moves:
			print(f"Invalid move: Cell not allowed at ({pos_y}, {pos_x})")
			print(f"Possible moves: {self.possible_moves}")
			return False

		self.board[small_board_index][cell_index] = self.current_player
		self.last_move = (pos_y, pos_x)

		if self.is_board_over(self.board[small_board_index]):
			self.big_board[small_board_index] = self.current_player
			self.board[small_board_index] = [self.current_player] * 9

		self.current_player = 3 - self.current_player

		# set the possible moves for the next player
		self.possible_moves = []
		if self.big_board[cell_index] == 0:
			for i in range(9):
				if self.board[cell_index][i] == 0:
					self.possible_moves.append(((cell_index // 3) * 3 + (i // 3), (cell_index % 3) * 3 + (i % 3)))
		else:
			for i in range(9):
				for j in range(9):
					if self.board[i][j] == 0:
						self.possible_moves.append(((i // 3) * 3 + (j // 3), (i % 3) * 3 + (j % 3)))

		return True

	def is_board_over(self, board):
		# Check if the board is finished
		if board[0] == board[1] == board[2] != 0:
			return True
		if board[3] == board[4] == board[5] != 0:
			return True
		if board[6] == board[7] == board[8] != 0:
			return True
		if board[0] == board[3] == board[6] != 0:
			return True
		if board[1] == board[4] == board[7] != 0:
			return True
		if board[2] == board[5] == board[8] != 0:
			return True
		if board[0] == board[4] == board[8] != 0:
			return True
		if board[2] == board[4] == board[6] != 0:
			return True
		return False
	
	def is_game_over(self):
		# Check if the game is over
		if self.is_board_over(self.big_board):
			return True
		if len(self.possible_moves) == 0:
			return True
		return False
	
	def winner(self):
		# Check if the game is over
		if self.is_game_over():
			if self.is_board_over(self.big_board):
				return 3 - self.current_player
			elif self.big_board.count(1) > self.big_board.count(2):
				return 1
			elif self.big_board.count(2) > self.big_board.count(1):
				return 2
			else:
				return 0
		else:
			return -1

	def string_to_send(self):
		string = str(self.last_move[0]) + " " + str(self.last_move[1]) + "\n"
		string += str(len(self.possible_moves)) + "\n"
		for move in self.possible_moves:
			string += str(move[0]) + " " + str(move[1]) + "\n"
		return string

	def print_board(self):
		# Print the current game state
		string_board = str(self.board[0][0]) + " " + str(self.board[0][1]) + " " + str(self.board[0][2]) + " | " + str(self.board[1][0]) + " " + str(self.board[1][1]) + " " + str(self.board[1][2]) + " | " + str(self.board[2][0]) + " " + str(self.board[2][1]) + " " + str(self.board[2][2]) + "\n"
		string_board += str(self.board[0][3]) + " " + str(self.board[0][4]) + " " + str(self.board[0][5]) + " | " + str(self.board[1][3]) + " " + str(self.board[1][4]) + " " + str(self.board[1][5]) + " | " + str(self.board[2][3]) + " " + str(self.board[2][4]) + " " + str(self.board[2][5]) + "\n"
		string_board += str(self.board[0][6]) + " " + str(self.board[0][7]) + " " + str(self.board[0][8]) + " | " + str(self.board[1][6]) + " " + str(self.board[1][7]) + " " + str(self.board[1][8]) + " | " + str(self.board[2][6]) + " " + str(self.board[2][7]) + " " + str(self.board[2][8]) + "\n"
		string_board += "------|-------|------\n"
		string_board += str(self.board[3][0]) + " " + str(self.board[3][1]) + " " + str(self.board[3][2]) + " | " + str(self.board[4][0]) + " " + str(self.board[4][1]) + " " + str(self.board[4][2]) + " | " + str(self.board[5][0]) + " " + str(self.board[5][1]) + " " + str(self.board[5][2]) + "\n"
		string_board += str(self.board[3][3]) + " " + str(self.board[3][4]) + " " + str(self.board[3][5]) + " | " + str(self.board[4][3]) + " " + str(self.board[4][4]) + " " + str(self.board[4][5]) + " | " + str(self.board[5][3]) + " " + str(self.board[5][4]) + " " + str(self.board[5][5]) + "\n"
		string_board += str(self.board[3][6]) + " " + str(self.board[3][7]) + " " + str(self.board[3][8]) + " | " + str(self.board[4][6]) + " " + str(self.board[4][7]) + " " + str(self.board[4][8]) + " | " + str(self.board[5][6]) + " " + str(self.board[5][7]) + " " + str(self.board[5][8]) + "\n"
		string_board += "------|-------|------\n"
		string_board += str(self.board[6][0]) + " " + str(self.board[6][1]) + " " + str(self.board[6][2]) + " | " + str(self.board[7][0]) + " " + str(self.board[7][1]) + " " + str(self.board[7][2]) + " | " + str(self.board[8][0]) + " " + str(self.board[8][1]) + " " + str(self.board[8][2]) + "\n"
		string_board += str(self.board[6][3]) + " " + str(self.board[6][4]) + " " + str(self.board[6][5]) + " | " + str(self.board[7][3]) + " " + str(self.board[7][4]) + " " + str(self.board[7][5]) + " | " + str(self.board[8][3]) + " " + str(self.board[8][4]) + " " + str(self.board[8][5]) + "\n"
		string_board += str(self.board[6][6]) + " " + str(self.board[6][7]) + " " + str(self.board[6][8]) + " | " + str(self.board[7][6]) + " " + str(self.board[7][7]) + " " + str(self.board[7][8]) + " | " + str(self.board[8][6]) + " " + str(self.board[8][7]) + " " + str(self.board[8][8]) + "\n"
		string_board = string_board.replace("0", ".")
		string_board = string_board.replace("1", "X")
		string_board = string_board.replace("2", "O")
		print(string_board)


def game_turn(bot, game, timeout):
	# Send the current game state to the current bot
	# print(f"Sending game state to bot: {bot.pid}")
	bot.stdin.write(game.string_to_send())
	bot.stdin.flush()

	# Use select to wait for input with a timeout
	start_time = time.time()
	# print(f"Waiting for bot to respond (timeout: {timeout}s)")
	ready, _, _ = select.select([bot.stdout], [], [], timeout)
	elapsed_time = time.time() - start_time

	if ready:
		# Read the bot's move
		move = bot.stdout.readline().strip()
		# print(f"Move from bot: '{move}'")

		# Update the game state based on the move
		if game.update(move) == False:
			return False

		# Check if the game is over
		if game.is_game_over():
			# print(f"Game finished! Player {game.winner()} wins!")
			return False
	else:
		print(f"Timeout: bot did not respond in time ({elapsed_time:.2f}s)")
		return False

	return True

def run_game(bot1_args, bot2_args):
	bot1 = subprocess.Popen(bot1_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
	bot2 = subprocess.Popen(bot2_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

	current_bot = bot1
	other_bot = bot2

	game = Game()

	game_turn(current_bot, game, 1)
	game_turn(other_bot, game, 1)

	while True:
		if game_turn(current_bot, game, 0.1) == False:
			break

		current_bot, other_bot = other_bot, current_bot
	
	bot1.terminate()
	bot2.terminate()

	return game.winner()


def main():
	bot1_args = ["./a.out"]
	bot2_args = ["./a.out"]

	bot1_win_count = 0
	bot2_win_count = 0
	draw_count = 0

	simulate_count = 50
	print(f"Simulating {simulate_count} games (~ {simulate_count * 7 / 60} minutes)...")

	for i in range(50):
		print(f"Game {i}...")
		winner = run_game(bot1_args, bot2_args)
		if winner == 0:
			draw_count += 1
		elif winner == 1:
			bot1_win_count += 1
		elif winner == 2:
			bot2_win_count += 1	
	
	print(f"Bot 1 wins: {bot1_win_count}")
	print(f"Bot 2 wins: {bot2_win_count}")
	print(f"Draws: {draw_count}")

if __name__ == '__main__':
	main()
