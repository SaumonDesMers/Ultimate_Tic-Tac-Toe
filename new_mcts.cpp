#ifndef DEBUG
	#undef _GLIBCXX_DEBUG
	#pragma GCC optimize "Ofast,unroll-loops,omit-frame-pointer,inline"
	#pragma GCC option("arch=native", "tune=native", "no-zero-upper")
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
#define actionIndex(am) int(ffs128(am) - 1)
#define actionMask(ai) (int128(1) << ai)

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

int stateId = 0;

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

const Mask128 fullOneMask = ~(int128(0x7fffffffffff) << 81);

const int gameIndexToStrIndex[81] = {
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

const char indexToPos[81][4] = {
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

const int posToIndex[9][9] = {
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
void print_binary(T x, int width = sizeof(T) * 8) {
	for (int i = width; i >= 0; --i)
		putchar(((x >> i) & 1) ? '1' : '0');
	putchar('\n');
}

struct Game
{
	Mask16 myBigBoard;
	Mask16 oppBigBoard;
	Mask128 myBoard;
	Mask128 oppBoard;
	Mask128 validAction;
	Mask128 lastAction;
	int myTurn;
	int validActionCount;
	int validActionComputed;
	int depth;

	Game() {}
	Game(Mask16 _myBigBoard, Mask16 _oppBigBoard, Mask128 _myBoard, Mask128 _oppBoard, int _myTurn, Mask128 _lastAction, int _depth) :
		myBigBoard(_myBigBoard),
		oppBigBoard(_oppBigBoard),
		myBoard(_myBoard),
		oppBoard(_oppBoard),
		myTurn(_myTurn),
		lastAction(_lastAction),
		validActionCount(0),
		validActionComputed(false),
		depth(_depth)
		{
			computeValidAction();
		}

	Game(const Game &src) :
		myBigBoard(src.myBigBoard),
		oppBigBoard(src.oppBigBoard),
		myBoard(src.myBoard),
		oppBoard(src.oppBoard),
		validAction(src.validAction),
		myTurn(src.myTurn),
		lastAction(src.lastAction),
		validActionCount(src.validActionCount),
		validActionComputed(src.validActionComputed),
		depth(src.depth) {}

	Game &operator=(const Game &src)
	{
		myBigBoard = src.myBigBoard;
		oppBigBoard = src.oppBigBoard;
		myBoard = src.myBoard;
		oppBoard = src.oppBoard;
		validAction = src.validAction;
		myTurn = src.myTurn;
		lastAction = src.lastAction;
		validActionCount = src.validActionCount;
		validActionComputed = src.validActionComputed;
		depth = src.depth;
		return *this;
	}

	Mask128 getUniqueSmallBoard(Mask128 board, int i) { return (board >> (i * 9)) & 0x1ff; }

	Mask128 smallBoardMask(int i) { return (int128(0x1ff) << (i * 9)); }

	bool isSmallBoardFinal(int i) { return (((myBigBoard | oppBigBoard) >> i) & 1); }

	int firstActionIndex(Mask128 mask) { return ffs128(validAction) - 1; }

	template<class Mask>
	int boardIsFinal(Mask board)
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

	void computeValidAction()
	{
		if (validActionComputed)
			return;

		// std::cerr << "Compute valid action" << std::endl;
		// std::cerr << "myBoard	  = " << mtos(myBoard, 81) << std::endl;
		// std::cerr << "oppBoard	 = " << mtos(oppBoard, 81) << std::endl;

		// cumule finished small board with the two players board
		const Mask128 freeCellMask = (~(myBoard | oppBoard)) & fullOneMask;
		// std::cerr << "freeCellMask = " << mtos(freeCellMask, 81) << std::endl;

		// get valid action by filtering only the free cells in the small board where you are forced to play
		// but if there is no last action: juste play in the middle
		// std::cerr << "action index = " << actionIndex(lastAction) << std::endl;
		const Mask128 forcePlay = (lastAction == -1 ? int128(1) << 40 : smallBoardMask(actionIndex(lastAction) % 9));
		// std::cerr << "forcePlay	= " << mtos(forcePlay, 81) << std::endl;
		validAction = freeCellMask & forcePlay;
		// std::cerr << "validAction  = " << mtos(validAction, 81) << std::endl;

		// if there is no valid action: play where you want
		if (!validAction) validAction = freeCellMask;
		// std::cerr << "validAction  = " << mtos(validAction, 81) << std::endl;

		validActionCount = popcount128(validAction);
		validActionComputed = true;
	}

	int getActionList(Mask128 actionList[81])
	{
		int i = 0;
		Mask128 validActionCopy = validAction;
		while (validActionCopy)
		{
			actionList[i++] = validActionCopy & -validActionCopy;
			validActionCopy &= (validActionCopy - 1);
		}
		return validActionCount;
	}

	Mask128 randAction()
	{
		const uint64_t low = (uint64_t)validAction;
		const uint64_t high = (uint64_t)(validAction >> 64);
		const uint64_t pop_low = __builtin_popcountll(low);
		const uint64_t r = rand() % validActionCount;

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
		// int actionIndex = actionIndex(action);
		// if (actionIndex > 80)
		// {
		// 	std::cerr << "wtf action = " << actionIndex << std::endl;
		// 	exit(0);
		// }
		// std::cerr << "Play" << std::endl;
		// std::cerr << "action is	= " << mtos(action, 81) << std::endl;

		const int smallBoardIndex = actionIndex(action) / 9;

		Mask128 &workingBoard = myTurn ? myBoard : oppBoard;
		Mask16 &workingBigBoard = myTurn ? myBigBoard : oppBigBoard;

		// if (workingBoard & action)
		// {
		// 	std::cerr << "action non disponible" << std::endl;
		// 	std::cerr << "validAction  = " << mtos(validAction, 81) << std::endl;
		// 	std::cerr << "action	   = " << mtos(action, 81) << std::endl;
		// 	std::cerr << "workingBoard = " << mtos(workingBoard, 81) << std::endl;
		// 	exit(0);
		// }
		workingBoard |= action;
		int result = boardIsFinal(getUniqueSmallBoard(workingBoard, smallBoardIndex));
		if (result)
		{
			workingBigBoard |= result << smallBoardIndex;
			workingBoard |= smallBoardMask(smallBoardIndex);
		}

		depth++;
		myTurn = !myTurn;
		lastAction = action;
		validActionCount = 0;
		validActionComputed = false;
		computeValidAction();
	}

	bool final()
	{
		return validActionCount == 0 || boardIsFinal(myBigBoard) || boardIsFinal(oppBigBoard);
	}

	float result()
	{
		if (boardIsFinal(myBigBoard))
			return 1;
		if (boardIsFinal(oppBigBoard))
			return 0;
		int diff = __builtin_popcount(myBigBoard) - __builtin_popcount(oppBigBoard);
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
		Mask128 myBoardTmp = myBoard;
		Mask128 oppBoardTmp = oppBoard;
		for (size_t i = 0; i < 81; i++)
		{

			if ((myBigBoard >> (i / 9)) & 1)
			{
				if (i % 9 != 4)
					str[gameIndexToStrIndex[i]] = 'o';
				else
					str[gameIndexToStrIndex[i]] = ' ';
			}
			else if ((oppBigBoard >> (i / 9)) & 1)
			{
				if (i % 9 == 0 || i % 9 == 2 || i % 9 == 4|| i % 9 == 6 || i % 9 == 8)
					str[gameIndexToStrIndex[i]] = 'x';
				else
					str[gameIndexToStrIndex[i]] = ' ';
			}
			else if (myBoardTmp & 1)
				str[gameIndexToStrIndex[i]] = 'o';
			else if (oppBoardTmp & 1)
				str[gameIndexToStrIndex[i]] = 'x';
			
			myBoardTmp >>= 1;
			oppBoardTmp >>= 1;
		}
		int actionIndex = actionIndex(lastAction);
		std::cerr << "myTurn = " << myTurn << "  lastAction = " << (actionIndex != -1 ? indexToPos[actionIndex(actionIndex)] : "none") << std::endl;
		std::cerr << str << std::endl;
	}
};

struct State
{
	float value;
	int visitCount;
	Game game;
	State *parent;
	State *children[81];
	int childrenCount;
	int id;

	State(Game __game, State *__parent) : value(0), visitCount(0), game(__game), parent(__parent), childrenCount(0), id(stateId++) {}
	~State()
	{
		for (size_t i = 0; i < childrenCount; i++)
			delete children[i];
	}

	State *expand()
	{
		// if final state return current state
		if (game.final())
			return this;

		// create games for each valid action
		Game nextGame[81];
		size_t nextGameCount = 0;
		Mask128 actionList[81];
		Game g;
		game.getActionList(actionList);
		for (size_t i = 0; i < game.validActionCount; i++)
		{
			g = game;
			// std::cerr << actionList[i] << std::endl;
			g.play(actionList[i]);
			nextGame[nextGameCount++] = g;
		}
		
		// detect if there is a winning final state (still don't know if useful)
		Game *winningFinalGame = NULL;
		for (size_t i = 0; i < game.validActionCount; i++)
		{
			if (nextGame[i].final() && nextGame[i].result() == 1)
			{
				winningFinalGame = &nextGame[i];
				break;
			}
		}

		childrenCount = 0;
		// if there is a final state: expand only this one
		if (winningFinalGame != NULL)
		{
			children[childrenCount++] = new State(*winningFinalGame, this);
		}
		// else: expand all next state
		else
		{
			for (size_t i = 0; i < nextGameCount; i++)
			{
				if (nextGame[i].final() && nextGame[i].result() == 0)
					continue;
				children[childrenCount++] = new State(nextGame[i], this);
			}
		}
		return children[0];
	}

	float UCB1()
	{
		if (parent == NULL)
			return -1;
		
		if (visitCount == 0)
			return __builtin_huge_valf();
		
		return (value / visitCount) + 2 * sqrt(::log(parent->visitCount) / visitCount);
	}

	State *maxUCB1Child()
	{
		State *child = NULL;
		float maxUCB1 = 0;
		for (size_t i = 0; i < childrenCount; i++)
		{
			float UCB1 = children[i]->UCB1();
			if (UCB1 > maxUCB1)
			{
				maxUCB1 = UCB1;
				child = children[i];
			}
		}
		return child;
	}

	void backpropagate(float __value, State *root)
	{
		visitCount++;
		value += __value;
		if (parent != NULL && this != root)
			parent->backpropagate(__value, root);
	}

	float rollout()
	{
		Game g = game;
		while (!g.final())
		{
			g.play(g.randAction());
		}
		return g.result();
	}

	State *maxAverageValueChild()
	{
		if (childrenCount == 0)
			return NULL;
		State *child = children[0];
		float maxAverageValue = -1;
		for (size_t i = 0; i < childrenCount; i++)
		{
			float averageValue = children[i]->value / children[i]->visitCount;
			if (averageValue > maxAverageValue)
			{
				maxAverageValue = averageValue;
				child = children[i];
			}
		}
		return child;
	}

	void log()
	{
		std::cerr << "State{t=" << setw(7) << left << value <<
			",n=" << setw(5) << left << visitCount <<
			",av=" << setw(10) << left << value / visitCount <<
			",action=" << (game.lastAction != -1 ? indexToPos[actionIndex(game.lastAction)] : "none") <<
			"}" << std::endl;
	}

};

State *opponentPlay(State *state, Mask128 action)
{
	for (size_t i = 0; i < state->childrenCount; i++)
	{
		if (action == state->children[i]->game.lastAction)
			return state->children[i];
	}
	std::cerr << "Invalid action " << actionIndex(action) << " in:" << std::endl;
	for (size_t i = 0; i < state->childrenCount; i++)
	{
		std::cerr << actionIndex(state->children[i]->game.lastAction) << " ";
	}
	std::cerr << std::endl;
	state->game.log();
	exit(1);
	return state;
}

void readInput(Mask128 &oppAction, int *validAction)
{
	int opp_row; int opp_col;
	cin >> opp_row >> opp_col; cin.ignore();
	if (opp_row == -1)
		oppAction = 0;
	else
		oppAction = int128(1) << posToIndex[opp_row][opp_col];
	// std::cerr << mtos(oppAction, 81) << std::endl;

	int valid_action_count;
	cin >> valid_action_count; cin.ignore();
	for (int i = 0; i < valid_action_count; i++)
	{
		int row; int col;
		cin >> row >> col; cin.ignore();
		validAction[i] = posToIndex[row][col];
	}
}

void generateInput(State *current, Mask128 &oppAction, int *validAction)
{
	oppAction = current->game.randAction();
}

State *mcts(State *initialState, Timer start, float timeout)
{
	State *current;
	int nbOfSimule = 0;

	while (true)
	{
		current = initialState;
		
		timer.set();
		while (current->childrenCount > 0)
			current = current->maxUCB1Child();
		
		if (current->visitCount > 0)
			current = current->expand();
		
		float value = current->rollout();
		
		current->backpropagate(value, initialState);
		
		nbOfSimule++;

		double diff = start.diff(false);
		if (diff > timeout)
			break;
	}
	std::cerr << "nb of simule = " << nbOfSimule << std::endl;
	return initialState->maxAverageValueChild();
}

State *mcts(State *initialState, int maxIter)
{
	State *current;
	int nbOfSimule = 0;

	while (nbOfSimule < maxIter)
	{

		current = initialState;

		while (!current->childrenCount)
			current = current->maxUCB1Child();

		if (current->visitCount > 0)
			current = current->expand();

		float value = current->rollout();

		current->backpropagate(value, initialState);

		nbOfSimule++;
	}
	std::cerr << "nb of simule = " << nbOfSimule << std::endl;
	return initialState->maxAverageValueChild();
}

int main(int ac, char *av[])
{
	srand(time(NULL));

	Game game = Game(0, 0, 0, 0, 0, -1, 0);

	State *state = new State(game, NULL);

	Timer start;

	State *child = mcts(state, start, 1000);
	std::cerr << "Simulation time = " << start.diff() << std::endl;
	// child->game.log();

	// for (size_t i = 0; i < state->game.validActionCount; i++)
	// 	state->children[i]->log();

	// while (!game.final())
	// {
	// 	std::cerr << "- NEXT TURN -" << std::endl;
	// 	game.play(game.randAction());
	// 	std::string str;
	// 	getline(cin, str);
	// }

	delete state;
	return 0;
}

/*
int main()
{
	srand(time(NULL));

	Mask128 oppAction;
	int validAction[81];

	Game initialGame = Game(0, 0, 0, 0, 0, -1, 0);
	State *initialState = new State(initialGame, NULL);

	State *current = initialState;

	int first = true;
	while (1)
	{

		if (current->game.final())
		{
			std::cerr << "result = " << current->game.result() << std::endl;
			delete initialState;
			return 0;
		}

		readInput(oppAction, validAction);
		// generateInput(current, oppAction, validAction);
		Timer start;

		if (current->game.final())
		{
			std::cerr << "result = " << current->game.result() << std::endl;
			delete initialState;
			return 0;
		}

		if (first)
		{
			if (oppAction != 0)
				current->game.play(oppAction);
			else
				current->game.myTurn = 1;
		}
		else
		{
			current = opponentPlay(current, oppAction);
		}

		current->game.log();
		// std::string str;
		// getline(cin, str);

		// my play
		State *child = mcts(current, start, first ? 990 : 90);

		// for (size_t i = 0; i < current->game.validActionCount; i++)
		// 	current->children[i]->log();

		std::cerr << "simule time " << start.diff() << std::endl;

		if (child == NULL)
		{
			std::cerr << "mcts did not return any action" << std::endl;
			cout << indexToPos[validAction[0]] << std::endl;
			current = opponentPlay(current, validAction[0]);
		}
		else
		{
			cout << indexToPos[actionIndex(child->game.lastAction)] << std::endl;
			current = child;
		}
		current->game.log();
		std::cerr << std::endl;
		first = false;
		// getline(cin, str);
	}
}
*/
