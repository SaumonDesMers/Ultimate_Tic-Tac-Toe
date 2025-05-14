#ifndef DEBUG
	#undef _GLIBCXX_DEBUG
	#pragma GCC optimize("O2,unroll-loops,omit-frame-pointer,inline")
	// #pragma GCC option("arch=native", "tune=native", "no-zero-upper")
#endif
#pragma GCC target("movbe,aes,pclmul,avx,avx2,f16c,fma,sse3,ssse3,sse4.1,sse4.2,rdrnd,popcnt,bmi,bmi2,lzcnt")

#include <iostream>
#include <string>
#include <cmath>
#include <random>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <cstring>
#include <immintrin.h>

#define int128(x) static_cast<__int128_t>(x)
#define FULL_ONE_MASK ~(int128(0x7fffffffffff) << 81)
#define random(min, max) min + rand() % (max - min)
#define action_index(am) int(ffs128(am) - 1)
#define action_mask(ai) (int128(1) << (ai))

#define get_small_board(board, i) ((board >> ((i) * 9)) & 0x1ff)
#define get_small_board_mask(i) (int128(0x1ff) << ((i) * 9))

using namespace std;

typedef __int128_t Mask128;
typedef __int16_t Mask16;

/*
	Smalls boards index in Mask16:
	 0 | 1 | 2
	---|---|---
	 3 | 4 | 5
	---|---|---
	 6 | 7 | 8

	Cells index in Mask128:
	 0  1  2 |  9 10 11 | 18 19 20
	 3  4  5 | 12 13 14 | 21 22 23
	 6  7  8 | 15 16 17 | 24 25 26
	---------|----------|---------
	27 28 29 | 36 37 38 | 45 46 47
	30 31 32 | 39 40 41 | 48 49 50
	33 34 35 | 42 43 44 | 51 52 53
	---------|----------|---------
	54 55 56 | 63 64 65 | 72 73 74
	57 58 59 | 66 67 68 | 75 76 77
	60 61 62 | 69 70 71 | 78 79 80

	Small game distribution in Mask128:
													 board 8   board 7   board 6   board 5   board 4   board 3   board 2   board 1   board 0
	00000000000000000000000000000000000000000000000 000000000 000000000 000000000 000000000 000000000 000000000 000000000 000000000 000000000

	Mask to test if board is finished:
	A B C
	D E F --> I H G F E D C B A
	G H I
*/

constexpr Mask16 test_ABC = 0b000000111;
constexpr Mask16 test_DEF = 0b000111000;
constexpr Mask16 test_GHI = 0b111000000;
constexpr Mask16 test_ADG = 0b001001001;
constexpr Mask16 test_BEH = 0b010010010;
constexpr Mask16 test_CFI = 0b100100100;
constexpr Mask16 test_AEI = 0b100010001;
constexpr Mask16 test_CEG = 0b001010100;

constexpr Mask128 test_ABC_big = int128(0b111111111111111111111111111);
constexpr Mask128 test_DEF_big = int128(0b111111111111111111111111111) << 27;
constexpr Mask128 test_GHI_big = int128(0b111111111111111111111111111) << 54;
constexpr Mask128 test_ADG_big = int128(0b111111111000000000000000000111111111000000000000000000111111111);
constexpr Mask128 test_BEH_big = int128(0b111111111000000000000000000111111111000000000000000000111111111) << 9;
constexpr Mask128 test_CFI_big = int128(0b111111111000000000000000000111111111000000000000000000111111111) << 18;
constexpr Mask128 test_AEI_big = int128(0b111111111000000000000000000000000000111111111) | (int128(0b111111111) << 72);
constexpr Mask128 test_CEG_big = int128(0b000000000000000000111111111000000000111111111000000000111111111000000000000000000);


// uint64_t shuffle_table[2] = {static_cast<uint64_t>(time(NULL)), static_cast<uint64_t>(time(NULL)) >> 16};
// uint64_t random(int min, int max)
// {
// 	uint64_t s1 = shuffle_table[0];
// 	uint64_t s0 = shuffle_table[1];
// 	uint64_t result = s0 + s1;
// 	shuffle_table[0] = s0;
// 	s1 ^= s1 << 23;
// 	shuffle_table[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
// 	return min + result % (max - min);
// }

struct Timer
{
	clock_t time_point;

	Timer() { set(); }

	void set() { time_point = clock(); }

	double diff(bool reset = true)
	{
		clock_t next_time_point = clock();
		clock_t tick_diff = next_time_point - time_point;
		double diff = (double)(tick_diff) / CLOCKS_PER_SEC * 1000;
		if (reset)
			time_point = next_time_point;
		return diff;
	}
} timer;

const Mask128 full_one_mask = ~(int128(0x7fffffffffff) << 81);

const int game_index_to_str_index[81] = {
	0,2,4,22,24,26,44,46,48,
	8,10,12,30,32,34,52,54,56,
	16,18,20,38,40,42,60,62,64,
	88,90,92,110,112,114,132,134,136,
	96,98,100,118,120,122,140,142,144,
	104,106,108,126,128,130,148,150,152,
	176,178,180,198,200,202,220,222,224,
	184,186,188,206,208,210,228,230,232,
	192,194,196,214,216,218,236,238,240
};

const char index_to_pos[81][4] = {
	"0 0","0 1","0 2","1 0","1 1","1 2","2 0","2 1","2 2",
	"0 3","0 4","0 5","1 3","1 4","1 5","2 3","2 4","2 5",
	"0 6","0 7","0 8","1 6","1 7","1 8","2 6","2 7","2 8",
	"3 0","3 1","3 2","4 0","4 1","4 2","5 0","5 1","5 2",
	"3 3","3 4","3 5","4 3","4 4","4 5","5 3","5 4","5 5",
	"3 6","3 7","3 8","4 6","4 7","4 8","5 6","5 7","5 8",
	"6 0","6 1","6 2","7 0","7 1","7 2","8 0","8 1","8 2",
	"6 3","6 4","6 5","7 3","7 4","7 5","8 3","8 4","8 5",
	"6 6","6 7","6 8","7 6","7 7","7 8","8 6","8 7","8 8",
};

const int pos_to_index[9][9] = {
	{ 0,  1,  2,  9, 10, 11, 18, 19, 20},
	{ 3,  4,  5, 12, 13, 14, 21, 22, 23},
	{ 6,  7,  8, 15, 16, 17, 24, 25, 26},
	{27, 28, 29, 36, 37, 38, 45, 46, 47},
	{30, 31, 32, 39, 40, 41, 48, 49, 50},
	{33, 34, 35, 42, 43, 44, 51, 52, 53},
	{54, 55, 56, 63, 64, 65, 72, 73, 74},
	{57, 58, 59, 66, 67, 68, 75, 76, 77},
	{60, 61, 62, 69, 70, 71, 78, 79, 80},
};

uint_fast8_t ffs128(__int128_t n)
{
	const uint_fast8_t cnt_hi = __builtin_ffsll(n >> 64);
	const uint_fast8_t cnt_lo = __builtin_ffsll(n);
	return cnt_lo ? cnt_lo : cnt_hi + 64;
}

uint_fast8_t ctz128(__int128_t n)
{
	const uint_fast8_t cnt_hi = __builtin_ctzll(n >> 64);
	const uint_fast8_t cnt_lo = __builtin_ctzll(n);
	return cnt_lo ? cnt_lo : cnt_hi + 64;
}

uint_fast8_t popcount128(__int128_t n)
{
	const uint_fast8_t cnt_hi = __builtin_popcountll(n >> 64);
	const uint_fast8_t cnt_lo = __builtin_popcountll(n);
	return cnt_lo + cnt_hi;
}

template <typename T>
void print_binary(T x, int width = sizeof(T) * 8)
{
	for (int i = width; i >= 0; --i)
		std::cerr << ((x >> i) & 1 ? '1' : '0');
	std::cerr << std::endl;
}

struct Game
{
	Mask128 my_board;
	Mask128 opp_board;
	Mask128 valid_action;
	uint8_t last_action_index;
	uint8_t my_turn;
	uint8_t valid_action_count;

	Game * parent = NULL;
	Game * children = NULL;
	int children_count = 0;

	bool force_win = false;
	bool force_lose = false;
	bool visited = false;

	Game() {}
	Game(Mask128 _my_board, Mask128 _opp_board, int _my_turn, uint8_t _last_action_index, Game * _parent = NULL):
		my_board(_my_board),
		opp_board(_opp_board),
		my_turn(_my_turn),
		last_action_index(_last_action_index),
		valid_action_count(0),
		parent(_parent)
	{}

	int is_board_final(Mask16 board)
	{
		return ((board & test_ABC) == test_ABC) ||
				((board & test_DEF) == test_DEF) ||
				((board & test_GHI) == test_GHI) ||
				((board & test_ADG) == test_ADG) ||
				((board & test_BEH) == test_BEH) ||
				((board & test_CFI) == test_CFI) ||
				((board & test_AEI) == test_AEI) ||
				((board & test_CEG) == test_CEG);
	}

	int is_big_board_final(Mask128 board)
	{
		return ((board & test_ABC_big) == test_ABC_big) ||
				((board & test_DEF_big) == test_DEF_big) ||
				((board & test_GHI_big) == test_GHI_big) ||
				((board & test_ADG_big) == test_ADG_big) ||
				((board & test_BEH_big) == test_BEH_big) ||
				((board & test_CFI_big) == test_CFI_big) ||
				((board & test_AEI_big) == test_AEI_big) ||
				((board & test_CEG_big) == test_CEG_big);
	}

	void compute_first_valid_action()
	{
		valid_action = action_mask(40);
		valid_action_count = 1;
	}

	int get_action_list(Mask128 action_list[81])
	{
		int i = 0;
		Mask128 valid_action_copy = valid_action;
		while (valid_action_copy)
		{
			action_list[i++] = valid_action_copy & -valid_action_copy;
			valid_action_copy &= (valid_action_copy - 1);
		}
		return valid_action_count;
	}

	Mask128 rand_action()
	{
		const uint64_t low = (uint64_t)valid_action;
		const uint64_t high = (uint64_t)(valid_action >> 64);
		const uint64_t pop_low = __builtin_popcountll(low);
		const uint64_t r = rand() % valid_action_count;

		if (pop_low > 0 && r < pop_low)
		{
			return (Mask128)_pdep_u64(1ULL << r, low);
		}
		else
		{
			return (Mask128)_pdep_u64(1ULL << (r - pop_low), high) << 64;
		}
	}

	void play(Mask128 action)
	{
		last_action_index = action_index(action);
		const int small_board_index = last_action_index / 9;

		Mask128 &working_board = my_turn ? my_board : opp_board;

		working_board |= action;
		if (is_board_final(get_small_board(working_board, small_board_index)))
		{
			working_board |= get_small_board_mask(small_board_index);
		}

		my_turn = !my_turn;
		
		
		// cumule finished small board with the two players board
		const Mask128 free_cell_mask = (~(my_board | opp_board)) & full_one_mask;

		// get valid action by filtering only the free cells in the small board where you are forced to play
		const Mask128 force_play = get_small_board_mask(last_action_index % 9);
		valid_action = free_cell_mask & force_play;

		// if there is no valid action: play where you want
		if (!valid_action) valid_action = free_cell_mask;

		valid_action_count = popcount128(valid_action);
	}

	bool is_final()
	{
		return valid_action_count == 0 || is_big_board_final(my_board) || is_big_board_final(opp_board);
	}

	bool rollout_should_stop()
	{
		// stop rollout if game is final or if opponent has more big cells than me
		return is_final();
	}

	int result()
	{
		if (is_big_board_final(my_board))
			return 1;
		if (is_big_board_final(opp_board))
			return 0;
		// int diff = __builtin_popcount(my_big_board) - __builtin_popcount(opp_big_board);
		// if (diff > 0)
		// 	return 1;
		// if (diff < 0)
		// 	return 0;
		return 0;
	}

	void log()
	{
		char str[242] = (
			". . . | . . . | . . .\n"
			". . . | . . . | . . .\n"
			". . . | . . . | . . .\n"
			"------|-------|------\n"
			". . . | . . . | . . .\n"
			". . . | . . . | . . .\n"
			". . . | . . . | . . .\n"
			"------|-------|------\n"
			". . . | . . . | . . .\n"
			". . . | . . . | . . .\n"
			". . . | . . . | . . ."
		);
		Mask128 my_board_tmp = my_board;
		Mask128 opp_board_tmp = opp_board;
		for (size_t i = 0; i < 81; i++)
		{
			// if ((my_big_board >> (i / 9)) & 1)
			// {
			// 	if (i % 9 != 4)
			// 		str[game_index_to_str_index[i]] = 'o';
			// 	else
			// 		str[game_index_to_str_index[i]] = ' ';
			// }
			// else if ((opp_big_board >> (i / 9)) & 1)
			// {
			// 	if (i % 9 == 0 || i % 9 == 2 || i % 9 == 4|| i % 9 == 6 || i % 9 == 8)
			// 		str[game_index_to_str_index[i]] = 'x';
			// 	else
			// 		str[game_index_to_str_index[i]] = ' ';
			// }
			if (my_board_tmp & 1)
				str[game_index_to_str_index[i]] = 'o';
			else if (opp_board_tmp & 1)
				str[game_index_to_str_index[i]] = 'x';
			
			my_board_tmp >>= 1;
			opp_board_tmp >>= 1;
		}
		std::cerr << "my_turn = " << bool(my_turn) << "  last_action = " << (last_action_index != -1 ? index_to_pos[last_action_index] : "none") << std::endl;
		std::cerr << str << std::endl;
	}

	void expand()
	{
		Mask128 action_list[81];
		valid_action_count = get_action_list(action_list);
		children_count = valid_action_count;
		children = new Game[valid_action_count];
		for (size_t i = 0; i < valid_action_count; i++)
		{
			children[i] = *this;
			children[i].children = NULL;
			children[i].children_count = 0;
			children[i].parent = this;
			children[i].visited = false;
			children[i].play(action_list[i]);
		}
	}
};

Game * opponentPlay(Game * game, uint8_t action)
{
	for (size_t i = 0; i < game->children_count; i++)
	{
		if (action == game->children[i].last_action_index)
			return &(game->children[i]);
	}
	std::cerr << "Invalid action " << int(action) << " in:" << std::endl;
	for (size_t i = 0; i < game->children_count; i++)
	{
		std::cerr << int(game->children[i].last_action_index) << " ";
	}
	std::cerr << std::endl;
	game->log();
	exit(1);
	return game;
}

void readInput(uint8_t & opp_action, int * valid_action)
{
	int opp_row; int opp_col;
	cin >> opp_row >> opp_col; cin.ignore();
	if (opp_row == -1)
		opp_action = -1;
	else
		opp_action = pos_to_index[opp_row][opp_col];
	// std::cerr << mtos(opp_action, 81) << std::endl;

	int valid_action_count;
	cin >> valid_action_count; cin.ignore();
	for (int i = 0; i < valid_action_count; i++)
	{
		int row; int col;
		cin >> row >> col; cin.ignore();
		valid_action[i] = pos_to_index[row][col];
	}
}

void generateInput(Game * current, uint8_t & opp_action, int * valid_action)
{
	opp_action = action_index(current->rand_action());
}


int visited_state = 0;
int simulate(Game * current, Timer start, float timeout)
{
	double diff = start.diff(false);
	if (diff > timeout)
		return -1;
	
	visited_state++;

	if (current->is_final())
		return current->result();
	

	if (current->children == NULL)
		current->expand();

	{
		for (size_t i = 0; i < current->children_count; i++)
		{
			if (current->children[i].visited || !current->children[i].is_final())
				continue;
	
			int result = current->children[i].result();
			if (result == -1)
				return -1;
	
			if (current->my_turn)
			{
				if (result == 1)
				{
					current->force_win = true;
					current->visited = true;
					return 1;
				}
			}
			else
			{
				if (result == 0)
				{
					current->force_lose = true;
					current->visited = true;
					return 0;
				}
			}
		}
	}


	bool all_children_win = true;
	bool all_children_lose = true;
	for (size_t i = 0; i < current->children_count; i++)
	{
		if (current->children[i].visited)
			continue;

		int result = simulate(&current->children[i], start, timeout);
		if (result == -1)
			return -1;
		
		if (result == 1)
			all_children_lose = false;
		if (result == 0)
			all_children_win = false;

		if (current->my_turn)
		{
			if (result == 1)
			{
				current->force_win = true;
				current->visited = true;
				return 1;
			}
		}
		else
		{
			if (result == 0)
			{
				current->force_lose = true;
				current->visited = true;
				return 0;
			}
		}
	}

	if (current->my_turn)
	{
		if (all_children_lose)
		{
			current->force_lose = true;
			current->visited = true;
			return 0;
		}
	}
	else
	{
		if (all_children_win)
		{
			current->force_win = true;
			current->visited = true;
			return 1;
		}
	}


	current->visited = true;
	return 0;
}


// int main(int ac, char *av[])
// {
// 	srand(time(NULL));

// 	Game game = Game(0, 0, 0, -1);
// 	game.compute_first_valid_action();

// 	Timer start;

// 	simulate(&game, start, 1000);

// 	std::cerr << "Simulation result:" << std::endl;
// 	std::cerr << "time = " << start.diff() << std::endl;
// 	std::cerr << "count = " << visited_state << std::endl;

// 	return 0;
// }


/*
int main()
{
	Mask128 opp_action;
	int valid_action[81];

	Game initial_game = Game(0, 0, 0, -1);
	initial_game.compute_first_valid_action();
	State initial_state = State(initial_game, NULL);

	std::vector<State> state_list; state_list.reserve(1000000);
	std::vector<State> next_state_list; next_state_list.reserve(1000000);
	state_list.push_back(initial_state);

	__int128_t total_state_count = 0;
	int depth = 0;
	while (!state_list.empty())
	{
		std::cout << "depth = " << depth << std::endl;
		std::cout << "state_list size = " << state_list.size() << std::endl;
		total_state_count += state_list.size();

		Timer start;

		for (size_t i = 0; i < state_list.size(); i++)
		{
			State &current = state_list[i];

			if (current.game.is_final())
				continue;

			Mask128 action_list[81];
			Game g;
			current.game.get_action_list(action_list);
			for (size_t j = 0; j < current.game.valid_action_count; j++)
			{
				g = current.game;
				g.play(action_list[j]);
				next_state_list.emplace_back(State(g, &current));
			}
		}
		state_list.swap(next_state_list);
		next_state_list.clear();

		std::cout << "Simulation time per state = " << (start.diff() / state_list.size() * 1000) << " ms" << std::endl;

		depth++;
		std::cout << std::endl;
		if (depth > 8)
			break;
	}

	std::cout << "Next step state count = " << state_list.size() << std::endl;
}
*/


int main()
{
	srand(time(NULL));

	uint8_t opp_action;
	int valid_action[81];

	Game initial_game = Game(0, 0, 0, -1, NULL);
	initial_game.compute_first_valid_action();

	Game *current = &initial_game;


	{ // first turn
		// readInput(opp_action, valid_action);
		generateInput(current, opp_action, valid_action);
		Timer start;
	
		if (opp_action != -1)
			current->play(action_mask(opp_action));
		else
			current->my_turn = 1;
	
		visited_state = 0;
		simulate(current, start, 990);

		std::cerr << "Simulation result:" << std::endl;
		std::cerr << "time = " << start.diff() << std::endl;
		std::cerr << "count = " << visited_state << std::endl;

		int visited_state_children = 0;
		Game *child = current->children;
		for (size_t i = 0; i < current->children_count; i++)
		{
			if (current->children[i].visited)
				visited_state_children++;

			if (child->force_lose)
			{
				child = &current->children[i];
				if (child->force_win)
					break;
			}
		}

		std::cerr << "visited_state_children = " << visited_state_children << " / " << current->children_count << std::endl;
	
		if (child == NULL)
		{
			std::cerr << "There is no children" << std::endl;
			cout << index_to_pos[valid_action[0]] << std::endl;
			current = opponentPlay(current, valid_action[0]);
		}
		else
		{
			cout << index_to_pos[child->last_action_index] << std::endl;
			std::cerr << "response time " << start.diff() << std::endl;
			current = child;
		}
		// current->log();
		std::cerr << std::endl;
	}

	while (1)
	{
		if (current->is_final())
			break;

		// readInput(opp_action, valid_action);
		generateInput(current, opp_action, valid_action);
		Timer start;

		if (current->is_final())
			break;

		current = opponentPlay(current, opp_action);

		visited_state = 0;
		simulate(current, start, 90);

		std::cerr << "Simulation result:" << std::endl;
		std::cerr << "time = " << start.diff() << std::endl;
		std::cerr << "count = " << visited_state << std::endl;

		// current->log();
		int visited_state_children = 0;
		Game *child = current->children;
		for (size_t i = 0; i < current->children_count; i++)
		{
			if (current->children[i].visited)
				visited_state_children++;
			if (current->children[i].force_win)
			{
				child = &current->children[i];
				break;
			}
		}

		std::cerr << "visited_state_children = " << visited_state_children << " / " << current->children_count << std::endl;

		if (child == NULL)
		{
			std::cerr << "There is no children" << std::endl;
			cout << index_to_pos[valid_action[0]] << std::endl;
			current = opponentPlay(current, valid_action[0]);
		}
		else
		{
			cout << index_to_pos[child->last_action_index] << std::endl;
			std::cerr << "response time " << start.diff() << std::endl;
			current = child;
		}
		// current->log();
		std::cerr << std::endl;
	}

	std::cerr << "result = " << current->result() << std::endl;
}

