#include <iostream>
#include <vector>
#include <string>
#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <functional> 

const int ROWS = 10,
COLS = 10,
COUNT_BATTLESHIPS = 0,
COUNT_CRUISERS = 0,
COUNT_DESTROYERS = 0,
COUNT_SUBMARINES = 1;

sf::TcpListener _listener;

enum Position {
	EMPTY,
	FILLED,
	DESTROYED,
	MISSED,
};

enum Ship {
	BATTLESHIP = 4,
	CRUISER = 3,
	DESTROYER = 2,
	SUBMARINE = 1,
};

enum GameState {
	STATE_WAITING_PLAYERS,
	STATE_PLACING_SHIPS,
	STATE_FIGHTING,
	STATE_GAME_FINISHED,
};

struct Player {
	std::vector<Ship> ships;
	Position Field[ROWS][COLS];
	std::string name;
	sf::TcpSocket *socket;

	Player() {
		name = "";
		for (int i = 0; i < COUNT_BATTLESHIPS; i++) {
			ships.push_back(Ship::BATTLESHIP);
		}
		for (int i = 0; i < COUNT_DESTROYERS; i++) {
			ships.push_back(Ship::DESTROYER);
		}
		for (int i = 0; i < COUNT_CRUISERS; i++) {
			ships.push_back(Ship::CRUISER);
		}
		for (int i = 0; i < COUNT_SUBMARINES; i++) {
			ships.push_back(Ship::SUBMARINE);
		}
	}
};

struct Game {
	GameState gameState;
	std::vector<Player*> players;
};

Game game;

Player player1,
player2;

int playerTurn = 0;

void ResetField(Player *player) {
	for (int x = 0; x < COLS; x++) {
		for (int y = 0; y < ROWS; y++) {
			player->Field[y][x] = EMPTY;
		}
	}
}

bool PlaceShip(Player *player, int x1, int x2, int y1, int y2) {

	int x_bigger = x1 > x2 ? x1 : x2;
	int y_bigger = y1 > y2 ? y1 : y2;

	int x_smaller = x_bigger == x1 ? x2 : x1;
	int y_smaller = y_bigger == y1 ? y2 : y1;

	if (x_bigger != x_smaller && y_bigger != y_smaller) return false;

	Ship shipPlaced = (Ship)(
		(x_bigger != x_smaller ?
			y_bigger - y_smaller : x_bigger - x_smaller)
		+ 1);

	bool shipWasntPlaced = false;

	for (auto it = player->ships.begin(); it != player->ships.end(); it++) {
		if (*it == shipPlaced) {
			shipWasntPlaced = true;
			player->ships.erase(it);
			break;
		}
	}

	if (!shipWasntPlaced) return false;

	for (int x = x_smaller; x <= x_bigger; x++) {
		for (int y = y_smaller; y <= y_bigger; y++) {

			if (x < 0 || x > COLS || y < 0 || y > ROWS) continue;

			//down
			if (player->Field[y + 1][x] == FILLED) {
				return false;
			}
			//downleft
			if (player->Field[y + 1][x - 1] == FILLED) {
				return false;
			}
			//left
			if (player->Field[y][x - 1] == FILLED) {
				return false;
			}
			//upleft
			if (player->Field[y - 1][x - 1] == FILLED) {
				return false;
			}
			//up
			if (player->Field[y - 1][x] == FILLED) {
				return false;
			}
			//upright
			if (player->Field[y - 1][x + 1] == FILLED) {
				return false;
			}
			//right
			if (player->Field[y][x + 1] == FILLED) {
				return false;
			}
			//downright
			if (player->Field[y + 1][x + 1] == FILLED) {
				return false;
			}
		}
	}

	for (int x = x_smaller; x <= x_bigger; x++) {
		for (int y = y_smaller; y <= y_bigger; y++) {
			player->Field[y][x] = FILLED;
		}
	}

	return true;
}

char squareToChar(Position *position) {
	switch (*position) {
	case Position::EMPTY:
		return ' ';
	case Position::DESTROYED:
		return 'X';
	case Position::FILLED:
		return '0';
	case Position::MISSED:
		return '-';
	default:
		return 'd';
	}
}

void PrintSquare(Position *position) {
	std::cout << "[";
	std::cout << squareToChar(position);
	std::cout << "]";
}

void PrintFields() {
	std::cout << "\n\n           PLAYER 1                          PLAYER 2";
	for (int y = 0; y < ROWS; y++) {
		std::cout << "\n";
		for (int x = 0; x < COLS; x++) {
			PrintSquare(&(game.players[0]->Field[y][x]));
		}
		std::cout << "    ";
		for (int x = 0; x < COLS; x++) {
			PrintSquare(&(game.players[1]->Field[y][x]));
		}
	}
}

bool HitField(Position field[ROWS][COLS], int x, int y) {
	Position *p_pos = &field[y][x];
	if (*p_pos == EMPTY || *p_pos == FILLED) {
		
		field[y][x] = *p_pos == EMPTY ? MISSED : DESTROYED;
		
		return true;
	}

	return false;
}

bool noShipsLeft() {
	for (auto it = game.players.begin(); it != game.players.end(); it++) {
		if ((*it)->ships.size() != 0) return false;
	}
	return true;
}

bool fieldDestroyed(Position field[ROWS][COLS]) {
	for (int x = 0; x < COLS; x++) {
		for (int y = 0; y < ROWS; y++) {
			if (field[y][x] == FILLED) {
				return false;
			}
		}
	}
	return true;
}

int getNextPlayerIndex() {
	return ( playerTurn + 1) % game.players.size();
}

char *fieldsToStr() {
	const int size = ROWS * COLS * 2;
	char returned[size];
	for (int i = 0; i < size; i++) {
		for (int y = 0; y < COLS; y++) {
			for (int x = 0; x < ROWS; x++) {
				returned[i] = squareToChar(game.players[playerTurn]->Field[y, x]);
			}
		}
	}
	return returned;
}

char* responceStr(char* msgReceived) {
	std::string receivedStr(msgReceived);
	switch (game.gameState) {
	case GameState::STATE_WAITING_PLAYERS:
		std::cout << "\nType 'rdy' to start the game: ";
		if (receivedStr == "rdy") {
			std::cout << "\nPlacing ships...";
			game.gameState = STATE_PLACING_SHIPS;
		}
		else {
			std::cout << "\nWaiting...";
		}
		break;
	case GameState::STATE_PLACING_SHIPS:
		std::cout << "\nType position x1 and x2, y1 and y2 to place ship on it: ";

		if (receivedStr.size() < 4) {
			std::cout << "\nIncorrect text entered amount.";
			break;
		}
		else if (PlaceShip(game.players[playerTurn], receivedStr[0] - '0', receivedStr[1] - '0', receivedStr[2] - '0', receivedStr[3] - '0')) {
			playerTurn = getNextPlayerIndex();
			PrintFields();
		}
		else {
			std::cout << "\nShip cannot be placed here. Try again.";
		}
		if (noShipsLeft()) {
			std::cout << "\nShip placing is done. The battle begins now!";
			game.gameState = GameState::STATE_FIGHTING;
		}
		break;
	case GameState::STATE_FIGHTING:
		std::cout << "\nWrite x and y to hit some pos: ";
		if (receivedStr.size() < 2) {
			std::cout << "\nIncorrect text entered amount.";
			break;
		}
		else if (HitField(game.players[getNextPlayerIndex()]->Field, receivedStr[0] - '0', receivedStr[1] - '0')) {
			if (fieldDestroyed(game.players[getNextPlayerIndex()]->Field)) {

				std::cout << "Game finished!";
				game.gameState = GameState::STATE_GAME_FINISHED;
			}
			playerTurn = getNextPlayerIndex();
			PrintFields();
		}
		else {
			std::cout << "\nYou already shooted this position.";
		}
		break;
	case GameState::STATE_GAME_FINISHED:
		//std::cin >> receivedStr;
		break;
	}
	return fieldsToStr();
}

void sendMessage(sf::TcpSocket* socket, char *message) {
	char messageToSend[400];

	for (int i = 0; i < 400; i++) {
		messageToSend[i] = '\0';
	}

	strcpy(messageToSend, message);
	if (socket->send(messageToSend, 400) != sf::Socket::Done) {
		std::cout << "\nSend message failed.";
	}
	else {
		std::cout << "\nMessage sent: " + *messageToSend;
	}
}

void sendMessageClients(char *message) {
	for (int i = 0; i < game.players.size(); i++) {
		sendMessage(game.players[i]->socket, message);
	}
}

void recieveInfoHandler(std::reference_wrapper<sf::TcpSocket*> socket) {
	while (true) {
		std::size_t received_buf_size = 400;
		char data_received[400];

		if (socket.get()->receive(data_received, 400, received_buf_size) != sf::Socket::Done)
		{
			std::cout << "\nClient disconnected.";
			return;
		}
		else {
			std::cout << std::string("\nText recieved: ").append(data_received).c_str();
			sendMessageClients(responceStr(data_received));
		}
	}
}

void acceptClient() {
	sf::TcpSocket clientSocket;

	if (_listener.accept(clientSocket) != sf::Socket::Done)
	{
		std::cout << "\nCan't accept client connection.";
	}
	else {
		std::cout << "\nFirst client connected with ip: ";
		std::cout << clientSocket.getRemoteAddress();

		Player *player = &Player();
		ResetField(player);
		player->socket = &clientSocket;
		game.players.push_back(player);

		std::cout << "\nPlayer 1 connected.";
		
		auto socketRef = std::ref(player->socket);
		
		std::thread acceptThread(acceptClient);
		acceptThread.detach();
		
		recieveInfoHandler(socketRef);
	}
}

int main() {
	_listener.listen(53000, sf::IpAddress::getLocalAddress());

	acceptClient();

	playerTurn = 0;

	std::cout << '\n';
	system("pause");
}