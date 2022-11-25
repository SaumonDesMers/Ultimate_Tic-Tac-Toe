#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <cstdlib>

using namespace std;

typedef vector<vector<int>> Grid;

int StateId = 0;

int random(int min, int max) {
   static bool first = true;
   if (first) {  
      srand(time(NULL));
      first = false;
   }
   return min + rand() % (max - min);
}

struct Action {
	int row; int col;
	Action(int _row = 0, int _col = 0) : row(_row), col(_col) {}
	Action(string s) {
        string s_row = s.substr(0, s.find_first_not_of("-0123456789"));
        string s_col = s.substr(s.find_last_not_of("-0123456789"), s.size());
        row = atoi(s_row.c_str());
        row = atoi(s_col.c_str());
    }
	Action(Action const &src) : row(src.row), col(src.col) {}
	Action &operator=(Action const &src) { row = src.row; col = src.col; return *this; }
	bool operator==(Action const &src) const { return row == src.row && col == src.col; }
	bool operator!=(Action const &src) const { return !(*this == src); }
	void set(int _row, int _col) { row = _row; col = _col; }
	void add(int _row, int _col) { row += _row; col += _col; }
    void add(Action &v) { row += v.row; col += v.col; }
    string to_str() { return to_string(col) + " " + to_string(row); }
} npos(-1, -1);

struct Game {
	Grid board;
	int turn;
	Action lastAction;

	Game(Grid __board, int __turn, Action __lastAction) : board(__board), turn(__turn), lastAction(__lastAction) {}
	Game(const Game &src) : board(src.board), turn(src.turn), lastAction(src.lastAction) {}
	Game &operator=(const Game &src) {
		board = src.board;
		turn = src.turn;
		lastAction = src.lastAction;
		return *this;
	}

	int nbPossibleActions() {
		int n = 0;
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 3; col++) {
				if (board[row][col] == 0)
					n++;
			}
		}
		return n;
	}

	vector<Action> possibleActions() {
		vector<Action> action;
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 3; col++) {
				if (board[row][col] == 0)
					action.push_back(Action(row, col));
			}
		}
		return action;
	}

	Game play(Action action) {
		Game game(board, turn == 1 ? -1 : 1, action);
		game.board[action.row][action.col] = turn;
		return game;
	}

	bool finish() {
		if (nbPossibleActions() == 0)
			return true;
		return playerWin();
	}

	bool playerWin() {
		int x[8][6] = {
			{0,0,0,1,0,2},
			{1,0,1,1,1,2},
			{2,0,2,1,2,2},
			{0,0,1,0,2,0},
			{0,1,1,1,2,1},
			{0,2,1,2,2,2},
			{0,0,1,1,2,2},
			{0,2,1,1,2,0}
		};

		for (int i = 0; i < 8; i++) {
			int result = board[x[i][0]][x[i][1]] + board[x[i][2]][x[i][3]] + board[x[i][4]][x[i][5]];
			if (abs(result) == 3)
				return true;
		}
		return false;
	}

	float result() {
		int x[8][6] = {
			{0,0,0,1,0,2},
			{1,0,1,1,1,2},
			{2,0,2,1,2,2},
			{0,0,1,0,2,0},
			{0,1,1,1,2,1},
			{0,2,1,2,2,2},
			{0,0,1,1,2,2},
			{0,2,1,1,2,0}
		};

		for (int i = 0; i < 8; i++) {
			int result = board[x[i][0]][x[i][1]] + board[x[i][2]][x[i][3]] + board[x[i][4]][x[i][5]];
			if (abs(result) == 3)
				return result == 3 ? 1 : 0;
		}
		return 0.5;
	}

	void inverse() {
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 3; col++) {
				if (board[row][col] != 0)
					board[row][col] = board[row][col] == 1 ? -1 : 1;
			}
		}
		turn = turn == 1 ? -1 : 1;
	}

	void log(char me, char notme) {
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 3; col++) {
				cerr << (board[row][col] == 0 ? '.' : board[row][col] == 1 ? me : notme) << " ";
			}
			cerr << endl;
		}
	}
};

struct State {
	float value;
	int nb_of_visit;
	Game game;
	State *parent;
	vector<State *> childs;
	int id;

	State(Game __game, State *__parent) : value(0), nb_of_visit(0), game(__game), parent(__parent) {}
	~State() {
		for (size_t i = 0; i < childs.size(); i++)
			delete childs[i];
	}

	State *expand() {
		if (game.finish())
			return NULL;
		vector<Action> action = game.possibleActions();
		for (size_t i = 0; i < action.size(); i++)
			childs.push_back(new State(game.play(action[i]), this));
		return childs.size() > 0 ? childs[0] : NULL;
	}

	float UCB1() {
		if (parent == NULL)
			return -1;
		
		if (nb_of_visit == 0)
			return INFINITY;
		
		return (value / nb_of_visit) + 2 * sqrt(::log(parent->nb_of_visit) / nb_of_visit);
	}

	State *maxUCB1Child() {
		State *child = NULL;
		float maxUCB1 = 0;
		for (size_t i = 0; i < childs.size(); i++) {
			float UCB1 = childs[i]->UCB1();
			if (UCB1 > maxUCB1) {
				maxUCB1 = UCB1;
				child = childs[i];
			}
		}
		return child;
	}

	void backpropagate(float __value) {
		nb_of_visit++;
		value += __value;
		if (parent != NULL)
			parent->backpropagate(__value);
	}

	float rollout() {
		Game g = game;
		while (!g.finish()) {
			vector<Action> actions = g.possibleActions();
			int randIndex = random(0, actions.size());
			g = g.play(actions[randIndex]);
		}
		return g.result();
	}

	State *maxAverageValueChild() {
		if (childs.empty())
			return NULL;
		State *child = childs[0];
		float maxAverageValue = -1;
		for (size_t i = 0; i < childs.size(); i++) {
			float averageValue = childs[i]->value / childs[i]->nb_of_visit;
			if (averageValue > maxAverageValue) {
				maxAverageValue = averageValue;
				child = childs[i];
			}
		}
		return child;
	}

	void log() {
		cerr << "State{t=" << value << ",\tn=" << nb_of_visit << ",\tav=" << value / nb_of_visit << ",\taction=" << game.lastAction.to_str() << "}" << endl;
	}

};

State *mcts(State *initialState, int maxIter) {
	State *current;
	while (maxIter--) {
		current = initialState;
		// cerr << "Iteration " << maxIter << endl;

		while (current->childs.size() > 0)
			current = current->maxUCB1Child();


		if (current->nb_of_visit > 0) {
			State *firstChild = current->expand();
			if (firstChild != NULL)
				current = firstChild;
		}
		// cerr << "simule " << current->game.lastAction.to_str() << ":" << endl;
		// current->game.log('o', 'x');
		float value = current->rollout();
		// cerr << "result = " << value << endl << endl;
		current->backpropagate(value);
	}
	return initialState->maxAverageValueChild();
}

int main(int ac, char *av[]) {

	if (ac != 2) {
		cerr << "Not enough arguments" << endl;
		return 1;
	}

	Grid board = {
		{0,0,0},
		{0,-1,-1},
		{0,0,1}
	};

	Game game = Game(board, 1, npos);
	State *state = new State(game, NULL);

	game.log('o', 'x');
	State *child = mcts(state, atoi(av[1]));
	for (size_t i = 0; i < child->parent->childs.size(); i++)
		child->parent->childs[i]->log();
	child->game.log('o', 'x');

	delete state;


	// Grid emptyBoard = {
	// 	{0,0,0},
	// 	{0,0,0},
	// 	{0,0,0}
	// };

	// Game initialGame = Game(emptyBoard, 1, npos);
	// State *initialState = new State(initialGame, NULL);

	// State *current = initialState;
	// while (!current->game.finish()) {

	// 	current = mcts(current, atoi(av[1]));
	// 	if (!current) {
	// 		cerr << "current = NULL" << endl;
	// 		break;
	// 	}
	// 	for (size_t i = 0; i < current->parent->childs.size(); i++)
	// 		current->parent->childs[i]->log();
	// 	current->game.log('o', 'x');
	// 	cerr << endl;

	// 	current->game.inverse();
	// 	State *nextState = new State(current->game, NULL);
	// 	delete current->parent;
	// 	current = nextState;
	// }

	// delete current;

	return 0;
}