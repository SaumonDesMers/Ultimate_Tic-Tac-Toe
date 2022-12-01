#undef _GLIBCXX_DEBUG                // disable run-time bound checking, etc
#pragma GCC optimize("Ofast,inline") // Ofast = O3,fast-math,allow-store-data-races,no-protect-parens

#ifndef __POPCNT__ // not march=generic

#pragma GCC target("bmi,bmi2,lzcnt,popcnt")                      // bit manipulation
#pragma GCC target("movbe")                                      // byte swap
#pragma GCC target("aes,pclmul,rdrnd")                           // encryption
#pragma GCC target("avx,avx2,f16c,fma,sse3,ssse3,sse4.1,sse4.2") // SIMD

#endif // end !POPCNT

#include <iostream>
#include <string>
#include <cmath>
#include <random>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <cstring>

#define int128(x) static_cast<__int128_t>(x)
#define FULL_ONE_MASK ~(int128(0x7fffffffffff) << 81)
#define random(min, max) min + rand() % (max - min)
#define actionIndex(am) int(int128_ffs(am) - 1)
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
// uint64_t random(int min, int max) {
// 	uint64_t s1 = shuffle_table[0];
// 	uint64_t s0 = shuffle_table[1];
// 	uint64_t result = s0 + s1;
// 	shuffle_table[0] = s0;
// 	s1 ^= s1 << 23;
// 	shuffle_table[1] = s1 ^ s0 ^ (s1 >> 18) ^ (s0 >> 5);
// 	return min + result % (max - min);
// }

struct Timer {
	clock_t time_point;

	Timer() { set(); }

	void set() { time_point = clock(); }

	double diff(bool reset = true) {
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

const string indexToPos[81] = {
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

uint_fast8_t int128_ffs(__int128_t n) {
	const uint_fast8_t cnt_hi = __builtin_ffsll(n >> 64);
	const uint_fast8_t cnt_lo = __builtin_ffsll(n);
	return cnt_lo ? cnt_lo : cnt_hi + 64;
}

uint_fast8_t int128_popcount(__int128_t n) {
	const uint_fast8_t cnt_hi = __builtin_popcountll(n >> 64);
	const uint_fast8_t cnt_lo = __builtin_popcountll(n);
	return cnt_lo + cnt_hi;
}

template<class Mask>
string mtos(Mask mask, size_t size) {
	string str;
	for (size_t i = 0; i < size; i++) {
		if (i % 9 == 0)
			str.insert(0, " ");
		str.insert(0, to_string(int(mask & 1)));
		mask >>= 1;
	}
	return str;
}

struct Game {
	Mask16 myBigBoard;
	Mask16 oppBigBoard;
	Mask128 myBoard;
	Mask128 oppBoard;
	Mask128 nonFreeCell;
	Mask128 validAction;
	Mask128 lastAction;
	int myTurn;
	int validActionCount;
	int validActionComputed;
	int depth;

	Game() {};
	Game(Mask16 _myBigBoard, Mask16 _oppBigBoard, Mask128 _myBoard, Mask128 _oppBoard, int _myTurn, Mask128 _lastAction, int _depth) :
		myBigBoard(_myBigBoard),
		oppBigBoard(_oppBigBoard),
		myBoard(_myBoard),
		oppBoard(_oppBoard),
		nonFreeCell(0),
		myTurn(_myTurn),
		lastAction(_lastAction),
		validActionCount(0),
		validActionComputed(false),
		depth(_depth) {
			computeValidAction();
		}

	Game(const Game &src) :
		myBigBoard(src.myBigBoard),
		oppBigBoard(src.oppBigBoard),
		myBoard(src.myBoard),
		oppBoard(src.oppBoard),
		nonFreeCell(src.nonFreeCell),
		validAction(src.validAction),
		myTurn(src.myTurn),
		lastAction(src.lastAction),
		validActionCount(src.validActionCount),
		validActionComputed(src.validActionComputed),
		depth(src.depth) {}

	Game &operator=(const Game &src) {
		myBigBoard = src.myBigBoard;
		oppBigBoard = src.oppBigBoard;
		myBoard = src.myBoard;
		oppBoard = src.oppBoard;
		nonFreeCell = src.nonFreeCell;
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

	int firstActionIndex(Mask128 mask) { return int128_ffs(validAction) - 1; }

	template<class Mask>
	int boardIsFinal(Mask board) {
		// the board must be at the right of the mask
		Mask winMask[8] = {0x7,0x38,0x1c0,0x49,0x92,0x124,0x111,0x54};

		for (size_t i = 0; i < 8; i++) {
			// maybe test "!(winMask[i] ^ (board & winMask))" to see if it's faster
			if (winMask[i] == (board & winMask[i]))
				return 1;
		}
		return 0;
	}

	void computeValidAction() {
		if (validActionComputed)
			return;

		// cerr << "Compute valid action" << endl;
		// cerr << "nonFreeCell  = " << mtos(nonFreeCell, 81) << endl;
		// cerr << "myBoard      = " << mtos(myBoard, 81) << endl;
		// cerr << "oppBoard     = " << mtos(oppBoard, 81) << endl;

		// cumule finished small board with the two players board
		Mask128 freeCellMask = ~(nonFreeCell | myBoard | oppBoard);
		// cerr << "freeCellMask = " << mtos(freeCellMask, 81) << endl;

		// get valid action by filtering only the free cells in the small board where you are forced to play
		// but if there is no last action: juste play in the middle
		// cerr << "action index = " << actionIndex(lastAction) << endl;
		Mask128 forcePlay = (lastAction == -1 ? int128(1) << 40 : smallBoardMask(actionIndex(lastAction) % 9));
		// cerr << "forcePlay    = " << mtos(forcePlay, 81) << endl;
		validAction = freeCellMask & forcePlay;
		// cerr << "validAction  = " << mtos(validAction, 81) << endl;

		// if there is no valid action: play where you want
		validAction = (validAction ? validAction : freeCellMask) & fullOneMask;
		// cerr << "validAction  = " << mtos(validAction, 81) << endl;

		validActionCount = int128_popcount(validAction);
		validActionComputed = true;
	}

	int getActionList(int actionList[81]) {
		int action = firstActionIndex(validAction);
		Mask128 validActionMask = validAction >> action;
		int i = 0;
		// loop while there is still a 1 (i.e. an action)
		while (validActionMask) {
			// test if action is valid
			if (validAction & (int128(1) << action)) {
				actionList[i++] = action;
			}
			validActionMask >>= 1;
			action++;
		}
		return validActionCount;
	}

	Mask128 randAction() {
		// cerr << "rand action" << endl;
		int randIndex = validActionCount == 1 ? 0 : random(0, validActionCount - 1);
		// get past trailing zero
		Mask128 action = int128(1) << firstActionIndex(validAction);
		// loop there is still a 1 (e.i. an action)
		// log();
		// cerr << "randIndex = " << randIndex << ", action = " << actionIndex(action) << endl;
		// cerr << mtos(validAction, 81) << endl;
		// cerr << mtos(action, 81) << endl;
		while (randIndex > 0 || !(action & validAction)) {
			randIndex -= action & validAction ? 1 : 0;
			action <<= 1;
		}
		// cerr << "action send  = " << mtos(action, 81) << endl;
		return action;
	}

	template<class T>
	void play(T t) = delete;

	void play(Mask128 action) {
		// int actionIndex = actionIndex(action);
		// if (actionIndex > 80) {
		// 	cerr << "wtf action = " << actionIndex << endl;
		// 	exit(0);
		// }
		// cerr << "Play" << endl;
		// cerr << "action is    = " << mtos(action, 81) << endl;

		int smallBoardIndex = (int128_ffs(action) - 1) / 9;

		Mask128 &workingBoard = myTurn ? myBoard : oppBoard;
		Mask16 &workingBigBoard = myTurn ? myBigBoard : oppBigBoard;

		// if (workingBoard & action) {
		// 	cerr << "action non disponible" << endl;
		// 	cerr << "freeCell     = " << mtos(~(nonFreeCell | myBoard | oppBoard), 81) << endl;
		// 	cerr << "validAction  = " << mtos(validAction, 81) << endl;
		// 	cerr << "action       = " << mtos(action, 81) << endl;
		// 	cerr << "workingBoard = " << mtos(workingBoard, 81) << endl;
		// 	exit(0);
		// }
		workingBoard |= action;
		int result = boardIsFinal(getUniqueSmallBoard(workingBoard, smallBoardIndex));
		if (result) {
			workingBigBoard |= result << smallBoardIndex;
			nonFreeCell |= smallBoardMask(smallBoardIndex);
		}
		nonFreeCell |= action;

		depth++;
		myTurn = !myTurn;
		lastAction = action;
		validActionCount = 0;
		validActionComputed = false;
		computeValidAction();
	}

	bool final() {
		return validActionCount == 0 || boardIsFinal(myBigBoard) || boardIsFinal(oppBigBoard);
	}

	float result() {
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

	void log() {
		string str("\
. . . | . . . | . . .\n\
. . . | . . . | . . .\n\
. . . | . . . | . . .\n\
------|-------|------\n\
. . . | . . . | . . .\n\
. . . | . . . | . . .\n\
. . . | . . . | . . .\n\
------|-------|------\n\
. . . | . . . | . . .\n\
. . . | . . . | . . .\n\
. . . | . . . | . . ."
		);
		Mask128 myBoardTmp = myBoard;
		Mask128 oppBoardTmp = oppBoard;
		for (size_t i = 0; i < 81; i++) {

			if ((myBigBoard >> (i / 9)) & 1) {
				if (i % 9 != 4)
					str[gameIndexToStrIndex[i]] = 'o';
				else
					str[gameIndexToStrIndex[i]] = ' ';
			}
			else if ((oppBigBoard >> (i / 9)) & 1) {
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
		cerr << "myTurn = " << myTurn << "  lastAction = " << (actionIndex != -1 ? indexToPos[actionIndex(actionIndex)] : "none") << endl;
		cerr << str << endl;
	}
};

struct State {
	float value;
	int visitCount;
	Game game;
	State *parent;
	State *children[81];
	int childrenCount;
	int id;

	State(Game __game, State *__parent) : value(0), visitCount(0), game(__game), parent(__parent), childrenCount(0), id(stateId++) {}
	~State() {
		for (size_t i = 0; i < childrenCount; i++)
			delete children[i];
	}

	State *expand() {
		// if final state return current state
		if (game.final())
			return this;

		// create games for each valid action
		vector<Game> nextGame;
		int actionList[81];
		Game g;
		game.getActionList(actionList);
		for (size_t i = 0; i < game.validActionCount; i++) {
			g = game;
			// cerr << actionList[i] << endl;
			g.play(actionMask(actionList[i]));
			nextGame.push_back(g);
		}
		
		// detect if there is a final state
		Game *finalGame = NULL;
		for (size_t i = 0; i < game.validActionCount; i++) {
			if (nextGame[i].final()) {
				finalGame = &nextGame[i];
				break;
			}
		}

		childrenCount = 0;
		// if there is a final state: expand only this one
		if (finalGame != NULL) {
			children[childrenCount++] = new State(*finalGame, this);
		}
		// else: expand all next state
		else {
			for (size_t i = 0; i < nextGame.size(); i++)
				children[childrenCount++] = new State(nextGame[i], this);
		}
		return children[0];
	}

	float UCB1() {
		if (parent == NULL)
			return -1;
		
		if (visitCount == 0)
			return __builtin_huge_valf();
		
		return (value / visitCount) + 2 * sqrt(::log(parent->visitCount) / visitCount);
	}

	State *maxUCB1Child() {
		State *child = NULL;
		float maxUCB1 = 0;
		for (size_t i = 0; i < childrenCount; i++) {
			float UCB1 = children[i]->UCB1();
			if (UCB1 > maxUCB1) {
				maxUCB1 = UCB1;
				child = children[i];
			}
		}
		return child;
	}

	void backpropagate(float __value, State *root) {
		visitCount++;
		value += __value;
		if (parent != NULL && this != root)
			parent->backpropagate(__value, root);
	}

	float rollout() {
		Game g = game;
		while (!g.final()) {
			g.play(g.randAction());
		}
		return g.result();
	}

	State *maxAverageValueChild() {
		if (childrenCount == 0)
			return NULL;
		State *child = children[0];
		float maxAverageValue = -1;
		for (size_t i = 0; i < childrenCount; i++) {
			float averageValue = children[i]->value / children[i]->visitCount;
			if (averageValue > maxAverageValue) {
				maxAverageValue = averageValue;
				child = children[i];
			}
		}
		return child;
	}

	void log() {
		cerr << "State{t=" << setw(7) << left << value <<
            ",n=" << setw(5) << left << visitCount <<
            ",av=" << setw(10) << left << value / visitCount <<
            ",action=" << (game.lastAction != -1 ? indexToPos[actionIndex(game.lastAction)] : "none") <<
            "}" << endl;
	}

};

State *opponentPlay(State *state, Mask128 action) {
	for (size_t i = 0; i < state->childrenCount; i++) {
		if (action == state->children[i]->game.lastAction)
			return state->children[i];
	}
	cerr << "Invalid action " << actionIndex(action) << " in:" << endl;
	for (size_t i = 0; i < state->childrenCount; i++) {
		cerr << actionIndex(state->children[i]->game.lastAction) << " ";
	}
	cerr << endl;
	state->game.log();
	exit(1);
	return state;
}

void readInput(Mask128 &oppAction, int *validAction) {
	int opp_row; int opp_col;
	cin >> opp_row >> opp_col; cin.ignore();
	if (opp_row == -1)
		oppAction = 0;
	else
		oppAction = int128(1) << posToIndex[opp_row][opp_col];
	// cerr << mtos(oppAction, 81) << endl;

	int valid_action_count;
	cin >> valid_action_count; cin.ignore();
	for (int i = 0; i < valid_action_count; i++) {
	    int row; int col;
	    cin >> row >> col; cin.ignore();
	    validAction[i] = posToIndex[row][col];
	}
}

void generateInput(State *current, Mask128 &oppAction, int *validAction) {
	oppAction = current->game.randAction();
}

State *mcts(State *initialState, Timer start, float timeout) {
	State *current;
    int nbOfSimule = 0;

	while (true) {
        double diff = start.diff(false);
        if (diff > timeout)
            break;

		current = initialState;

		timer.set();
		while (current->childrenCount > 0)
			current = current->maxUCB1Child();

		if (current->visitCount > 0)
			current = current->expand();

		float value = current->rollout();

		current->backpropagate(value, initialState);

        nbOfSimule++;
	}
    cerr << "nb of simule = " << nbOfSimule << endl;
	return initialState->maxAverageValueChild();
}

State *mcts(State *initialState, int maxIter) {
	State *current;
    int nbOfSimule = 0;

	while (nbOfSimule < maxIter) {

		current = initialState;

		while (!current->childrenCount)
			current = current->maxUCB1Child();

		if (current->visitCount > 0)
			current = current->expand();

		float value = current->rollout();

		current->backpropagate(value, initialState);

        nbOfSimule++;
	}
    cerr << "nb of simule = " << nbOfSimule << endl;
	return initialState->maxAverageValueChild();
}

// int main(int ac, char *av[]) {

// 	srand(time(NULL));

// 	Game game = Game(0, 0, 0, 0, 0, -1, 0);

// 	State *state = new State(game, NULL);

// 	Timer start;

// 	State *child = mcts(state, start, 1000);
// 	cerr << "Simulation time = " << start.diff() << endl;
// 	// child->game.log();

// 	for (size_t i = 0; i < state->game.validActionCount; i++)
// 		state->children[i]->log();

// 	// while (!game.final()) {
// 	// 	cerr << "- NEXT TURN -" << endl;
// 	// 	game.play(game.randAction());
// 	// 	string str;
// 	// 	getline(cin, str);
// 	// }

// 	delete state;
// 	return 0;
// }

int main() {
	srand(time(NULL));

	Mask128 oppAction;
	int validAction[81];

	Game initialGame = Game(0, 0, 0, 0, 0, -1, 0);
	State *initialState = new State(initialGame, NULL);

    State *current = initialState;

	int first = true;
    while (1) {

		if (current->game.final()) {
			cerr << "result = " << current->game.result() << endl;
			delete initialState;
			return 0;
		}

		readInput(oppAction, validAction);
		// generateInput(current, oppAction, validAction);
        Timer start;

		if (current->game.final()) {
			cerr << "result = " << current->game.result() << endl;
			delete initialState;
			return 0;
		}

		if (first) {
			if (oppAction != 0)
				current->game.play(oppAction);
			else
				current->game.myTurn = 1;
		}
		else {
			current = opponentPlay(current, oppAction);
		}

        current->game.log();
		// string str;
		// getline(cin, str);

        // my play
        State *child = mcts(current, start, first ? 990 : 90);

		// for (size_t i = 0; i < current->game.validActionCount; i++)
		// 	current->children[i]->log();

        cerr << "simule time " << start.diff() << endl;

        if (child == NULL) {
            cerr << "mcts did not return any action" << endl;
            cout << indexToPos[validAction[0]] << endl;
			current = opponentPlay(current, validAction[0]);
        }
        else {
            cout << indexToPos[actionIndex(child->game.lastAction)] << endl;
            current = child;
        }
        current->game.log();
        cerr << endl;
		first = false;
		// getline(cin, str);
    }
}