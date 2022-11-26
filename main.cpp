#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <random>
#include <cstdlib>
#include <ctime>
#include <iomanip>

using namespace std;

typedef vector<vector<int>> Grid;

int stateId = 0;

int random(int min, int max) {
   static bool first = true;
   if (first) {  
      srand(time(NULL));
      first = false;
   }
   return min + rand() % (max - min);
}

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
    string to_str() { return to_string(row) + " " + to_string(col); }
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

	bool final() {
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

	void log() {
		for (int row = 0; row < 3; row++) {
			for (int col = 0; col < 3; col++) {
				cerr << (board[row][col] == 0 ? '.' : board[row][col] == 1 ? 'o' : 'x') << " ";
			}
			if (row == 1)
				cerr << " next " << (turn == 1 ? 'o' : 'x');
			cerr << endl;
		}
	}
};

struct State {
	float value;
	int nb_of_visit;
	Game game;
	State *parent;
	vector<State *> children;
	int id;

	State(Game __game, State *__parent) : value(0), nb_of_visit(0), game(__game), parent(__parent), id(stateId++) {}
	~State() {
		for (size_t i = 0; i < children.size(); i++)
			delete children[i];
	}

	State *expand() {
		// if final state return current state
		if (game.final())
			return this;

		// get all valid action
		vector<Action> action = game.possibleActions();

		// create games for each valid action
		vector<Game> nextGame;
		for (size_t i = 0; i < action.size(); i++)
			nextGame.push_back(game.play(action[i]));
		
		// detect if there is a final state
		Game *finalGame = NULL;
		for (size_t i = 0; i < nextGame.size(); i++) {
			if (nextGame[i].final()) {
				finalGame = &nextGame[i];
				break;
			}
		}

		// if there is a final state: expand only this one
		if (finalGame != NULL) {
			children.push_back(new State(*finalGame, this));
		}
		// else: expand all next state
		else {
			for (size_t i = 0; i < nextGame.size(); i++)
				children.push_back(new State(nextGame[i], this));
		}
		return children[0];
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
		for (size_t i = 0; i < children.size(); i++) {
			float UCB1 = children[i]->UCB1();
			if (UCB1 > maxUCB1) {
				maxUCB1 = UCB1;
				child = children[i];
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

	float rollout(bool debug = false) {
		Game g = game;
		while (!g.final()) {
			vector<Action> actions = g.possibleActions();
			for (size_t i = 0; i < actions.size(); i++) {
				Game test = g.play(actions[i]);
				if (test.final()) {
					if (debug) {
						cerr << "best play -> " << actions[i].to_str() << endl;
						test.log();
					}
					return test.result();
				}
			}
			int randIndex = random(0, actions.size());
			g = g.play(actions[randIndex]);
			if (debug) {
				cerr << "random play -> " << actions[randIndex].to_str() << endl;
				g.log();
				cerr << endl;
			}
		}
		return g.result();
	}

	State *maxAverageValueChild() {
		if (children.empty())
			return NULL;
		State *child = children[0];
		float maxAverageValue = -1;
		for (size_t i = 0; i < children.size(); i++) {
			float averageValue = children[i]->value / children[i]->nb_of_visit;
			if (averageValue > maxAverageValue) {
				maxAverageValue = averageValue;
				child = children[i];
			}
		}
		return child;
	}

	void log() {
		cerr << "State{t=" << setw(7) << left << value <<
            ",n=" << setw(5) << left << nb_of_visit <<
            ",av=" << setw(10) << left << value / nb_of_visit <<
            ",action=" << game.lastAction.to_str() <<
            "}" << endl;
	}

};

void dump_node(State *node, std::ofstream & myfile, int deep) {
    if (--deep <= 0)
        return;
    myfile << '\t' << node->id << "[label=\"[" << node->game.lastAction.to_str() << "]\n"
    <<"P=" << node->game.turn << "W=" << node->value << ";V=" << node->nb_of_visit<< '\"';
    // if (isTerminal(node->bigboard))
    // {
    //     myfile << " color=\"";
    //     if (isWin(node->bigboard))
    //         myfile << "green";
    //     else if (isLost(node->bigboard))
    //         myfile << "red";
    //     else
    //         myfile << "blue";
    //     myfile << '\"';
    // }
    myfile <<"]\n";
    for(int i = 0; i < node->children.size(); i++) {
        myfile << '\t' << node->id << " -> " << node->children[i]->id << std::endl;
    }
    for (int i = 0; i < node->children.size(); i++) {
        dump_node(node->children[i], myfile, deep);
    }
}

void dump_tree(std::string fileName, State *root) {
    std::ofstream myfile;
	myfile.open(fileName);
    myfile << "strict digraph {\n";
    myfile << "\tnode [shape=\"rect\"]\n";

    dump_node(root, myfile, 5);

    myfile << "}";
    myfile.close();
}

State *opponentPlay(State *state, Action action) {
	for (size_t i = 0; i < state->children.size(); i++) {
		if (action == state->children[i]->game.lastAction)
			return state->children[i];
	}
	return state;
}

void readInput(Action &opponentAction, vector<Action> &validAction) {
	int opponent_row; int opponent_col;
	cin >> opponent_row >> opponent_col; cin.ignore();
	opponentAction.set(opponent_row, opponent_col);
	validAction.clear();
	// int valid_action_count;
	// cin >> valid_action_count; cin.ignore();
	// for (int i = 0; i < valid_action_count; i++) {
	//     int row; int col;
	//     cin >> row >> col; cin.ignore();
	//     validAction.push_back(Action(row, col));
	// }
}

State *mcts(State *initialState, Timer start, float timeout) {
	State *current;
    int nbOfSimule = 0;

	// if (initialState->children.empty())
	// 	initialState->expand();
	while (true) {
		
        double diff = start.diff(false);
        if (diff > timeout)
            break;

		current = initialState;

		while (!current->children.empty())
			current = current->maxUCB1Child();

		if (current->nb_of_visit > 0) {
			State *firstChild = current->expand();
			if (firstChild != NULL)
				current = firstChild;
		}

		float value = current->rollout();

		current->backpropagate(value);

        nbOfSimule++;
	}
    cerr << "nb of simule = " << nbOfSimule << endl;
	return initialState->maxAverageValueChild();
}

State *mcts(State *initialState, int maxIter) {
	State *current;
    int nbOfSimule = 0;

	// if (initialState->children.empty())
	// 	initialState->expand();
	while (nbOfSimule < maxIter) {

		current = initialState;

		while (!current->children.empty())
			current = current->maxUCB1Child();

		if (current->nb_of_visit > 0) {
			current = current->expand();
		}

		float value = current->rollout();

		current->backpropagate(value);

        nbOfSimule++;
	}
    cerr << "nb of simule = " << nbOfSimule << endl;
	return initialState->maxAverageValueChild();
}

void rolloutTest(Grid board) {
	cerr << "--- rollout test ---" << endl;
	Game game = Game(board, 1, npos);
	State *state = new State(game, NULL);

	state->game.log();
	cerr << endl;

	float val = state->rollout(true);
	cerr << "\nvalue = " << val << endl;
	delete state;
	cerr << "--------------------" << endl;
}

void mctsTest(Grid board) {
	cerr << "---- mcts  test ----" << endl;
	Game game = Game(board, 1, npos);
	State *state = new State(game, NULL);

	state->game.log();
	cerr << endl;

	State *child = mcts(state, 1000);

	for (size_t i = 0; i < state->children.size(); i++)
		state->children[i]->log();
	child->game.log();
	delete state;
	cerr << "--------------------" << endl;
}

// int main(int ac, char *av[]) {

// 	Grid board = {
// 		{1,0,0},
// 		{0,-1,0},
// 		{0,-1,0}
// 	};

// 	rolloutTest(board);
// 	mctsTest(board);

// 	return 0;
// }

int main() {
	int step = 0;

	Action opponentAction;
	vector<Action> validAction;

	Game initialGame = Game({{0,0,0},{0,0,0},{0,0,0}}, 1, npos);
	State *initialState = new State(initialGame, NULL);

    State *current = initialState;

    while (1) {
		readInput(opponentAction, validAction);
        Timer start;

		if (step == 0) {
			if (opponentAction.row != -1) {
				current->game.board[opponentAction.row][opponentAction.col] = -1;
			}
		}
		else {
			current = opponentPlay(current, opponentAction);
		}

        current->game.log();
        cerr << endl;

        // my play
        State *child = mcts(current, start, 75);

		// dump_tree("tree.dot", current);

        cerr << "simule time " << start.diff() << endl;

        for (size_t i = 0; i < current->children.size(); i++)
            current->children[i]->log();
        
        if (child == NULL) {
            cerr << "mcts did not return any action" << endl;
            cout << validAction[0].to_str() << endl;
        }
        else {
            cout << child->game.lastAction.to_str() << endl;
            current = child;
        }
        current->game.log();
        cerr << endl;
		step++;
    }
}