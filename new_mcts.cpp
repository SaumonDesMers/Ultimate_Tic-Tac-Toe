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

	ABC = 000000111 = 0x7
	DEF = 000111000 = 0x38
	GHI = 111000000 = 0x1c0
	ADG = 001001001 = 0x49
	BEH = 010010010 = 0x92
	CFI = 100100100 = 0x124
	AEI = 100010001 = 0x111
	CEG = 001010100 = 0x54


	o o o | . . . | . . .
	o   o | . . . | . . .
	o o o | . . . | . . .
	------|-------|------
	. . . | x   x | . . .
	. . . |   x   | . . .
	. . . | x   x | . . .
	------|-------|------
	. . . | . . . | . . .
	. . . | . . . | . . .
	. . . | . . . | . . .

*/

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
		putchar(((x >> i) & 1) ? '1' : '0');
	putchar('\n');
}

struct Game
{
	Mask16 my_big_board;
	Mask16 opp_big_board;
	Mask128 my_board;
	Mask128 opp_board;
	Mask128 valid_action;
	Mask128 last_action;
	int my_turn;
	int valid_action_count;

	Game() {}
	Game(Mask16 _my_big_board, Mask16 _opp_big_board, Mask128 _my_board, Mask128 _opp_board, int _my_turn, Mask128 _last_action) :
		my_big_board(_my_big_board),
		opp_big_board(_opp_big_board),
		my_board(_my_board),
		opp_board(_opp_board),
		my_turn(_my_turn),
		last_action(_last_action),
		valid_action_count(0)
	{}

	template<class Mask>
	int is_board_final(Mask board)
	{
		return ((board & 0x7) == 0x7) ||
			((board & 0x38) == 0x38) ||
			((board & 0x1c0) == 0x1c0) ||
			((board & 0x49) == 0x49) ||
			((board & 0x92) == 0x92) ||
			((board & 0x124) == 0x124) ||
			((board & 0x111) == 0x111) ||
			((board & 0x54) == 0x54);
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
		const int small_board_index = action_index(action) / 9;

		Mask128 &working_board = my_turn ? my_board : opp_board;
		Mask16 &working_big_board = my_turn ? my_big_board : opp_big_board;

		working_board |= action;
		if (is_board_final(get_small_board(working_board, small_board_index)))
		{
			working_big_board |= 1 << small_board_index;
			working_board |= get_small_board_mask(small_board_index);
		}

		my_turn = !my_turn;
		last_action = action;
		
		
		// cumule finished small board with the two players board
		const Mask128 free_cell_mask = (~(my_board | opp_board)) & full_one_mask;

		// get valid action by filtering only the free cells in the small board where you are forced to play
		const Mask128 force_play = get_small_board_mask(action_index(last_action) % 9);
		valid_action = free_cell_mask & force_play;

		// if there is no valid action: play where you want
		if (!valid_action) valid_action = free_cell_mask;

		valid_action_count = popcount128(valid_action);
	}

	bool is_final()
	{
		return valid_action_count == 0 || is_board_final(my_big_board) || is_board_final(opp_big_board);
	}

	bool rollout_should_stop()
	{
		// stop rollout if game is final or if opponent has more big cells than me
		return is_final() || popcount128(my_big_board) < popcount128(opp_big_board);
	}

	float result()
	{
		if (is_board_final(my_big_board))
			return 1;
		if (is_board_final(opp_big_board))
			return 0;
		int diff = __builtin_popcount(my_big_board) - __builtin_popcount(opp_big_board);
		if (diff > 0)
			return 1;
		if (diff < 0)
			return 0;
		return 0.5;
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

			if ((my_big_board >> (i / 9)) & 1)
			{
				if (i % 9 != 4)
					str[game_index_to_str_index[i]] = 'o';
				else
					str[game_index_to_str_index[i]] = ' ';
			}
			else if ((opp_big_board >> (i / 9)) & 1)
			{
				if (i % 9 == 0 || i % 9 == 2 || i % 9 == 4|| i % 9 == 6 || i % 9 == 8)
					str[game_index_to_str_index[i]] = 'x';
				else
					str[game_index_to_str_index[i]] = ' ';
			}
			else if (my_board_tmp & 1)
				str[game_index_to_str_index[i]] = 'o';
			else if (opp_board_tmp & 1)
				str[game_index_to_str_index[i]] = 'x';
			
			my_board_tmp >>= 1;
			opp_board_tmp >>= 1;
		}
		int action_index = action_index(last_action);
		std::cerr << "my_turn = " << my_turn << "  last_action = " << (action_index != -1 ? index_to_pos[action_index(action_index)] : "none") << std::endl;
		std::cerr << str << std::endl;
	}
};

struct State
{
	Game game;
	State *parent;
	State *children;
	int children_count = 0;
	float value = 0;
	int visit_count = 0;

	State(Game __game = Game(0, 0, 0, 0, 0, -1), State * __parent = NULL):
		game(__game), parent(__parent) {}

	~State()
	{
		// for (size_t i = 0; i < children_count; i++)
		// 	delete children[i];
		// delete[] children;
	}

	State * expand()
	{
		// if final state return current state
		if (game.is_final())
			return this;

		// create games for each valid action
		Game next_game[81];
		size_t next_game_count = 0;
		Mask128 action_list[81];
		Game g;
		game.get_action_list(action_list);
		for (size_t i = 0; i < game.valid_action_count; i++)
		{
			g = game;
			// std::cerr << action_list[i] << std::endl;
			g.play(action_list[i]);
			next_game[next_game_count++] = g;
		}
		
		// detect if there is a winning final state
		Game *winning_final_game = NULL;
		for (size_t i = 0; i < game.valid_action_count; i++)
		{
			if (next_game[i].is_final() && next_game[i].result() == 1)
			{
				winning_final_game = &next_game[i];
				break;
			}
		}

		// allocate memory for children
		children = new State[next_game_count];

		children_count = 0;
		// if there is a winning final state: expand only this one
		if (winning_final_game != NULL)
		{
			children[children_count++] = State(*winning_final_game, this);
		}
		// else: expand all next state
		else
		{
			for (size_t i = 0; i < next_game_count; i++)
			{
				// don't expand the losing final state
				if (next_game[i].is_final() && next_game[i].result() == 0)
					continue;
				children[children_count++] = State(next_game[i], this);
			}
		}
		return children;
	}

	float UCB1()
	{
		if (parent == NULL)
			return -1;
		
		if (visit_count == 0)
			return __builtin_huge_valf();
		
		return (value / visit_count) + 2 * sqrt(::log(parent->visit_count) / visit_count);
	}

	State * max_UCB1_child()
	{
		State *child = NULL;
		float max_UCB1 = 0;
		for (size_t i = 0; i < children_count; i++)
		{
			float UCB1 = children[i].UCB1();
			if (UCB1 > max_UCB1)
			{
				max_UCB1 = UCB1;
				child = &children[i];
			}
		}
		return child;
	}

	void backpropagate(float new_value, State * root, int expanded_child_count)
	{
		visit_count++;
		value += new_value;
		if (parent != NULL && this != root)
			parent->backpropagate(new_value, root, expanded_child_count);
	}

	float rollout()
	{
		Game g = game;
		while (!g.rollout_should_stop())
		{
			g.play(g.rand_action());
		}
		return g.result();
	}

	State * max_average_value_child()
	{
		if (children_count == 0)
			return NULL;
		State *child = &children[0];
		float max_average_value = -1;
		for (size_t i = 0; i < children_count; i++)
		{
			float average_value = children[i].value / children[i].visit_count;
			if (average_value > max_average_value)
			{
				max_average_value = average_value;
				child = &children[i];
			}
		}
		return child;
	}

	void log()
	{
		std::cerr << "State{t=" << setw(7) << left << value <<
			",n=" << setw(5) << left << visit_count <<
			",av=" << setw(10) << left << value / visit_count <<
			",action=" << (game.last_action != -1 ? index_to_pos[action_index(game.last_action)] : "none") <<
			"}" << std::endl;
	}

};

State * opponentPlay(State * state, Mask128 action)
{
	for (size_t i = 0; i < state->children_count; i++)
	{
		if (action == state->children[i].game.last_action)
			return &(state->children[i]);
	}
	std::cerr << "Invalid action " << action_index(action) << " in:" << std::endl;
	for (size_t i = 0; i < state->children_count; i++)
	{
		std::cerr << action_index(state->children[i].game.last_action) << " ";
	}
	std::cerr << std::endl;
	state->game.log();
	exit(1);
	return state;
}

void readInput(Mask128 & opp_action, int * valid_action)
{
	int opp_row; int opp_col;
	cin >> opp_row >> opp_col; cin.ignore();
	if (opp_row == -1)
		opp_action = 0;
	else
		opp_action = int128(1) << pos_to_index[opp_row][opp_col];
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

void generateInput(State * current, Mask128 & opp_action, int * valid_action)
{
	opp_action = current->game.rand_action();
}

State * mcts(State * initial_state, Timer start, float timeout)
{
	State * current;
	int nb_of_simule = 0;

	while (true)
	{
		current = initial_state;
		
		while (current->children_count > 0)
			current = current->max_UCB1_child();
		
		int extended_state_count = 0;
		if (current->visit_count > 0)
		{
			State * tmp = current;
			current = current->expand();
			extended_state_count = tmp->children_count;
		}
		
		float value = current->rollout();
		
		current->backpropagate(value, initial_state, extended_state_count);
		
		nb_of_simule++;

		double diff = start.diff(false);
		if (diff > timeout)
			break;
	}

	std::cerr << "Simulation result:" << std::endl;
	std::cerr << "time = " << start.diff() << std::endl;
	std::cerr << "count = " << nb_of_simule << std::endl;

	return initial_state->max_average_value_child();
}

State * mcts(State * initial_state, int max_iter)
{
	State *current;
	int nb_of_simule = 0;

	while (nb_of_simule < max_iter)
	{
		current = initial_state;

		while (current->children_count > 0)
			current = current->max_UCB1_child();

		if (current->visit_count > 0)
			current = current->expand();

		float value = current->rollout();

		current->backpropagate(value, initial_state, 0);

		nb_of_simule++;
	}
	return initial_state->max_average_value_child();
}

/*
int main(int ac, char *av[])
{
	srand(time(NULL));

	Game game = Game(0, 0, 0, 0, 0, -1);
	game.compute_first_valid_action();

	State *state = new State(game, NULL);

	Timer start;

	State *child = mcts(state, start, 1000);

	delete state;
	return 0;
}
*/


int main()
{
	Mask128 opp_action;
	int valid_action[81];

	Game initial_game = Game(0, 0, 0, 0, 0, -1);
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
}


/*
int main()
{
	srand(time(NULL));

	Mask128 opp_action;
	int valid_action[81];

	Game initial_game = Game(0, 0, 0, 0, 0, -1);
	initial_game.compute_first_valid_action();
	State *initial_state = new State(initial_game, NULL);

	State *current = initial_state;


	{ // first turn
		readInput(opp_action, valid_action);
		// generateInput(current, opp_action, valid_action);
		Timer start;
	
		if (opp_action != 0)
			current->game.play(opp_action);
		else
			current->game.my_turn = 1;
	
		State *child = mcts(current, start, 990);
	
		if (child == NULL)
		{
			std::cerr << "mcts did not return any action" << std::endl;
			cout << index_to_pos[valid_action[0]] << std::endl;
			current = opponentPlay(current, valid_action[0]);
		}
		else
		{
			cout << index_to_pos[action_index(child->game.last_action)] << std::endl;
			std::cerr << "response time " << start.diff() << std::endl;
			current = child;
		}
		// current->game.log();
		std::cerr << std::endl;
	}

	while (1)
	{
		if (current->game.is_final())
			break;

		readInput(opp_action, valid_action);
		// generateInput(current, opp_action, valid_action);
		Timer start;

		if (current->game.is_final())
			break;

		current = opponentPlay(current, opp_action);

		// current->game.log();
		State *child = mcts(current, start, 90);
		// State *child = mcts(current, 100);

		if (child == NULL)
		{
			std::cerr << "mcts did not return any action" << std::endl;
			cout << index_to_pos[valid_action[0]] << std::endl;
			current = opponentPlay(current, valid_action[0]);
		}
		else
		{
			cout << index_to_pos[action_index(child->game.last_action)] << std::endl;
			std::cerr << "response time " << start.diff() << std::endl;
			current = child;
		}
		// current->game.log();
		std::cerr << std::endl;
	}

	std::cerr << "result = " << current->game.result() << std::endl;
	delete initial_state;
}
*/
